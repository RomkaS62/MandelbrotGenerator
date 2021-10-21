#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "big_int.h"
#include "mbarray.h"

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

	for (i = len - 1; i > 0; i--) {
		if (buf[i] != val)
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

/* Reads a 64-bit unsigned integer from uint32_t array starting at bit that is
 * of bits length. */
static uint64_t get_u64(uint32_t *buf, size_t length, size_t bit, size_t bits)
{
	size_t u32_shift;	/* Whole uint32_t's shifted. */
	size_t bit_shift;	/* Bits remaining to shift after shifting whole
				   uint32_t's */
	uint64_t ret;

	/* Someone wants to read bits outside the buffer. */
	if ((bit + bits) / 32 > length) {
		return INT_MIN;
	}

	u32_shift = bit / 32;
	bit_shift = bit % 32;

	/* If bit_shift is 0, life is easy. */
	if (!bit_shift) {
		ret = buf[u32_shift] | buf[u32_shift + 1];
	} else {
		/* Discard trailing bit_shift bits form first uint32_t and read
		 * extra bit_shift bits from third uint32_t. Imgagine that
		 * bit_shift is 5. Situation looks like the following:
		 *              Discarded
		 *      buf[0]  00000           000000000000000000000000000
		 *
		 *      buf[1]                  00000000000000000000000000000000
		 *
		 *      buf[2]  00000           000000000000000000000000000
		 *                              Discarded
		 *
		 * Since these are all unsigned integers, no undefined
		 * behaviour should ensue from bit shifts.
		 * */
		ret = ((uint64_t)buf[u32_shift] << bit_shift) << 32;
		ret |= (uint64_t)buf[u32_shift + 1] << bit_shift;
		ret |= (uint64_t)buf[u32_shift + 2] >> (32 - bit_shift);
	}
	ret >>= (64 - bits);

	return ret;
}

/* Adds val to buf in place at position 'at' while checking for overflow.
 * Returns any overflow the entire operation produced. */
static uint32_t u32arr_add_u32(uint32_t *buf, size_t len, size_t at, uint32_t val)
{
	uint64_t sum;
	uint32_t overflow;

	if (!val)
		return 0;

	if (at >= len) {
		fprintf(stderr, "Attempted add at %zu in array of size %zu\n", at, len);
		exit(1);
	}

	sum = buf[at] + val;
	overflow = sum >> 32;
	buf[at] = sum;
	if (overflow) {
		if (at >= len) {
			return overflow;
		}
		return u32arr_add_u32(buf, len, at + 1, overflow);
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

static u32arr_sub_u64(uint32_t *buf, size_t len, size_t at, uint64_t val)
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

	ret = malloc(sizeof(*ret));
	if (init & ~0xFFFFFFFFUL) {
		ret->arr.buf = malloc(2 * sizeof(ret->arr.buf[0]));
		ret->arr.buf[0] = (uint32_t)(init & 0xFFFFFFFFUL);
		ret->arr.buf[1] = (uint32_t)(init >> 32);
		ret->arr.len = 2;
	} else {
		ret->arr.buf = malloc(sizeof(ret->arr.buf[0]));
		ret->arr.buf[0] = (uint32_t)init;
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
	memcpy(ret->arr.buf, init, length);

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

	ret = bi_new_p(i1->arr.len + i2->arr.len + 1);
	for (i = 0; i < i2->arr.len; i++) {
		u32arr_mul_u32(&i1->arr, i2->arr.buf[i], i, &ret->arr);
	}

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
