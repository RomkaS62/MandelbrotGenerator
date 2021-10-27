#ifndef MANDELBROT_BIG_INT_H
#define MANDELBROT_BIG_INT_H

#include <stdlib.h>
#include <stdint.h>
#include "mbarray.h"

#ifdef __cplusplus
extern "C" {
#endif

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
struct big_int * bi_new(size_t size, uint32_t init);
struct big_int * bi_new_from_u32arr(size_t size, const uint32_t *init, size_t length);
struct big_int * bi_new_from_str(size_t size, const char *str, int radix);
void bi_delete(struct big_int *i);

int bi_cmp_u64(const struct big_int *a, uint64_t val);
int bi_cmp(const struct big_int *i1, const struct big_int *i2);
struct big_int * bi_cpy(const struct big_int *i);

int bi_is_zero(const struct big_int *i);
void bi_set_zero(struct big_int *i);

/* --- Operator functions producing a new value --- */

struct big_int * bi_add_u64(const struct big_int *i, uint64_t val);
struct big_int * bi_sub_u64(const struct big_int *i, uint64_t val);
struct big_int * bi_mul_u64(const struct big_int *i, uint64_t val);

struct big_int * bi_add(const struct big_int *i1, const struct big_int *i2);
struct big_int * bi_sub(const struct big_int *i1, const struct big_int *i2);
struct big_int * bi_mul(const struct big_int *i1, const struct big_int *i2);

/* --- Operator functions modifiying first argument. --- */

void bi_add_u64_i(struct big_int *i, uint64_t val);
void bi_sub_u64_i(struct big_int *i, uint64_t val);
void bi_mul_u64_i(struct big_int *i, uint64_t val);

void bi_add_i(struct big_int *ret, const struct big_int *i);
void bi_sub_i(struct big_int *ret, const struct big_int *i);
void bi_mul_i(struct big_int *ret, const struct big_int *i);

char * bi_tostr(const struct big_int *ret, unsigned radix);

#define MB_MAX(_a, _b) (((_a) > (_b)) ? (_a) : (_b))

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_BIG_INT_H */
