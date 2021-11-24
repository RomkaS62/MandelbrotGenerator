#ifndef MANDELBROT_FIXED_16_48_H
#define MANDELBROT_FIXED_16_48_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

static inline uint64_t bits(const int count)
{
	return ~(~0ULL << count);
}

static inline uint64_t u64range(const uint64_t num, const int offset, const int count)
{
	return (num >> offset) & bits(count);
}

struct u128 {
	uint64_t low;
	uint64_t high;
};

/*
 *             ah   al
 *             bh   bl
 *           ---------
 *           ahbl albl
 *           albh
 *      ahbh
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

static inline uint64_t u64fmul(uint64_t a, uint64_t b, int fracbits)
{
	struct u128 prod;

	prod = u64mul(a, b);
	prod.low += 1UL << (fracbits - 1);
	prod.high += prod.low < (1UL << (fracbits - 1));

	return prod.high << (64 - fracbits) | (prod.low >> fracbits);
}

void draw_lines_u64f4(void *data);

#if defined(__cplusplus)
}
#endif

#endif /* MANDELBROT_FIXED_16_48_H */
