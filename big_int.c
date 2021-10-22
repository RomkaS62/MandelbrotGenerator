#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include "big_int.h"
#include "mbarray.h"
#include "mbitr.h"
#include "parse.h"
#include "log.h"

static uint32_t zero_buf = 0;
static const struct big_int bi_zero = {
	{ &zero_buf, 1 }
};

static uint32_t one_buf = 1;
static const struct big_int bi_one = {
	{ &one_buf, 1 }
};

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

static size_t trailing_u32(uint32_t *buf, size_t len, uint32_t val)
{
	size_t i;
	size_t ret = 0;
	uint32_t u;

	foreach_reverse (buf, len, i, u) {
		if (u != val)
			break;
		ret++;
	}

	return ret;
}

static void remove_trailing(uint32_t **buf, size_t *len, size_t remove)
{
	*buf = realloc(*buf, sizeof(**buf) * (*len - remove));
	*len -= remove;
}

/* When adding, subtracting or multiplying one extra uint32_t may appear at the
 * end of the underlying array. It needs to be removed. */
static void normalize(struct big_int *i)
{
	size_t trailing_zeroes;

	if (i->arr.len == 1)
		return;

	trailing_zeroes = trailing_u32(i->arr.buf, i->arr.len, 0);
	trailing_zeroes = (trailing_zeroes < i->arr.len) ? trailing_zeroes : i->arr.len - 1;
	remove_trailing(&i->arr.buf, &i->arr.len, trailing_zeroes);
}

