#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math.h"
#include "bmp.h"
#include "hue.h"
#include "mbthreading.h"

extern unsigned long attempts;

static void matrix_x_fs(CPLX_T *ret, size_t rows, size_t cols, CPLX_T from, CPLX_T step)
{
	size_t i;

	for (i = 0; i < cols; i++)
		ret[i] = step * i + from;

	for (i = 1; i < rows; i++)
		memcpy(ret + i * cols, ret, cols * sizeof(*ret));
}

static void d_arr_set(CPLX_T *arr, const size_t length, const CPLX_T val)
{
	size_t i;

	for (i = 0; i < length; i++)
		arr[i] = val;
}

static void matrix_y_fs(CPLX_T *ret, size_t rows, size_t cols, CPLX_T from, CPLX_T step)
{
	CPLX_T val;
	size_t y;
	size_t row_offset;

	for (y = 0; y < rows; y++) {
		val = y * step + from;
		row_offset = y * cols;
		d_arr_set(ret + row_offset, cols, val);
	}
}

struct iteration_s {
	CPLX_T *real;
	CPLX_T *img;
	CPLX_T *real_ret;
	CPLX_T *img_ret;
	unsigned *iterations;
	size_t length;
};

static CPLX_T * alloc_double(size_t length)
{
	return (CPLX_T *) malloc(length * sizeof(CPLX_T));
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
	CPLX_T ratio;

	p->length = length;
	p->r = calloc(sizeof(p->r[0]), length);
	p->g = calloc(sizeof(p->g[0]), length);
	p->b = calloc(sizeof(p->b[0]), length);
	p->a = calloc(sizeof(p->a[0]), length);

	for (i = 0; i < length; i++) {
		ratio = (CPLX_T)i / (CPLX_T)length;
		p->r[i] = hue_r(ratio) * (CPLX_T)255;
		p->g[i] = hue_g(ratio) * (CPLX_T)255;
		p->b[i] = hue_b(ratio) * (CPLX_T)255;
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
static const CPLX_T max_distance = (CPLX_T)4.0;

static CPLX_T square(const CPLX_T val)
{
	return val * val;
}

static inline CPLX_T magnitude_sqr(const CPLX_T x, const CPLX_T y)
{
	return square(x) + square(y);
}

static inline int distance_check(const CPLX_T x, const CPLX_T y)
{
	return magnitude_sqr(x, y) < max_distance && !isnan(x) && !isnan(y) && !isnan(x * y);
}

static void iterate(
		const CPLX_T *restrict real,
		const CPLX_T *restrict img,
		CPLX_T *restrict real_ret,
		CPLX_T *restrict img_ret,
		unsigned *restrict iterations,
		const size_t length)
{
	size_t i;
	size_t j;
	CPLX_T tmpr;
	CPLX_T tmpi;
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
			img_ret[j] = (2.0f * tmpr * tmpi + img[j]) * within_bounds
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
		iterate(itr->real + offset, itr->img + offset,
				itr->real_ret + offset, itr->img_ret + offset,
				itr->iterations + offset, block_size);
	}

	iterate(itr->real + remainder_offset, itr->img + remainder_offset,
			itr->real_ret + remainder_offset, itr->img_ret + remainder_offset,
			itr->iterations + remainder_offset, remainder);
}

void * DRAW_FUNC_NAME(void *data)
{
	struct draw_lines_data *ld = (struct draw_lines_data*)data;
	struct iteration_s itr;
	struct pallette_s pallette;
	size_t cols;
	size_t rows;
	CPLX_T from_y;

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
