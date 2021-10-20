#ifndef MANDELBROT_CDOUBLE_H
#define MANDELBROT_CDOUBLE_H

#ifdef __cplusplus
extern "C" {
#endif

struct cdouble {
	double real;
	double img;
};

static inline struct cdouble cd_mul(struct cdouble d1, struct cdouble d2)
{
	struct cdouble ret;

	ret.real = d1.real * d2.real - d1.img * d2.img;
	ret.img = d1.real * d2.img + d1.img * d2.real;

	return ret;
}

static inline struct cdouble cd_add(struct cdouble d1, struct cdouble d2)
{
	struct cdouble ret;

	ret.real = d1.real + d2.real;
	ret.img = d1.img + d2.img;

	return ret;
}

/*
 * (a + bi) * (a + bi)
 *      = (a * a + a * bi) + (abi + b^2 * i^2)
 *      = a^2 + abi + abi + b^2 * (-1)
 *      = a^2 + 2abi - b^2
 *      = (a^2 - b^2) + 2abi
 */
static inline struct cdouble cd_sqr(struct cdouble d)
{
	struct cdouble ret;

	ret.real = d.real * d.real - d.img * d.img;
	ret.img = 2 * d.real * d.img;

	return ret;
}

static inline double cd_magnitude_sqr(struct cdouble d)
{
	return d.real * d.real + d.img * d.img;
}

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_CDOUBLE_H */