static void ensure_capacity(struct mb_array_u32 *i, size_t capacity)
{
	if (capacity <= i->len) {
		i->buf = realloc(i->buf, sizeof(i->buf[0]) * capacity);
		memmove(i->buf + capacity - i->len, i->buf, i->len);
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

static struct big_int * bi_cpy_p(const struct big_int *i, size_t size)
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
		fprintf(stderr, "Attempted add at %zu in array of size %zu\n", at, len);
		exit(1);
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

static void u32arr_add_u64(uint32_t *buf, size_t len, size_t at, uint64_t val)
{
	struct u64_split s;

	s = split_u64(val);
	u32arr_add_u32(buf, len, at, s.low);
	u32arr_add_u32(buf, len, at + 1, s.high);
}

static uint32_t u32arr_sub_u32(uint32_t *buf, size_t len, size_t at, uint32_t val)
{
	uint64_t diff;
	uint32_t underflow;

	if (!val)
		return 0;

	if (at >= len) {
		fprintf(stderr, "Attempted subtraction at %zu in array of size %zu\n", at, len);
		exit(1);
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

/* Multiplies arr with val and adds the result to ret starting at offset 'at' */
static void u32arr_mul_u32(
		const struct mb_array_u32 *arr,
		uint32_t val,
		size_t at,
		struct mb_array_u32 *ret)
{
	size_t i;
	size_t digits_to_multiply;
	uint64_t p;
	uint32_t pl;
	uint32_t ph;

	for (i = 0; i < arr->len; i++) {
		p = (uint64_t)arr->buf[i] * val;
		ph = (uint32_t)(p >> 32);
		pl = (uint32_t)(p & 0xFFFFFFFFUL);
		u32arr_add_u32(ret->buf, ret->len, at + i, pl);
		u32arr_add_u32(ret->buf, ret->len, at + i + 1, ph);
	}
}

static void u32arr_mul_u32_i(struct mb_array_u32 *arr, size_t digits, uint32_t val, size_t at)
{
	size_t i;
	struct u64_split s;
	uint32_t u;

	foreach_reverse (arr->buf, digits, i, u) {
		s = split_u64((uint64_t)u * (uint64_t)val);
		arr->buf[i + at] = s.low;
		if (i + 1 < digits)
			u32arr_add_u32(arr->buf, arr->len, i + at + 1, s.high);
	}
}

static void u32arr_mul_u64(
		const struct mb_array_u32 *arr,
		uint64_t val,
		size_t at,
		struct mb_array_u32 *ret)
{
	struct u64_split s;

	s = split_u64(val);
	u32arr_mul_u32(arr, s.low, at, ret);
	u32arr_mul_u32(arr, s.high, at + 1, ret);
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
	unsigned digit_bits;
	int ch;	
	char u32_char_block[16];

	digits = mb_num_of_digits(str, radix);
	blk_size = 1 << (mb_log2(radix) - 1);
	u32_blocks = digits / blk_size;
	rem_digits = digits % blk_size;
	digit_bits = 32 / blk_size;

	ret->len = u32_blocks + !!rem_digits;
	ret->buf = malloc(ret->len * sizeof(ret->buf[0]));

	foreach_strn_br(str, u32_char_block, blk_size, i, u32_blocks) {
		u = 0;
		foreach_strn_lr(u32_char_block, ch, j, blk_size) {
			u <<= digit_bits;
			u |= mb_parse_digit(ch, radix);
		}
		ret->buf[i] = u;
	}
	u = 0;
	foreach_strn_l(str + u32_blocks * blk_size, ch, i, rem_digits) {
		u <<= digit_bits;
		u |= mb_parse_digit(ch, radix);
	}
	if (rem_digits)
		ret->buf[ret->len - 1] = u;
}

static void parse_generic(struct mb_array_u32 *ret, const char *str, unsigned radix)
{
	size_t digits;
	size_t i;
	size_t buf_len;
	unsigned digit;
	int ch;

	/* Radix that may or may not be a power of two. Life is hard. */

	digits = mb_num_of_digits(str, radix);
	buf_len = (size_t)ceil(digits * log2(radix) / 32.0);
	ret->buf = malloc(buf_len * sizeof(ret->buf[0]));
	ret->len = buf_len;
	for (ch = *str; *str; str++, ch = *str) {
		dbg_hexdump_u32(ret->buf, ret->len);
		u32arr_mul_u32_i(ret, buf_len, radix, 0);
		digit = mb_parse_digit(ch, radix);
		u32arr_add_u32(ret->buf, ret->len, 0, digit);
	}
	dbg_hexdump_u32(ret->buf, ret->len);
}

struct mb_array_u32 u32arr_from_str(const char *str, unsigned radix)
{
	struct mb_array_u32 ret;

	switch (radix) {
	case 2:
	case 4:
	case 8:
	case 16:
	case 32:
		parse_pow2(&ret, str, radix);
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

struct big_int * bi_new(uint32_t init)
{
	struct big_int *ret;

	ret = malloc(sizeof(*ret));
	ret->arr.buf = calloc(1, sizeof(ret->arr.buf[0]));
	ret->arr.len = 1;

	return ret;
}

struct big_int * bi_new_u64(uint64_t init)
{
	struct big_int *ret;
	struct u64_split val;

	val = split_u64(init);
	ret = malloc(sizeof(*ret));
	if (val.high) {
		ret->arr.buf = malloc(2 * sizeof(ret->arr.buf[0]));
		ret->arr.buf[0] = val.low;
		ret->arr.buf[1] = val.high;
		ret->arr.len = 2;
	} else {
		ret->arr.buf = malloc(sizeof(ret->arr.buf[0]));
		ret->arr.buf[0] = val.low;
		ret->arr.len = 1;
	}

	return ret;
}

struct big_int * bi_new_from_u32arr(const uint32_t *init, size_t length)
{
	struct big_int *ret;

	ret = malloc(sizeof(*ret));
	ret->arr.len = length;
	ret->arr.buf = calloc(length, sizeof(ret->arr.buf[0]));
	memcpy(ret->arr.buf, init, length * sizeof(ret->arr.buf[0]));

	return ret;
}

struct big_int * bi_new_from_str(const char *str, int radix)
{
	struct big_int *ret;

	ret = malloc(sizeof(*ret));
	ret->arr = u32arr_from_str(str, radix);

	if (ret->arr.buf == NULL) {
		free(ret);
		return NULL;
	}

	normalize(ret);
	return ret;
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

	if (i1->arr.len > i2->arr.len) {
		return 1;
	} else if (i1->arr.len < i2->arr.len) {
		return -1;
	}

	for (i = 0; i < i1->arr.len; i++) {
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

	ret = malloc(sizeof(*ret));
	ret->arr.len = i->arr.len;
	ret->arr.buf = malloc(ret->arr.len * sizeof(ret->arr.buf[0]));
	memcpy(ret->arr.buf, i->arr.buf, ret->arr.len * sizeof(ret->arr.buf[0]));

	return ret;
}

int bi_is_zero(const struct big_int *i)
{
	if (i->arr.len == 1) {
		if (i->arr.buf[0] == 0)
			return 1;
	}
	return 0;
}

/* --- Operator functions producing a new value --- */

struct big_int * bi_add_u64(const struct big_int *i, uint64_t val)
{
	struct big_int *ret;

	ret = bi_cpy_p(i, i->arr.len + 1);
	u32arr_add_u64(ret->arr.buf, ret->arr.len, 0, val);
	normalize(ret);

	return ret;
}

struct big_int * bi_sub_u64(const struct big_int *i, uint64_t val)
{
	struct big_int *ret;

	ret = bi_cpy_p(i, i->arr.len + 1);
	u32arr_sub_u64(i->arr.buf, i->arr.len, 0, val);
	normalize(ret);

	return ret;
}

struct big_int * bi_mul_u64(const struct big_int *i, uint64_t val)
{
	struct big_int *ret;

	if (bi_is_zero(i) || val == 0)
		return bi_cpy(&bi_zero);

	ret = bi_cpy_p(i, i->arr.len + 1);
	u32arr_mul_u64(&i->arr, val, 0, &ret->arr);
	normalize(ret);

	return ret;
}

struct big_int * bi_add(const struct big_int *i1, const struct big_int *i2)
{
	struct big_int *ret;
	size_t i;

	ret = bi_cpy_p(i1, MB_MAX(i1->arr.len, i2->arr.len) + 1);
	for (i = 0; i < i2->arr.len; i++) {
		u32arr_add_u32(ret->arr.buf, ret->arr.len, i, i2->arr.buf[i]);
	}
	normalize(ret);

	return ret;
}

struct big_int * bi_sub(const struct big_int *i1, const struct big_int *i2)
{
	struct big_int *ret;
	size_t i;

	ret = bi_cpy_p(i1, MB_MAX(i1->arr.len, i2->arr.len));
	for (i = 0; i < i2->arr.len; i++) {
		u32arr_sub_u32(ret->arr.buf, ret->arr.len, i, i2->arr.buf[i]);
	}
	normalize(ret);

	return ret;
}

struct big_int * bi_mul(const struct big_int *i1, const struct big_int *i2)
{
	struct big_int *ret;
	size_t i;

	if (bi_is_zero(i1) || bi_is_zero(i2))
		return bi_cpy(&bi_zero);

	ret = bi_new_p(i1->arr.len + i2->arr.len + 1);
	for (i = 0; i < i2->arr.len; i++) {
		u32arr_mul_u32(&i1->arr, i2->arr.buf[i], i, &ret->arr);
	}
	normalize(ret);

	return ret;
}

/* --- Operator functions modifying first argument --- */

void bi_add_u64_i(struct big_int *i, uint64_t val)
{
	ensure_capacity(&i->arr, i->arr.len + 1);
	u32arr_add_u32(i->arr.buf, i->arr.len, val, 0);
	normalize(i);
}

void bi_sub_u64_i(struct big_int *i, uint64_t val)
{
	u32arr_sub_u64(i->arr.buf, i->arr.len, 0, val);
	normalize(i);
}

void bi_mul_u64_i(struct big_int *i, uint64_t val)
{
	uint64_t p;
	size_t digits;
	struct u64_split s;

	digits = i->arr.len;
	s = split_u64(val);
	ensure_capacity(&i->arr, i->arr.len + 2);
	u32arr_mul_u32_i(&i->arr, digits, s.high, 1);
	u32arr_mul_u32_i(&i->arr, digits, s.low, 0);
	normalize(i);
}

void bi_add_i(struct big_int *ret, const struct big_int *num)
{
	size_t i;

	ensure_capacity(&ret->arr, ret->arr.len + 1);
	for (i = 0; i < num->arr.len; i++) {
		u32arr_add_u32(ret->arr.buf, ret->arr.len, i, num->arr.buf[i]);
	}
	normalize(ret);
}

void bi_sub_i(struct big_int *ret, const struct big_int *num)
{
	size_t i;

	for (i = 0; i < num->arr.len; i++) {
		u32arr_sub_u32(ret->arr.buf, ret->arr.len, i, num->arr.buf[i]);
	}
	normalize(ret);
}

void bi_mul_i(struct big_int *ret, const struct big_int *op)
{
	size_t i;
	size_t digits;
	uint32_t u;

	digits = ret->arr.len;
	ensure_capacity(&ret->arr, ret->arr.len + op->arr.len);
	foreach_reverse (op->arr.buf, op->arr.len, i, u) {
		u32arr_mul_u32_i(&ret->arr, digits, u, i);
	}
	normalize(ret);
}

char * bi_tostr(const struct big_int *ret)
{
	char *str;
	char *write_head;
	size_t i;
	uint32_t u;

	str = malloc(ret->arr.len * 8 + 1);
	write_head = str;
	foreach_reverse (ret->arr.buf, ret->arr.len, i, u) {
		sprintf(write_head, "%08X", u);
		write_head += 8;
	}

	return str;
}

