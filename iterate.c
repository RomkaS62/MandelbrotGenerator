#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math.h"
#include "iterate.h"
#include "bmp.h"
#include "hue.h"
#include "mbthreading.h"

extern unsigned long attempts;

static void matrix_x_fs(double *ret, size_t rows, size_t cols, double from, double step)
{
	size_t i;

	for (i = 0; i < cols; i++)
		ret[i] = step * i + from;

	for (i = 1; i < rows; i++)
		memcpy(ret + i * cols, ret, cols * sizeof(*ret));
}

static void d_arr_set(double *arr, const size_t length, const double val)
{
	size_t i;

	for (i = 0; i < length; i++)
		arr[i] = val;
}

static void matrix_y_fs(double *ret, size_t rows, size_t cols, double from, double step)
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

struct iteration_s {
	double *real;
	double *img;
	double *real_ret;
	double *img_ret;
	unsigned *iterations;
	size_t length;
};

static double * alloc_double(size_t length)
{
	return (double *) malloc(length * sizeof(double));
}

static unsigned * calloc_unsigned(size_t length)
{
	return (unsigned *) calloc(length, sizeof(unsigned));
}

static void itr_init(struct iteration_s *itr, size_t length)
{
	itr->real = alloc_double(length);
	itr->img = alloc_double(length);
	itr->real_ret = alloc_double(length);
	itr->img_ret = alloc_double(length);
	itr->iterations = calloc_unsigned(length);
	itr->length = length;
}

static void itr_free(struct iteration_s *itr)
{
	free(itr->real);
	free(itr->img);
	free(itr->real_ret);
	free(itr->img_ret);
	free(itr->iterations);
}

struct pallette_s {
	unsigned char *r;
	unsigned char *g;
	unsigned char *b;
	unsigned char *a;
	size_t length;
};

static void pallette_init(struct pallette_s *p, size_t length)
{
	size_t i;
	double ratio;

	p->length = length;
	p->r = calloc(sizeof(p->r[0]), length);
	p->g = calloc(sizeof(p->g[0]), length);
	p->b = calloc(sizeof(p->b[0]), length);
	p->a = calloc(sizeof(p->a[0]), length);

	for (i = 0; i < length; i++) {
		ratio = (double)i / (double)length;
		p->r[i] = hue_r(ratio) * 255.0;
		p->g[i] = hue_g(ratio) * 255.0;
		p->b[i] = hue_b(ratio) * 255.0;
		p->a[i] = 255;
	}
}

static void pallette_free(struct pallette_s *p)
{
	free(p->r);
	free(p->g);
	free(p->b);
	free(p->a);
}

static inline unsigned char pallette_red(const struct pallette_s *pallette, size_t idx)
{
	return pallette->r[idx % pallette->length];
}

static inline unsigned char pallette_green(const struct pallette_s *pallette, size_t idx)
{
	return pallette->g[idx % pallette->length];
}

static inline unsigned char pallette_blue(const struct pallette_s *pallette, size_t idx)
{
	return pallette->b[idx % pallette->length];
}

static void draw_pixels(
		const uint16_t ln_from,
		const uint16_t ln_to,
		const unsigned *iterations,
		struct bmp_img *img,
		const struct pallette_s *pallette)
{
	size_t line;
	size_t col;
	size_t index;
	unsigned itr;
	int is_black;

	for (line = ln_from; line < ln_to; line++) {
		for (col = 0; col < img->width; col++) {
			index = (line - ln_from) * img->width + col;
			itr = iterations[index];
			is_black = itr >= attempts;
			bmp_set_r(img, col, line, pallette_red(pallette, itr) * !is_black);
			bmp_set_g(img, col, line, pallette_green(pallette, itr) * !is_black);
			bmp_set_b(img, col, line, pallette_blue(pallette, itr) * !is_black);
		}
	}
}

static const size_t block_size = 512;
static const double max_distance = 4.0;

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

static void iterate_block(
		const double *restrict real,
		const double *restrict img,
		double *restrict real_ret,
		double *restrict img_ret,
		unsigned *restrict iterations)
{
	size_t i;
	size_t j;
	double tmpr;
	double tmpi;
	int within_bounds;

	memcpy(real_ret, real, sizeof(real[0]) * block_size);
	memcpy(img_ret, img, sizeof(img[0]) * block_size);
	for (i = 0; i < attempts; i++) {
		for (j = 0; j < block_size; j++) {
			tmpr = real_ret[j];
			tmpi = img_ret[j];
			within_bounds = distance_check(tmpr, tmpi);
			iterations[j] += 1 * within_bounds;
			real_ret[j] = (square(tmpr) - square(tmpi) + real[j]) * within_bounds
				+ tmpr * !within_bounds;
			img_ret[j] = (2.0 * tmpr * tmpi + img[j]) * within_bounds
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
		const size_t length)
{
	size_t i;
	size_t j;
	double tmpr;
	double tmpi;
	int within_bounds;

	memcpy(real_ret, real, sizeof(real[0]) * block_size);
	memcpy(img_ret, img, sizeof(img[0]) * block_size);
	for (i = 0; i < attempts; i++) {
		for (j = 0; j < length; j++) {
			tmpr = real_ret[j];
			tmpi = img_ret[j];
			within_bounds = distance_check(tmpr, tmpi);
			iterations[j] += 1 * within_bounds;
			real_ret[j] = (square(tmpr) - square(tmpi) + real[j]) * within_bounds
				+ tmpr * !within_bounds;
			img_ret[j] = (2.0 * tmpr * tmpi + img[j]) * within_bounds
				+ tmpi * !within_bounds;
		}
	}
}

static void cmpl_buf_sqr_add_sd(struct iteration_s *itr)
{
	size_t blocks;
	size_t remainder;
	size_t remainder_offset;
	size_t offset;
	size_t i;

	blocks = itr->length / block_size;
	remainder = itr->length % block_size;
	remainder_offset = itr->length - blocks * block_size;

	for (i = 0; i < blocks; i++) {
		offset = i * block_size;
		iterate_block(itr->real + offset, itr->img + offset,
				itr->real_ret + offset, itr->img_ret + offset,
				itr->iterations + offset);
	}

	iterate(itr->real + remainder_offset, itr->img + remainder_offset,
			itr->real_ret + remainder_offset, itr->img_ret + remainder_offset,
			itr->iterations + remainder_offset, remainder);
}

void * draw_lines(void *data)
{
	struct draw_lines_data *ld = (struct draw_lines_data*)data;
	struct iteration_s itr;
	struct pallette_s pallette;
	size_t cols;
	size_t rows;
	double from_y;

	cols = ld->img->width;
	rows = ld->ln_to - ld->ln_from;
	from_y = ld->from_y + ld->step * ld->ln_from;

	itr_init(&itr, rows * cols);
	pallette_init(&pallette, attempts / 3);

	matrix_x_fs(itr.real, rows, cols, ld->from_x, ld->step);
	matrix_y_fs(itr.img, rows, cols, from_y, ld->step);

	cmpl_buf_sqr_add_sd(&itr);

	draw_pixels(ld->ln_from, ld->ln_to, itr.iterations, ld->img, &pallette);

	itr_free(&itr);
	pallette_free(&pallette);

	return NULL;
}
