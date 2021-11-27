#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "fixed.h"
#include "mbthreading.h"
#include "global.h"
#include "hue.h"

static uint64_t u64_pow10(unsigned pow)
{
	uint64_t ret = 10;

	if (pow == 0)
		return 0;

	while (pow --> 0)
		ret *= 10;

	return ret;
}

void u64f_print(const uint64_t num, const int precision)
{
	static const int dec_digits = 5;

	struct u128 fracpart_128;
	uint64_t absnum = fpabs(num);
	uint64_t intpart = (uint64_t)labs(absnum >> precision);
	uint64_t fracpart = absnum & ~(~0UL << precision);
	int sign = fpneg(num);

	fracpart_128 = u64mul(fracpart, u64_pow10(dec_digits));
	fracpart_128 = u128_shr(fracpart_128, precision);

	if (sign) {
		printf("-%"PRIu64, intpart);
	} else {
		printf("%"PRIu64, intpart);
	}

	printf(".%0*"PRIu64, dec_digits, fracpart_128.low);
}

static void fcmplx_print(const uint64_t real, const uint64_t img, const int precision)
{
	u64f_print(real, precision);
	fputs(" + i", stdout);
	u64f_print(img, precision);
}

static void newline(void)
{
	fputc('\n', stdout);
}

static int distance_check(const uint64_t real, const uint64_t img, const int precision)
{
	int ret;

	ret = (fsquare(real, precision) + fsquare(img, precision))
		< (4UL << precision);

	return ret;
}

static void fcmplx_iterate(
		const uint64_t *restrict real,
		const uint64_t *restrict img,
		uint64_t *restrict real_ret,
		uint64_t *restrict img_ret,
		const int precision,
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
	uint64_t sqrdiff;
	int within_bounds;

	memcpy(real_ret, real, length * sizeof(real[0]));
	memcpy(img_ret, img, length * sizeof(img[0]));

	for (i = 0; i < attempts; i++) {
		for (j = 0; j < length; j++) {
			tmpr = real_ret[j];
			tmpi = img_ret[j];
			if (!distance_check(tmpr, tmpi, precision))
				continue;
			iterations[j] += 1;
			sqrr = fsquare(tmpr, precision);
			sqri = fsquare(tmpi, precision);
			sqrdiff = sqrr - sqri;
			real_ret[j] = sqrdiff + real[j];
			img_ret[j] = (u64fmul(tmpr, tmpi, precision) << 1) + img[j];
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

static uint64_t dtou64(const double val, const int precision)
{
	uint64_t ret;

	ret = (uint64_t)(fabs(val) * (1UL << precision));
	if (val < 0.0)
		ret = u64inv(ret);


	return ret;
}

#define ALLOC_ARRAY(_arr, _len) malloc((_len) * sizeof((_arr)[0]))
#define CALLOC_ARRAY(_arr, _len) calloc((_len), sizeof((_arr)[0]))

void draw_lines_u64f4(void *data)
{
	struct pallette_s pallette;

	struct draw_lines_data *ld = data;
	size_t rows = ld->ln_to - ld->ln_from;
	size_t cols = ld->img->width;
	size_t length = rows * cols;

	uint64_t from_x = dtou64(ld->from_x, fixed_precision);
	uint64_t from_y = dtou64(ld->from_y + ld->step * ld->ln_from, fixed_precision);
	uint64_t step = dtou64(ld->step, fixed_precision);

	uint64_t *real = ALLOC_ARRAY(real, length);
	uint64_t *img = ALLOC_ARRAY(img, length);
	uint64_t *real_ret = ALLOC_ARRAY(real_ret, length);
	uint64_t *img_ret = ALLOC_ARRAY(img_ret, length);
	unsigned *iterations = CALLOC_ARRAY(iterations, length);

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
