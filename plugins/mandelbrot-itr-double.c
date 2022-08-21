#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mandelbrot-itr.h"
#include "fractalgen/plugin.h"

static void matrix_x_fs(double * restrict ret, size_t rows, size_t cols, double from, double step)
{
	size_t i;

	for (i = 0; i < cols; i++)
		ret[i] = step * i + from;

	for (i = 1; i < rows; i++)
		memcpy(ret + i * cols, ret, cols * sizeof(*ret));
}

static void d_arr_set(double * restrict arr, const size_t length, const double val)
{
	size_t i;

	for (i = 0; i < length; i++)
		arr[i] = val;
}

static void matrix_y_fs(double * restrict ret, size_t rows, size_t cols, double from, double step)
{
	double val;
	size_t y;
	size_t row_offset;

	for (y = 0; y < rows; y++) {
		val = y * step + from;
		row_offset = y * cols;
		d_arr_set(ret + row_offset, cols, val);
	}
}

static const double max_distance = (double)4.0;

static double square(const double val)
{
	return val * val;
}

static inline double magnitude_sqr(const double x, const double y)
{
	return square(x) + square(y);
}

static inline int distance_check(const double x, const double y)
{
	return magnitude_sqr(x, y) < max_distance && !isnan(x) && !isnan(y) && !isnan(x * y);
}

#define CHUNK_SIZE (128 / sizeof(double))
static void iterate_chunk(
		const double *restrict real,
		const double *restrict img,
		double *restrict real_ret,
		double *restrict img_ret,
		unsigned *restrict iterations,
		unsigned iteration_count)
{
	size_t i;
	size_t j;
	double tmpr;
	double tmpi;
	int within_bounds;

	memcpy(real_ret, real, sizeof(real[0]) * CHUNK_SIZE);
	memcpy(img_ret, img, sizeof(img[0]) * CHUNK_SIZE);

	for (i = 0; i < iteration_count; i++) {
		for (j = 0; j < CHUNK_SIZE; j++) {
			tmpr = real_ret[j];
			tmpi = img_ret[j];
			within_bounds = distance_check(tmpr, tmpi);
			iterations[j] += 1 * within_bounds;

			real_ret[j] = (square(tmpr) - square(tmpi) + real[j]) * within_bounds
				+ tmpr * !within_bounds;

			img_ret[j] = (2.0f * tmpr * tmpi + img[j]) * within_bounds
				+ tmpi * !within_bounds;
		}
	}
}

static void iterate(
		const double *restrict real,
		const double *restrict img,
		double *restrict real_ret,
		double *restrict img_ret,
		unsigned *restrict iterations,
		unsigned iteration_count,
		const size_t length)
{
	size_t i;
	size_t j;
	double tmpr;
	double tmpi;
	int within_bounds;

	memcpy(real_ret, real, sizeof(real[0]) * length);
	memcpy(img_ret, img, sizeof(img[0]) * length);

	for (i = 0; i < iteration_count; i++) {
		for (j = 0; j < length; j++) {
			tmpr = real_ret[j];
			tmpi = img_ret[j];
			within_bounds = distance_check(tmpr, tmpi);
			iterations[j] += 1 * within_bounds;

			real_ret[j] = (square(tmpr) - square(tmpi) + real[j]) * within_bounds
				+ tmpr * !within_bounds;

			img_ret[j] = (2.0f * tmpr * tmpi + img[j]) * within_bounds
				+ tmpi * !within_bounds;
		}
	}
}

static void iterate_mandelbrot_d(
	const struct iteration_spec_s *spec,
	unsigned * restrict iterations,
	const struct param_set_s *params)
{
	double *buf;
	double *real;
	double *img;
	double *real_ret;
	double *img_ret;
	size_t buf_len;
	size_t i;

	buf_len = spec->rows * spec->cols;

	buf = malloc(buf_len * 4 * sizeof(buf[0]));
	real = buf;
	img = real + buf_len;
	real_ret = img + buf_len;
	img_ret = real_ret + buf_len;

	matrix_x_fs(real, spec->rows, spec->cols, spec->from_x, spec->step);
	matrix_y_fs(img, spec->rows, spec->cols, spec->from_y, -spec->step);

	for (i = 0; i + CHUNK_SIZE < buf_len; i += CHUNK_SIZE) {
		iterate_chunk(real + i, img + i, real_ret + i, img_ret + i,
				iterations + i, spec->iterations);
	}

	iterate(real + i, img + i, real_ret + i, img_ret + i,
			iterations + i, spec->iterations, buf_len - i);

	free(buf);
}

static const struct iterator_func_s itr_funcs[] = {
	{ "mandelbrot-double", iterate_mandelbrot_d }
};

#define ITR_FUNC_COUNT (sizeof(itr_funcs) / sizeof(itr_funcs[0]))

const struct fractal_iterator_s iterators = {
	ITR_FUNC_COUNT, itr_funcs, 0, NULL
};