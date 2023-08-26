#include <stdlib.h>
#include <string.h>

#include "fractalgen/plugin.h"
#include "fractalgen/param_set.h"

#include "debug.h"

static void meshgrid_x(float * restrict arr, size_t rows, size_t cols, float from, float step)
{
	size_t i;

	for (i = 0; i < cols; i++) {
		arr[i] = from + i * step;
	}

	for (i = 1; i < rows; i++) {
		memcpy(arr + i * cols, arr, cols * sizeof(arr[0]));
	}
}

static void arr_set(float * restrict arr, size_t length, const float val)
{
	size_t i;

	for (i = 0; i < length; i++) {
		arr[i] = val;
	}
}

static void meshgrid_y(float * restrict arr, size_t rows, size_t cols, float from, float step)
{
	size_t i;

	for (i = 0; i < rows; i++) {
		arr_set(arr + i * cols, cols, from + i * step);
	}
}

#define CHUNK_SIZE	(128 / sizeof(float))
static void julia_iterate_chunk(
	const float * restrict real,
	const float * restrict img,
	float * restrict real_ret,
	float * restrict img_ret,
	const float const_real,
	const float const_img,
	unsigned * restrict iterations,
	unsigned itr_count)
{
	size_t i;
	size_t j;
	float sqr_real;
	float sqr_img;

	memcpy(real_ret, real, sizeof(real[0]) * CHUNK_SIZE);
	memcpy(img_ret, img, sizeof(img[0]) * CHUNK_SIZE);

	/* (a + bi)^2 = a^2 - 2abi - b^2
	 * real = a^2 - b^2
	 * img = 2ab
	 */
	for (i = 0; i < itr_count; i++) {
		for (j = 0; j < CHUNK_SIZE; j++) {
			sqr_real = real_ret[j] * real_ret[j];
			sqr_img = img_ret[j] * img_ret[j];
			iterations[j] += ((sqr_real + sqr_img) <= 4.0) ? 1 : 0;
			img_ret[j] = 2.0f * real_ret[j] * img_ret[j] + const_img;
			real_ret[j] = sqr_real - sqr_img + const_real;
		}
	}
}

static void julia_iterate(
	const float * restrict real,
	const float * restrict img,
	float * restrict real_ret,
	float * restrict img_ret,
	const float const_real,
	const float const_img,
	unsigned * restrict iterations,
	unsigned itr_count,
	const size_t length)
{
	size_t i;
	size_t j;
	float sqr_real;
	float sqr_img;

	memcpy(real_ret, real, sizeof(real[0]) * length);
	memcpy(img_ret, img, sizeof(img[0]) * length);

	/* (a + bi)^2 = a^2 - 2abi - b^2
	 * real = a^2 - b^2
	 * img = 2ab
	 */
	for (i = 0; i < itr_count; i++) {
		for (j = 0; j < length; j++) {
			sqr_real = real_ret[j] * real_ret[j];
			sqr_img = img_ret[j] * img_ret[j];
			iterations[j] += ((sqr_real + sqr_img) <= 4.0) ? 1 : 0;
			img_ret[j] = 2.0f * real_ret[j] * img_ret[j] + const_img;
			real_ret[j] = sqr_real - sqr_img + const_real;
		}
	}
}

static void iterate_old(
	const struct frg_iteration_request_s *spec,
	unsigned * restrict iterations,
	const struct frg_param_set_s *params)
{
	float *buf;
	float *real;
	float *img;
	float *real_ret;
	float *img_ret;
	float const_real;
	float const_img;
	size_t buf_len;
	size_t i;

	dbg_puts("Using old julia iteration function");

	const_real = (float)param_set_get_double_d(params, "creal", 1.0);
	const_img = (float)param_set_get_double_d(params, "cimg", 1.0);

	dbg_printf("Iterating julia with constant of %f + %fi\n", const_real, const_img);

	buf_len = spec->rows * spec->cols;

	buf = malloc(buf_len * 4 * sizeof(buf[0]));
	real = buf;
	img = real + buf_len;
	real_ret = img + buf_len;
	img_ret = real_ret + buf_len;

	meshgrid_x(real, spec->rows, spec->cols, spec->from_x, spec->step);
	meshgrid_y(img, spec->rows, spec->cols, spec->from_y, spec->step);

	for (i = 0; i + CHUNK_SIZE < spec->rows; i += CHUNK_SIZE) {
		julia_iterate_chunk(
			real + i, img + i, real_ret + i, img_ret + i,
			const_real, const_img,
			iterations + i,
			spec->iterations);
	}

	julia_iterate(
		real + i, img + i, real_ret + i, img_ret + i,
		const_real, const_img,
		iterations + i,
		spec->iterations,
		buf_len - i);

	free(buf);
}

