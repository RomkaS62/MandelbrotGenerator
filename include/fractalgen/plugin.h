#ifndef MANDELBROT_FRACTAL_ITERATOR_H
#define MANDELBROT_FRACTAL_ITERATOR_H

#include "bmp.h"

#ifdef __cplusplus
extern "C" {
#endif

struct iteration_spec_s {
	unsigned short rows;
	unsigned short cols;
	unsigned iterations;
	double from_x;
	double from_y;
	double step;
};

typedef void (*iterate_fn)(
	const struct iteration_spec_s *spec,
	unsigned * restrict iterations);

struct iterator_func_s {
	const char *name;
	iterate_fn iterate;
};

typedef void (*render_fn)(
	const struct iteration_spec_s *spec,
	unsigned * restrict iterations,
	struct pixel *img);

struct render_func_s {
	const char *name;
	render_fn render;
};

struct fractal_iterator_s {
	const int iterate_func_count;
	const struct iterator_func_s *iterate_funcs;
	const int render_func_count;
	const struct render_func_s *render_funcs;
};

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_FRACTAL_ITERATOR_H */
