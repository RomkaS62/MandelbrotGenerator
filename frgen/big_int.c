#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>
#include "big_int.h"
#include "mbarray.h"
#include "mbitr.h"
#include "parse.h"
#include "log.h"

struct u64_split {
	uint32_t low;
	uint32_t high;
};

static inline struct u64_split split_u64(uint64_t u)
{
	struct u64_split ret;

	ret.low = (uint32_t)(u & 0xFFFFFFFFUL);
	ret.high = (uint32_t)(u >> 32);

	return ret;
}

static void ensure_capacity(struct mb_array_u32 *i, size_t capacity)
{
	if (capacity > i->len) {
		i->buf = realloc(i->buf, sizeof(i->buf[0]) * capacity);
		memset(i->buf + i->len, 0, (capacity - i->len) * sizeof(i->buf[0]));
		i->len = capacity;
	}
}

static inline void ptr_swap(void **p1, void **p2)
{
	void *tmp;
	tmp = *p1;
	*p1 = *p2;
	*p2 = tmp;
}

static struct big_int * bi_new_p(size_t precision)
{
	struct big_int *ret;

	ret = malloc(sizeof(*ret));
	ret->arr.buf = calloc(precision, sizeof(ret->arr.buf[0]));
	ret->arr.len = precision;

	return ret;
}

static struct big_int * bi_cpy_po(const struct big_int *i, size_t size, size_t offset)
{
	struct big_int *ret;

	ret = bi_new_p(size);
	memcpy(ret->arr.buf + offset, i->arr.buf, size * sizeof(i->arr.buf[0]));

	return ret;
}

struct big_int * bi_cpy_p(const struct big_int *i, size_t size)
{
	return bi_cpy_po(i, size, 0);
}

/* Adds val to buf in place at position 'at' while checking for overflow.
 * Returns any overflow the entire operation produced. */
static uint32_t u32arr_add_u32(uint32_t *buf, size_t len, size_t at, uint32_t val)
{
	uint64_t sum;
	uint32_t carry;

	if (!val)
		return 0;

	if (at >= len) {
		return val;
	}

	sum = (uint64_t)buf[at] + val;
	carry = sum >> 32;
	buf[at] = sum;
	if (carry) {
		if (at >= len) {
			return carry;
		}
		return u32arr_add_u32(buf, len, at + 1, carry);
	}

	return 0;
}

static uint32_t u32arr_sub_u32(uint32_t *buf, size_t len, size_t at, uint32_t val)
{
	uint64_t diff;
	uint32_t underflow;

	if (!val)
		return 0;

	if (at >= len) {
		return val;
	}

	diff = buf[at] - val;
	/* Unsigned subtraction modulo 2^32. At most, underflow can be 1. */
	underflow = !!(buf[at] < val);
	buf[at] = diff;
	if (underflow) {
		if (at >= len) {
			return underflow;
		}
		return u32arr_sub_u32(buf, len, at + 1, underflow);
	}

	return 0;
}

static void u32arr_sub_u64(uint32_t *buf, size_t len, size_t at, uint64_t val)
{
	struct u64_split s;

	s = split_u64(val);
	u32arr_sub_u32(buf, len, at, s.low);
	u32arr_sub_u32(buf, len, at + 1, s.low);
}

static unsigned mb_log2(unsigned long num)
{
	unsigned ret = 0;
	while (num >>= 1) {
		ret++;
	}

	return ret;
}

static void parse_pow2(
		size_t size,
		struct mb_array_u32 *ret,
		const char *str,
		unsigned radix)
{
	size_t i;
	size_t j;
	uint32_t u;		/* Parsed uint32_t from str. */
	size_t digits;		/* Total number of digits in str. */
	size_t blk_size;	/* Digits to completely fill a uint32_t. */
	size_t u32_blocks;	/* Number of such blocks. */
	size_t rem_digits;	/* Remaining digits that do not fill a uint32_t. */
	size_t u32s;
	unsigned digit_bits;
	int ch;
	char u32_char_block[16];

	digits = mb_num_of_digits(str, radix);
	blk_size = 1 << (mb_log2(radix) - 1);
	u32_blocks = digits / blk_size;
	rem_digits = digits % blk_size;
	digit_bits = 32 / blk_size;

	u32s = u32_blocks + !!rem_digits;

	if (u32s < size) {
		ret->len = size;
	} else {
		ret->len = u32s;
	}

	ret->buf = calloc(ret->len, sizeof(ret->buf[0]));

	/* Strings being parsed here are big-endian, but the underlying array
	 * is little-endian. A hex string like "FFFFFFFF1" would be parsed as
	 * 0x1 0xFFFFFFFF in memory. */
	foreach_strn_br(str + rem_digits, u32_char_block, blk_size, i, u32_blocks) {
		u = 0;
		foreach_strn_l(u32_char_block, ch, j, blk_size) {
			u <<= digit_bits;
			u |= mb_parse_digit(ch, radix);
		}
		ret->buf[i] = u;
	}
	u = 0;
	foreach_strn_l(str, ch, i, rem_digits) {
		u <<= digit_bits;
		u |= mb_parse_digit(ch, radix);
	}
	if (rem_digits)
		ret->buf[u32s - 1] = u;
}

