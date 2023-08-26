#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fractalgen/plugin.h"
#include "fractalgen/memmove.h"

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

#if defined(__GNUC__) && defined(_ISOC11_SOURCE)
	#define CAN_USE_ALIGNED_BUFFERS	(1)
#endif

#if defined(__GNUC__) && defined(_ISOC11_SOURCE)
	#define ASSUME_ALIGNED(__ptr, __a)	\
		do { __ptr = __builtin_assume_aligned((__ptr), (__a)); } while (0)

	static inline void * frg_aligned_malloc(const size_t size, const size_t alignment)
	{
		return aligned_alloc(alignment, size);
	}
#else	/* Cannot use aligned buffers */
	#define ASSUME_ALIGNED(__ptr, __a) do {} while (0)
	#define frg_aligned_malloc(__sz, __a) malloc(__sz)
#endif

#define BUFFER_ALIGNMENT	(64)

/* Make sure the final size of mandelbrot_block_s comes out a multiple of 16 */
#define BLOCK_ROWS		(4)
#define BLOCK_COLS		(4)
#define BLOCK_LENGTH	(BLOCK_ROWS * BLOCK_COLS)

#define SQUARE(__x) ((__x) * (__x))
struct mandelbrot_block_s {
	double real[BLOCK_LENGTH];
	double img[BLOCK_LENGTH];
	double real_ret[BLOCK_LENGTH];
	double img_ret[BLOCK_LENGTH];
	unsigned iterations[BLOCK_LENGTH];
};

/* Test to see if a point is inside the main cardiod:
 *
 * p = sqrt((x - 1/4)^2 + y^2)
 * x <= p - 2p^2 + 1/4
 *
 * Square roots are a pain to compute:
 *
 * x <= sqrt((x - 1/4)^2 + y^2) - 2((x - 1/4)^2 + y^2) + 1/4
 * x - 1/4 + 2((x - 1/4)^2 + y^2) <= sqrt((x - 1/4)^2 + y^2)
 * (x - 1/4 + 2((x - 1/4)^2 + y^2))^2 <= (x - 1/4)^2 + y^2
 *
 * a = (x - 1/4 + 2((x - 1/4)^2 + y^2))^2
 * b = (x - 1/4)^2 + y^2
 */
static int block_inside_main_cardiod(struct mandelbrot_block_s *block)
{
	int i;
	double a;
	double b;
	double x;
	double y;

	for (i = 0; i < BLOCK_LENGTH; i++) {
		x = block->real[i];
		y = block->img[i];

		a = x - 0.25 + 2.0 * (SQUARE(x - 0.25) + SQUARE(y));
		a *= a;

		b = SQUARE(x - 0.25) + SQUARE(y);

		if (a > b) {
			return 0;
		}
	}

	return 1;
}

static void block_set_itr(struct mandelbrot_block_s *block, unsigned itr)
{
	int i;

	for (i = 0; i < BLOCK_LENGTH; i++) {
		block->iterations[i] = itr;
	}
}

static void iterate_block(
	struct mandelbrot_block_s *block,
	unsigned itr_count)
{
	unsigned i;
	unsigned j;
	double real_sqr;
	double img_sqr;

	ASSUME_ALIGNED(block, BUFFER_ALIGNMENT);

	if (block_inside_main_cardiod(block)) {
		block_set_itr(block, itr_count);
		return;
	}

	memcpy(block->real_ret, block->real, sizeof(block->real));
	memcpy(block->img_ret, block->img, sizeof(block->img));

	for (i = 0; i < itr_count; i++) {
		for (j = 0; j < BLOCK_LENGTH; j++) {
			real_sqr = SQUARE(block->real_ret[j]);
			img_sqr = SQUARE(block->img_ret[j]);
			block->iterations[j] += (real_sqr + img_sqr <= 4.0) ? 1 : 0;
			block->img_ret[j] = 2.0 * block->real_ret[j] * block->img_ret[j] + block->img[j];
			block->real_ret[j] = real_sqr - img_sqr + block->real[j];
		}
	}
}

static inline size_t ceil_div(size_t num, size_t den)
{
	return num / den + ((num % den) ? 1 : 0);
}

static void iterate_mandelbrot(
	const struct frg_iteration_request_s *spec,
	unsigned * restrict iterations,
	const struct frg_param_set_s *params)
{
	struct pack_matrix_blocks_args_s pack_args;
	struct mandelbrot_block_s *blocks;
	size_t buf_len;
	size_t i;
	size_t j;
	size_t block_rows;
	size_t block_cols;
	size_t blk_idx;
	double from_x;
	double from_y;

	block_rows = ceil_div(spec->rows, BLOCK_ROWS);
	block_cols = ceil_div(spec->cols, BLOCK_COLS);

	buf_len = block_rows * block_cols;

	blocks = frg_aligned_malloc(buf_len * sizeof(blocks[0]), BUFFER_ALIGNMENT);

	for (i = 0; i < block_rows; i++) {
		from_y = spec->from_y + i * spec->step * BLOCK_ROWS;
		for (j = 0; j < block_cols; j++) {
			from_x = spec->from_x + j * spec->step * BLOCK_COLS;
			blk_idx = i * block_cols + j;

			matrix_x_fs(blocks[blk_idx].real, BLOCK_ROWS, BLOCK_COLS, from_x, spec->step);
			matrix_y_fs(blocks[blk_idx].img, BLOCK_ROWS, BLOCK_COLS, from_y, spec->step);
		}
	}

	for (i = 0; i < buf_len; i++) {
		iterate_block(&blocks[i], spec->iterations);
	}

	pack_args.dest = (char *)iterations;
	pack_args.dest_rows = spec->rows;
	pack_args.dest_cols = spec->cols * sizeof(unsigned);

	pack_args.src = (const char *restrict)blocks->iterations;
	pack_args.src_rows = block_rows;
	pack_args.src_cols = block_cols;
	pack_args.src_block_rows = BLOCK_ROWS;
	pack_args.src_block_cols = BLOCK_COLS * sizeof(unsigned);
	pack_args.src_stride = sizeof(blocks[0]);

	frg_pack_matrix_blocks(&pack_args);

	free(blocks);
}

extern void frg_module_init(struct frg_render_fn_repo_s *itr)
{
	frg_fn_repo_register_iterator(itr, "mandelbrot-double", iterate_mandelbrot);
}
