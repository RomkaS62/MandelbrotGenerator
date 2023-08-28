#ifndef MANDELBROT_FIXED_16_48_H
#define MANDELBROT_FIXED_16_48_H

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#if defined(__cplusplus)
extern "C" {
#endif

static inline uint64_t bits(const int count)
{
	return ~(~0ULL << count);
}

static inline uint64_t u64inv(const uint64_t x)
{
	return ~x + 1;
}

static inline uint64_t u64range(const uint64_t num, const int offset, const int count)
{
	return (num >> offset) & bits(count);
}

static inline int u64f_intbits(const int precision)
{
	return 64 - precision;
}

static inline uint64_t u64ffromi64(const int64_t num, const int precision)
{
	if (num >= 0)
		return (uint64_t)(num << precision);
	else
		return u64inv(labs(num)) << precision;
}

static inline int fpneg(const uint64_t a)
{
	return (a & (1ULL << 63)) != 0;
}

/* Returns maximum signed integer value represantable with given precision. */
static inline int64_t u64f_int64_max(const int precision)
{
	return INT64_MAX >> precision;
}

/* Returns minimum signed integer value representable with given precision. */
static inline int64_t u64f_int64_min(const int precision)
{
	return INT64_MIN >> precision;
}

static inline uint64_t fpabs(const uint64_t a)
{
	if (fpneg(a)) {
		return u64inv(a);
	} else {
		return a;
	}
}

static inline int64_t u64ftoi64(const uint64_t num, const int precision)
{
	if (fpneg(num))
		return -(int64_t)(fpabs(num) >> precision);
	else
		return (int64_t)num >> precision;
}

struct u128 {
	uint64_t low;
	uint64_t high;
};

static inline struct u128 u128_shr(const struct u128 num, const int shift)
{
	struct u128 ret;

	ret.high = num.high >> shift;
	ret.low = num.low >> shift;
	ret.low |= (num.high & ~(~0UL << (64 - shift))) << (64 - shift);

	return ret;
}

/*
 *             ah   al
 *             bh   bl
 *           ---------
 *           ahbl albl
 *      albh albh
 *  ------------------
 *  rhh  rhl  rlh  rll
 * */
static inline struct u128 u64mul(const uint64_t a, const uint64_t b)
{
	struct u128 ret = { 0, 0 };
	uint64_t al;
	uint64_t ah;
	uint64_t bl;
	uint64_t bh;

	al = u64range(a, 0, 32);
	ah = u64range(a, 32, 32);
	bl = u64range(b, 0, 32);
	bh = u64range(b, 32, 32);

	ret.low = al * bl;

	ret.low += (ah * bl) << 32;
	ret.high += ret.low < (ah * bl) << 32;
	ret.high += u64range(ah * bl, 32, 32);

	ret.low += (al * bh) << 32;
	ret.high += ret.low < (al * bh) << 32;
	ret.high += u64range(al * bh, 32, 32);

	ret.high += ah * bh;

	return ret;
}

/*
 *          a . b  c
 *          d . e  f
 *         ---------
 *          fa fb fc
 *       ea fb fc
 *    da fb fc
 *   ---------------
 *  |   .     |
 * */
static inline uint64_t u64fmul(const uint64_t a, const uint64_t b, const int precision)
{
	uint64_t ret;
	struct u128 prod;

	prod = u64mul(fpabs(a), fpabs(b));
	prod.low += 1UL << (precision - 1);
	prod.high += prod.low < (1UL << (precision - 1));

	ret = prod.high << (64 - precision) | (prod.low >> precision);
	if (fpneg(a) ^ fpneg(b)) {
		ret = u64inv(ret);
	}

	return ret;
}

static inline uint64_t fsquare(const uint64_t a, const int precision)
{
	return u64fmul(fpabs(a), fpabs(a), precision);
}

void u64f_print(const uint64_t num, const int precision);
void draw_lines_u64f4(void *data);

#if defined(__cplusplus)
}
#endif

#endif /* MANDELBROT_FIXED_16_48_H */