static void u32arr_mul_u32_i(struct mb_array_u32 *ret, size_t digits, size_t at, uint32_t val)
{
	struct u64_split p;
	uint32_t row1_val;
	size_t i;

	foreach_reverse (ret->buf, digits, i, row1_val) {
		p = split_u64((uint64_t)row1_val * (uint64_t)val);
		u32arr_add_u32(ret->buf, ret->len, i + at + 1, p.high);
		u32arr_add_u32(ret->buf, ret->len, i + at, p.low);
	}
}

static void parse_generic(struct mb_array_u32 *ret, const char *str, unsigned radix)
{
	size_t digits;
	size_t buf_len;
	unsigned digit;
	int ch;

	digits = mb_num_of_digits(str, radix);
	buf_len = (size_t)ceil(digits * log2(radix) / 32.0);
	ret->buf = calloc(buf_len, sizeof(ret->buf[0]));
	ret->len = buf_len;

	for (ch = *str; *str; str++, ch = *str) {
		u32arr_mul_u32_i(ret, buf_len, 0, radix);
		digit = mb_parse_digit(ch, radix);
		u32arr_add_u32(ret->buf, ret->len, 0, digit);
	}
}

struct mb_array_u32 u32arr_from_str(size_t size, const char *str, unsigned radix)
{
	struct mb_array_u32 ret;

	switch (radix) {
	case 2:
	case 4:
	case 8:
	case 16:
	case 32:
		parse_pow2(size, &ret, str, radix);
		return ret;
	}

	if (radix > 10 + 'z' - 'a' + 1) {
		m_eprintf("Unsupported base %i\n", radix);
		ret.buf = NULL;
		ret.len = 0;
		return ret;
	}

	parse_generic(&ret, str, radix);

	return ret;
}

void bi_init_s(struct big_int *ret, size_t size, uint32_t init, size_t shift)
{
	if (!size)
		size = 1;

	if (size < shift + 1)
		size = shift + 1;

	ret->arr.buf = calloc(1, sizeof(ret->arr.buf[0]) * size);
	ret->arr.len = size;
	ret->arr.buf[shift] = init;
}

void bi_init_u64_s(struct big_int *ret, size_t size, uint64_t init, size_t shift)
{
	struct u64_split val;

	if (size < shift + 1)
		size = shift + 1;

	val = split_u64(init);
	ret->arr.buf = calloc(size, sizeof(ret->arr.buf[0]));
	ret->arr.buf[shift] = val.low;
	ret->arr.buf[shift + 1] = val.high;
}

void bi_init_from_u32arr(struct big_int *ret, size_t size, const uint32_t *init, size_t length)
{
	if (length > size)
		size = length;

	ret->arr.len = size;
	ret->arr.buf = calloc(size, sizeof(ret->arr.buf[0]));
	memcpy(ret->arr.buf, init, length * sizeof(ret->arr.buf[0]));
}

void bi_init_from_str(struct big_int *ret, size_t size, const char *str, int radix)
{
	ret->arr = u32arr_from_str(size, str, radix);
}

void bi_delete(struct big_int *i)
{
	if (!i)
		return;
	free(i->arr.buf);
	free(i);
}

int bi_cmp_u64(const struct big_int *a, uint64_t val)
{
	uint64_t a_val = 0;

	assert(a != NULL);

	if (a->arr.len > 2)
		return 1;

	if (a->arr.len >= 1)
		a_val = a->arr.buf[0];
	if (a->arr.len >= 2)
		a_val |= ((uint64_t)a->arr.buf[1]) << 32;

	if (a_val > val)
		return 1;
	else if (a_val < val)
		return -1;

	return 0;
}

