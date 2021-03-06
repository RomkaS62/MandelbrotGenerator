#ifndef MANDELBROT_BIG_INT_H
#define MANDELBROT_BIG_INT_H

#include <stdlib.h>
#include <stdint.h>
#include "mbarray.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MB_MAX(_a, _b) (((_a) > (_b)) ? (_a) : (_b))

/* Represents a little-endian unsigned big integer. No such thing as overflow
 * when adding and multiplying. Underflow results in 0.
 *
 * All arithmetic operations either return an integer the same size as the
 * bigger argument or do not change the size of the argument if an operation is
 * done in-place.
 * */
struct big_int {
	struct mb_array_u32 arr;	/* Little-endian array of uin32_t's. */
};

/* The folowing functions all have a size argument. One unit of size is
 * 32-bits. The size argument specifies the minimum size of the returned value.
 * If value represented by given initialisation values is greater than what
 * size would allow, it is adjusted to however much is needed to represent the
 * value without overflow. */
void bi_init_s(struct big_int *ret, size_t size, uint32_t init, size_t shift);
void bi_init_u64_s(struct big_int *ret, size_t size, uint64_t init, size_t shift);
void bi_init_from_u32arr(struct big_int *ret, size_t size, const uint32_t *init, size_t length);
void bi_init_from_str(struct big_int *ret, size_t size, const char *str, int radix);
void bi_delete(struct big_int *i);

#define bi_init(__ret, __size, __init) bi_init_s(__ret, __size, __init, 0)
#define bi_init_u64(__ret, __size, __init) bi_init_u64_s(__ret, __size, __init, 0)

static inline struct big_int * bi_new_s(size_t size, uint32_t init, size_t shift)
{
	struct big_int *ret;

	ret = malloc(sizeof(*ret));
	bi_init_s(ret, size, init, shift);

	return ret;
}

static inline struct big_int * bi_new_u64(size_t size, uint64_t init, size_t shift)
{
	struct big_int *ret;

	ret = malloc(sizeof(*ret));
	bi_init_u64_s(ret, size, init, shift);

	return ret;
}

static inline struct big_int * bi_new_from_u32arr(size_t size, const uint32_t *init, size_t length)
{
	struct big_int *ret;

	ret = malloc(sizeof(*ret));
	bi_init_from_u32arr(ret, size, init, length);

	return ret;
}

static inline struct big_int * bi_new_from_str(size_t size, const char *str, int radix)
{
	struct big_int *ret;

	ret = malloc(sizeof(*ret));
	bi_init_from_str(ret, size, str, radix);
	if (ret->arr.buf == NULL)
		return NULL;

	return ret;
}

int bi_cmp_u64(const struct big_int *a, uint64_t val);
int bi_cmp(const struct big_int *i1, const struct big_int *i2);
struct big_int * bi_cpy(const struct big_int *i);
struct big_int * bi_cpy_p(const struct big_int *i, size_t size);

int bi_is_zero(const struct big_int *i);
void bi_set_zero(struct big_int *i);

/* --- Operator functions modifiying first argument. --- */

/* In place operations with a shifted value. Positive shift means a left
 * shifted second operand. Otherwise --- right shifted. */

void bi_add_u64_is(struct big_int *i, uint64_t val, int32_t shift);
void bi_sub_u64_is(struct big_int *i, uint64_t val, int32_t shift);
void bi_mul_u64_is(struct big_int *i, uint64_t val, int32_t shift);

void bi_add_is(struct big_int *ret, const struct big_int *i, int32_t shift);
void bi_sub_is(struct big_int *ret, const struct big_int *i, int32_t shift);
void bi_mul_is(struct big_int *ret, const struct big_int *i, int32_t shift);

#define bi_add_u64_i(__i, __val) bi_add_u64_is(__i, __val, 0)
#define bi_sub_u64_i(__i, __val) bi_sub_u64_is(__i, __val, 0)
#define bi_mul_u64_i(__i, __val) bi_mul_u64_is(__i, __val, 0)

#define bi_add_i(__ret, __i) bi_add_is(__ret, __i, 0)
#define bi_sub_i(__ret, __i) bi_sub_is(__ret, __i, 0)
#define bi_mul_i(__ret, __i) bi_mul_is(__ret, __i, 0)

/* --- Operator functions producing a new value --- */

static inline struct big_int * bi_add_u64(const struct big_int *i, uint64_t val)
{
	struct big_int *ret;

	ret = bi_cpy_p(i, MB_MAX(i->arr.len, 2));
	bi_add_u64_i(ret, val);

	return ret;
}

static inline struct big_int * bi_sub_u64(const struct big_int *i, uint64_t val)
{
	struct big_int *ret;

	ret = bi_cpy_p(i, MB_MAX(i->arr.len, 2));
	bi_sub_u64_i(ret, val);

	return ret;
}

static inline struct big_int * bi_mul_u64(const struct big_int *i, uint64_t val)
{
	struct big_int *ret;

	ret = bi_cpy_p(i, MB_MAX(i->arr.len, 2));
	bi_mul_u64_i(ret, val);

	return ret;
}


static inline struct big_int * bi_add(const struct big_int *i1, const struct big_int *i2)
{
	struct big_int *ret;

	ret = bi_cpy_p(i1, MB_MAX(i1->arr.len, 2));
	bi_add_i(ret, i2);

	return ret;
}

static inline struct big_int * bi_sub(const struct big_int *i1, const struct big_int *i2)
{
	struct big_int *ret;

	ret = bi_cpy_p(i1, MB_MAX(i1->arr.len, 2));
	bi_sub_i(ret, i2);

	return ret;
}

static inline struct big_int * bi_mul(const struct big_int *i1, const struct big_int *i2)
{
	struct big_int *ret;

	ret = bi_cpy_p(i1, MB_MAX(i1->arr.len, 2));
	bi_mul_i(ret, i2);

	return ret;
}

char * bi_tostr(const struct big_int *ret, unsigned radix);

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_BIG_INT_H */