#define BLOCK_ROWS		(16)
#define BLOCK_COLS		(16)
#define BLOCK_PIXELS	(BLOCK_ROWS * BLOCK_COLS)
struct julia_block_s {
	float real[BLOCK_PIXELS];
	float img[BLOCK_PIXELS];
	unsigned iterations[BLOCK_PIXELS];
};

static void blk_meshgrid(struct julia_block_s *block, float fromX, float fromY, float step)
{
	size_t i;
	size_t j;

	for (i = 0; i < BLOCK_ROWS; i++) {
		for (j = 0; j < BLOCK_COLS; j++) {
			block->real[i * BLOCK_COLS + j] = fromX + j * step;
			block->img[i * BLOCK_COLS + j] = fromY + i * step;
		}
	}
}

static void blk_arr_meshgrid(struct julia_block_s * restrict blocks,
	size_t rows, size_t cols,	/* Number of blocks */
	float fromX, float fromY, float step)	/* Pixel values */
{
	size_t i;
	size_t j;

	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			blk_meshgrid(&blocks[i * cols + j],
				fromX + step * j * BLOCK_COLS,
				fromY + step * i * BLOCK_ROWS, step);
		}
	}
}

static void iterate_block(struct julia_block_s *block, unsigned iterations,
	const float c_real, const float c_img)
{
	size_t i;
	size_t j;
	float real_sqr;
	float img_sqr;

	for (i = 0; i < iterations; i++) {
		for (j = 0; j < BLOCK_PIXELS; j++) {
			real_sqr = block->real[j] * block->real[j];
			img_sqr = block->img[j] * block->img[j];
			block->iterations[j] += (real_sqr + img_sqr <= 4.0f) ? 1 : 0;
			block->img[j] = 2.0f * block->img[j] * block->real[j] + c_img;
			block->real[j] = real_sqr - img_sqr + c_real;;
		}
	}
}

static void iterate(
	const struct frg_iteration_request_s *spec,
	unsigned * restrict iterations,
	const struct frg_param_set_s *params)
{
	struct julia_block_s *blocks;
	size_t block_rows;
	size_t block_cols;
	size_t block_row;
	size_t block_col;
	size_t i;
	size_t j;
	float const_real;
	float const_img;
	int use_old;

	use_old = param_set_value_exists(params, "use-old");
	const_real = (float)param_set_get_double_d(params, "creal", 1.0);
	const_img = (float)param_set_get_double_d(params, "cimg", 1.0);

	dbg_printf("Iterating julia with constant of %f + %fi\n", const_real, const_img);

	if (use_old) {
		iterate_old(spec, iterations, params);
		return;
	}

	block_rows = spec->rows / BLOCK_ROWS + ((spec->rows % BLOCK_ROWS) ? 1 : 0);
	block_cols = spec->cols / BLOCK_COLS + ((spec->cols % BLOCK_COLS) ? 1 : 0);
	blocks = calloc(block_rows * block_cols, sizeof(blocks[0]));

	dbg_printf("Pixels: [%u x %u], blocks: [%zu x %zu]\n",
		spec->rows, spec->cols, block_rows, block_cols);

	blk_arr_meshgrid(blocks, block_rows, block_cols,
		spec->from_x, spec->from_y, spec->step);

	for (i = 0; i < block_rows; i++) {
		for (j = 0; j < block_cols; j++) {
			iterate_block(&blocks[i * block_cols + j], spec->iterations,
				const_real, const_img);
		}
	}

	for (i = 0; i < spec->rows; i++) {
		block_row = i / BLOCK_ROWS;
		for (j = 0; j < spec->cols; j++) {
			block_col = j / BLOCK_COLS;

			iterations[i * spec->cols + j]
				= blocks[block_row * block_cols + block_col]
					.iterations[(i % BLOCK_ROWS) * BLOCK_COLS + (j % BLOCK_COLS)];
		}
	}

	free(blocks);
}

void frg_module_init(struct frg_render_fn_repo_s *itr)
{
	frg_fn_repo_register_iterator(itr, "julia-float", iterate);
}