int bi_cmp(const struct big_int *i1, const struct big_int *i2)
{
	size_t i;

	assert(i1 != NULL);
	assert(i2 != NULL);

	if (i1->arr.len > i2->arr.len) {
		return 1;
	} else if (i1->arr.len < i2->arr.len) {
		return -1;
	}

	foreach_reverse_idx (i, i1->arr.len) {
		if (i1->arr.buf[i] > i2->arr.buf[i]) {
			return 1;
		} else if (i1->arr.buf[i] < i2->arr.buf[i]) {
			return -1;
		}
	}

	return 0;
}

struct big_int * bi_cpy(const struct big_int *i)
{
	struct big_int *ret;

	assert(i != NULL);

	ret = malloc(sizeof(*ret));
	ret->arr.len = i->arr.len;
	ret->arr.buf = malloc(ret->arr.len * sizeof(ret->arr.buf[0]));
	memcpy(ret->arr.buf, i->arr.buf, ret->arr.len * sizeof(ret->arr.buf[0]));

	return ret;
}

int bi_is_zero(const struct big_int *i)
{
	assert(i != NULL);

	if (i->arr.len == 1) {
		if (i->arr.buf[0] == 0)
			return 1;
	}
	return 0;
}

void bi_set_zero(struct big_int *i)
{
	assert(i != NULL);

	i->arr.len = 1;
	i->arr.buf = realloc(i->arr.buf, i->arr.len * sizeof(i->arr.buf[0]));
	i->arr.buf[0] = 0;
}

/* --- Operator functions modifying first argument --- */

void bi_add_u64_is(struct big_int *i, uint64_t val, int32_t shift)
{
	assert(i != NULL);
	ensure_capacity(&i->arr, i->arr.len + 1);
	u32arr_add_u32(i->arr.buf, i->arr.len, val, shift);
}

void bi_sub_u64_is(struct big_int *i, uint64_t val, int32_t shift)
{
	int cmp;

	assert(i != NULL);

	if (val == 0)
		return;

	cmp = bi_cmp_u64(i, val);

	if (cmp <= 0) {
		bi_set_zero(i);
		return;
	}

	u32arr_sub_u64(i->arr.buf, i->arr.len, shift, val);
}

void bi_mul_u64_is(struct big_int *i, uint64_t val, int32_t shift)
{
	size_t digits;
	struct u64_split s;

	assert(i != NULL);

	if (bi_is_zero(i) || val == 0) {
		bi_set_zero(i);
		return;
	}

	digits = i->arr.len;
	s = split_u64(val);
	ensure_capacity(&i->arr, i->arr.len + 2);
	u32arr_mul_u32_i(&i->arr, digits, s.high, 1 + shift);
	u32arr_mul_u32_i(&i->arr, digits, s.low, shift);
}

void bi_add_is(struct big_int *ret, const struct big_int *num, int32_t shift)
{
	size_t i;

	assert(ret != NULL);
	assert(num != NULL);

	ensure_capacity(&ret->arr, ret->arr.len + 1);
	for (i = 0; i < num->arr.len; i++) {
		u32arr_add_u32(ret->arr.buf, ret->arr.len, i + shift, num->arr.buf[i]);
	}
}

void bi_sub_is(struct big_int *ret, const struct big_int *num, int32_t shift)
{
	size_t i;

	assert(ret != NULL);
	assert(num != NULL);

	for (i = 0; i < num->arr.len; i++) {
		u32arr_sub_u32(ret->arr.buf, ret->arr.len, i + shift, num->arr.buf[i]);
	}
}

