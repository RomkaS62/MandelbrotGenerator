#ifndef MANDELBROT_BIG_FIXED
#define MANDELBROT_BIG_FIXED

#include <stdlib.h>
#include "big_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed point unsigned number. */
struct big_fixed {
	struct big_int i;	/* Underlying integer. */
	size_t binp;		/* Binary point. Number of integral uint32_t's.
				   Always less than i->arr.len. */
};

static inline void bf_init(struct big_fixed *f, size_t integral, size_t frac)
{
	bi_init(&f->i, integral + frac, 0);
}

static inline void bf_init_u64(struct big_fixed *f, size_t integral, size_t frac, uint64_t init)
{
	f->binp = integral;
	bi_init_u64(&f->i, init, (int32_t)frac);
}

void bf_add_i(struct big_fixed *f1, const struct big_fixed *f2);
void bf_sub_i(struct big_fixed *f1, const struct big_fixed *f2);
void bf_mul_i(struct big_fixed *f1, const struct big_fixed *f2);

int bf_cmp(const struct big_fixed *f1, const struct big_fixed *f2);

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_BIG_FIXED */
