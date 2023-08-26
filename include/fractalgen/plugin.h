#ifndef MANDELBROT_FRACTAL_ITERATOR_H
#define MANDELBROT_FRACTAL_ITERATOR_H

#include <gramas/ptr_array.h>

#include "bmp.h"
#include "fractalgen/param_set.h"

#ifdef __cplusplus
extern "C" {
#endif

struct frg_iteration_request_s {
	unsigned short rows;
	unsigned short cols;
	unsigned iterations;
	double from_x;
	double from_y;
	double step;
};

typedef void (*iterate_fn)(
	const struct frg_iteration_request_s *spec,
	unsigned * restrict iterations,
	const struct frg_param_set_s *params);

struct frg_iterate_func_s {
	char *name;
	iterate_fn iterate;
};

typedef void (*render_fn)(
	const struct frg_iteration_request_s *spec,
	unsigned * restrict iterations,
	struct pixel *img,
	const struct frg_param_set_s *params);

struct frg_render_func_s {
	char *name;
	render_fn render;
};

struct frg_render_fn_repo_s {
	struct ptr_array iterate_funcs;
	struct ptr_array render_funcs;
};

typedef int (*frg_module_init_fn)(struct frg_render_fn_repo_s *itr);

int frg_load_modules(
		const char *iterator_glob_pattern,
		struct frg_render_fn_repo_s *iterators);

void frg_fn_repo_init(struct frg_render_fn_repo_s *itr);
void frg_fn_repo_destroy(struct frg_render_fn_repo_s *itr);
iterate_fn frg_fn_repo_get_iterator(struct frg_render_fn_repo_s *itr, const char *name);
render_fn frg_fn_repo_get_renderer(struct frg_render_fn_repo_s *itr, const char *name);

void frg_fn_repo_register_iterator(
		struct frg_render_fn_repo_s *itr,
		const char *name,
		iterate_fn fn);

void frg_fn_repo_register_renderer(
		struct frg_render_fn_repo_s *itr,
		const char *name,
		render_fn fn);

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_FRACTAL_ITERATOR_H */