void bi_mul_is(struct big_int *ret, const struct big_int *op, int32_t shift)
{
	size_t i;
	size_t j;
	size_t row1_digits;
	uint32_t row1_val;
	uint32_t row2_val;
	struct u64_split p;

	assert(ret != NULL);
	assert(op != NULL);

	if (bi_is_zero(ret) || bi_is_zero(op)) {
		bi_set_zero(ret);
		return;
	}

	/* Multiply first digit of first row with all digits of bottom row and
	 * add the resulting digits with carry where they would usually go.
	 *
	 * That place "where it would usually go" is index of first row digit +
	 * index of digit from second row.
	 *
	 *		[4]	[3]	[2]	[1]	[0]
	 *				1	2	3
	 *				4	5	6
	 *				-----------------
	 *		4	5	6
	 *			8	10	12
	 *				12	15	18
	 *		----------------------------------
	 *		4	13	28	27	18
	 * */
	row1_digits = ret->arr.len;	/* Number of digits in first row. */
	ensure_capacity(&ret->arr, ret->arr.len + op->arr.len);

	/* op takes place of the second row. Iterate over it in reverse,
	 * multiply each digit with ret and add the product to ret shifted by
	 * its own length + i - 1 (also in reverse). */
	foreach_reverse (ret->arr.buf, row1_digits, i, row1_val) {
		if (!row1_val)
			continue;
		foreach_reverse (op->arr.buf, op->arr.len, j, row2_val) {
			if (!row2_val)
				continue;
			p = split_u64((uint64_t)row1_val * (uint64_t)row2_val);
			u32arr_add_u32(ret->arr.buf, ret->arr.len, i + j + 1 + shift, p.high);
			if (j)
				u32arr_add_u32(ret->arr.buf, ret->arr.len, i + j + shift, p.low);
			else
				ret->arr.buf[i] = p.low;
		}
	}
}

static char u32_to_char(uint32_t num)
{
	assert(num <= 127);

	if (num < 10) {
		return '0' + num;
	} else {
		return 'A' + (char)num - 10;
	}
}

/* Write uint32_t to str in big-endian format while skipping most significant
 * zeroes. */
static void write_u32_pow4_sz(char **str, uint32_t num, unsigned bits)
{
	uint32_t mask;
	unsigned iterations;
	uint32_t radix_digit;
	size_t i;

	mask = (uint32_t)(~(~0UL << bits));
	iterations = 32 / bits;

	for (i = 0; i < iterations; i++) {
		if ((num >> (bits * (iterations - i - 1))) & mask)
			break;
	}

	for (; i < iterations; i++) {
		radix_digit = (num >> (bits * (iterations - i - 1))) & mask;
		**str = u32_to_char(radix_digit);
		*str += 1;
	}
}

static void write_u32_pow4(char **str, uint32_t num, unsigned bits)
{
	unsigned iterations;
	uint32_t mask;
	size_t i;

	assert(bits <= 4);

	iterations = 32 / bits;
	mask = (uint32_t)(~(~0UL << bits));

	for (i = 0; i < iterations; i++) {
		**str = u32_to_char((num >> (bits * (iterations - i - 1))) & mask);
		*str += 1;
	}
}

static unsigned u32_digits_pow4(uint32_t digit, unsigned bits)
{
	unsigned ret = 0;

	while (digit) {
		ret++;
		digit >>= bits;
	}

	return ret;
}

/* A more efficient to string function for radix powers of 4. */
static char * tostr_pow4(const struct big_int *num, unsigned radix)
{
	unsigned bits;		/* Bits represented by a signgle character. */
	unsigned msb_digits;	/* Digits in radix of most significant uint32_t. */
	char *str;
	char *write_head;
	size_t i;
	uint32_t u32_digit;

	bits = mb_log2(radix);
	msb_digits = u32_digits_pow4(num->arr.buf[num->arr.len - 1], bits);
	str = malloc((num->arr.len - 1) * (32 / bits) + msb_digits + 1);
	write_head = str;

	write_u32_pow4_sz(&write_head, num->arr.buf[num->arr.len - 1], bits);

	foreach_reverse (num->arr.buf, num->arr.len - 1, i, u32_digit) {
		write_u32_pow4(&write_head, u32_digit, bits);
	}

	*write_head++ = '\0';
	return str;
}

static char * copy_str(const char *str)
{
	char *ret;
	size_t len;

	len = strlen(str);
	ret = malloc(len);
	memcpy(ret, str, len);

	return ret;
}

char * bi_tostr(const struct big_int *num, unsigned radix)
{
	assert(num != NULL);

	if (bi_is_zero(num))
		return copy_str("0");

	switch (radix) {
	case 2:
	case 4:
	case 16:
	case 32:
		return tostr_pow4(num, radix);
	}

	/* Implementing conversion to a non-power of 2 radix string would
	 * require division. Division is slow as all hell and is not actually
	 * needed to draw an image of the mandelbrot set. For the time being,
	 * it will remain unimplemented.*/
	m_eprintf("Conversion to string in base %u is not supported yet.\n",
			radix);

	return NULL;
}

