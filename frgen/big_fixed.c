#include <stdlib.h>
#include "mbitr.h"
#include "big_int.h"
#include "big_fixed.h"

/* Returns integral uint32_t at given offset from binary point. The bigger the
 * offset the more significant the digit. */
static inline uint32_t get_iu32(const struct big_fixed *f, size_t ofst)
{
	if (ofst > f->binp)
		return 0;

	return f->i.arr.buf[f->i.arr.len  - f->binp - 1 + ofst];
}

static inline int32_t sub_ss(size_t s1, size_t s2)
{
	if (s1 > s2)
		return (int32_t)(s1 - s2);
	else
		return -((int32_t)(s2 - s1));
}

static inline uint32_t get_fu32(const struct big_fixed *f, size_t ofst)
{
	size_t idx;
	size_t first_fu32;

	first_fu32 = f->i.arr.len - f->binp - 1;
	if (first_fu32 < ofst)
		return 0;
	idx = first_fu32 - ofst;

	return f->i.arr.buf[idx];
}

static inline size_t fractional_digits(const struct big_fixed *f)
{
	return f->i.arr.len - f->binp;
}

/* Addition is fairly simple.
 *
 *	1. Shift second integer so that their binary points align.
 *	2. add.
 *
 * Well. This is not what I'll actually do. Instead, I shall add second integer
 * to first at an offset. This does the same thing and does not require
 * shifting an entire big int.
 * */
void bf_add_i(struct big_fixed *f1, const struct big_fixed *f2)
{
	bi_add_is(&f1->i, &f2->i, sub_ss(f2->binp, f1->binp));
}

/* Subtracting is pretty much the exact same as addition just with opposite
 * operation. */
void bf_sub_i(struct big_fixed *f1, const struct big_fixed *f2)
{
	bi_add_is(&f1->i, &f2->i, sub_ss(f2->binp, f1->binp));
}

void bf_mul_i(struct big_fixed *f1, const struct big_fixed *f2)
{
	bi_mul_is(&f1->i, &f2->i, sub_ss(f2->binp, f1->binp));
}

int bf_cmp(const struct big_fixed *f1, const struct big_fixed *f2)
{
	size_t ids;	/* Max integral digits. */
	size_t fds;	/* Max fractional digits. */
	size_t i;
	uint32_t val1;
	uint32_t val2;

	ids = MB_MAX(f1->binp, f2->binp);
	fds = MB_MAX(fractional_digits(f1), fractional_digits(f2));

	foreach_reverse_idx (i, ids) {
		val1 = get_iu32(f1, i);
		val2 = get_iu32(f2, i);
		if (val1 > val2)
			return 1;
		else if (val1 < val2)
			return -1;
	}

	foreach_reverse_idx (i, fds) {
		val1 = get_fu32(f1, i);
		val2 = get_fu32(f2, i);
		if (val1 > val2)
			return 1;
		else if (val1 < val2)
			return -1;
	}

	return 0;
}
