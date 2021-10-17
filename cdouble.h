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
	return (struct cdouble) { d1.real * d2.real - d1.img * d2.img,
		d1.real * d2.img + d1.img * d2.real
	};
}

static inline struct cdouble cd_add(struct cdouble d1, struct cdouble d2)
{
	return (struct cdouble) {d1.real + d2.real, d1.img + d2.img};
}

static inline struct cdouble cd_sqr(struct cdouble d)
{
	return cd_mul(d, d);
}

static inline double cd_magnitude_sqr(struct cdouble d)
{
	return d.real * d.real + d.img * d.img;
}

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_CDOUBLE_H */
