#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "fixed.h"
#include "mbthreading.h"
#include "global.h"
#include "hue.h"

static uint64_t fsquare(const uint64_t a, const int fracbits)
{
	return u64fmul(a, a, fracbits);
}

static uint64_t fpabs(const uint64_t a)
{
	int is_negative = (a & (1ULL << 63)) != 0;

	if (is_negative) {
		return (uint64_t)-((int64_t)a);
	} else {
		return a;
	}
}

static int abs_f_lt_u64(const uint64_t a, const uint64_t b, const int fracbits)
{
	return fpabs(a) < (b << fracbits);
}

static void fcmplx_iterate(
		const uint64_t *restrict real,
		const uint64_t *restrict img,
		uint64_t *restrict real_ret,
		uint64_t *restrict img_ret,
		const int fracbits,
		unsigned *restrict iterations,
		const size_t length,
		const unsigned long attempts)
{
	unsigned long i;
	size_t j;
	uint64_t tmpr;
	uint64_t tmpi;
	uint64_t sqrr;
	uint64_t sqri;
	uint64_t sqrsum;
	int within_bounds;

	memcpy(real_ret, real, length * sizeof(real[0]));
	memcpy(img_ret, img, length * sizeof(img[0]));

	for (i = 0; i < attempts; i++) {
		for (j = 0; j < length; j++) {
			tmpr = real_ret[j];
			tmpi = img_ret[j];
			within_bounds = abs_f_lt_u64(tmpr, 2, fracbits)
					&& abs_f_lt_u64(tmpi, 2, fracbits);
			iterations += tmpr < (2ULL << fracbits) || tmpi < (2ULL << fracbits);
			sqrr = fsquare(tmpr, fracbits);
			sqri = fsquare(tmpi, fracbits);
			sqrsum = sqrr + sqri;
			real_ret[j] = sqrsum * within_bounds + tmpr * !within_bounds;
			img_ret[j] = (u64fmul(tmpr, tmpi, fracbits) << 1) * within_bounds
					+ tmpi * !within_bounds;
		}
	}
}

static void fcmplx_linspace_x(uint64_t *restrict arr, uint64_t from, uint64_t step,
		size_t rows, size_t cols)
{
	size_t row;
	size_t col;

	for (col = 0; col < cols; col++)
		arr[col] = from + step * col;

	for (row = 1; row < rows; row++)
		memcpy(arr + row * cols, arr, cols * sizeof(arr[0]));
}

static void fcmplx_linspace_y(uint64_t *restrict arr, uint64_t from, uint64_t step,
		size_t rows, size_t cols)
{
	size_t row;
	size_t col;
	uint64_t val;

	for (row = 0; row < rows; row++) {
		val = from + step * row;
		for (col = 0; col < cols; col++) {
			arr[row * cols + col] = val;
		}
	}
}

static uint64_t dtou64(const double val, const int fracbits)
{
	uint64_t ret;

	ret = (uint64_t)(fabs(val) * (1 << fracbits));
	ret = (~ret + 1) * (val < 0.0) + ret * (val >= 0.0);

	return ret;
}

#define ALLOC_ARRAY(_arr, _len) malloc((_len) * sizeof((_arr)[0]))

void draw_lines_u64f4(void *data)
{
	struct pallette_s pallette;

	struct draw_lines_data *ld = data;
	size_t rows = ld->ln_to - ld->ln_from;
	size_t cols = ld->img->width;
	size_t length = rows * cols;

	uint64_t from_x = dtou64(ld->from_x, fixed_precision);
	uint64_t from_y = dtou64(ld->from_y, fixed_precision);
	uint64_t step = dtou64(ld->step, fixed_precision);

	uint64_t *real = ALLOC_ARRAY(real, length);
	uint64_t *img = ALLOC_ARRAY(img, length);
	uint64_t *real_ret = ALLOC_ARRAY(real_ret, length);
	uint64_t *img_ret = ALLOC_ARRAY(img_ret, length);
	unsigned *iterations = ALLOC_ARRAY(iterations, length);

	pallette_init(&pallette, pallette_length);
	fcmplx_linspace_x(real, from_x, step, rows, cols);
	fcmplx_linspace_y(img, from_y, step, rows, cols);

	fcmplx_iterate(real, img, real_ret, img_ret, fixed_precision, iterations, length, attempts);
	draw_pixels(ld->ln_from, ld->ln_to, iterations, ld->img, &pallette);

	free(real);
	free(img);
	free(real_ret);
	free(img_ret);
	free(iterations);
	pallette_free(&pallette);
}
