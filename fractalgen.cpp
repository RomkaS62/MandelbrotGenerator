#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <inttypes.h>

#include <vector>
#include <thread>
#include <memory>

#include "bmp.h"
#include "hue.h"
#include "cdouble.h"
#include "fixed.h"
#include "global.h"
#include "frgen_string.h"

#include "fractalgen/param_set.h"
#include "fractalgen/plugin.h"

#include "debug.h"

#include <gramas/dynarray.h>
#include <gramas/ptr_array.h>

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#endif

static const char * get_opt(const char *opt, int offset, const char *default_val,
		int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; i++) {
		if (strcmp(opt, argv[i]) == 0) {
			if (i + offset < argc)
				return argv[i + offset];
			else
				return default_val;
		}
	}

	return default_val;
}

static int opt_is_set(const char *opt, int is_val, int is_not_val, int argc,
		char **argv)
{
	int i;

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], opt) == 0)
			return is_val;
	}

	return is_not_val;
}

static uint16_t get_opt_u16(const char *opt, int offset, uint16_t default_value,
		int argc, char **argv)
{
	unsigned long ret;
	char *endptr;
	const char *str;

	str = get_opt(opt, offset, NULL, argc, argv);
	if (!str)
		return default_value;
	if (!str)
		return default_value;
	ret = strtoul(str, &endptr, 0);
	if (ret > UINT16_MAX || *endptr || endptr == str)
		return default_value;

	return (uint16_t)ret;
}

static double get_opt_d(const char *opt, int offset, double default_value,
		int argc, char **argv)
{
	double ret;
	char *endptr;
	const char *str;

	str = get_opt(opt, offset, NULL, argc, argv);
	if (!str)
		return default_value;
	ret = strtod(str, &endptr);
	if (*endptr || endptr == str)
		return default_value;

	return ret;
}

static unsigned long get_opt_ul(const char *opt, int offset,
		unsigned long default_value, int argc, char **argv)
{
	unsigned long ret;
	char *endptr;
	const char *retstr;

	retstr = get_opt(opt, offset, NULL, argc, argv);
	if (!retstr)
		return default_value;
	ret = strtoul(retstr, &endptr, 0);
	if (*endptr || endptr == retstr)
		return default_value;

	return ret;
}

struct draw_lines_data_s {
	struct frg_iteration_request_s spec;
	unsigned * iterations;
	struct pixel *img;
	const struct frg_param_set_s *params;
	iterate_fn iterate;
	render_fn render;
};

static void draw_lines(struct draw_lines_data_s *data)
{
	data->iterate(&data->spec, data->iterations, data->params);
	data->render(&data->spec, data->iterations, data->img, data->params);
}

static void join_all(std::vector<std::unique_ptr<std::thread>> &thread_pool)
{
	for (size_t i = 0; i < thread_pool.size(); i++) {
		thread_pool[i].get()->join();
	}
}

#ifndef NDEBUG

static unsigned u_max(const unsigned *itr, size_t length)
{
	size_t i;
	unsigned ret;

	ret = itr[0];

	for (i = 1; i < length; i++) {
		if (ret < itr[i]) {
			ret = itr[i];
		}
	}

	return ret;
}

#define DUMP_COLS	(120)
#define DUMP_ROWS	(35)

static void dump_iterations(const unsigned *itr, size_t rows, size_t cols)
{
	size_t i = 0;
	size_t j = 0;
	size_t a = 0;
	size_t b = 0;
	size_t line = 0;
	size_t next_line = 0;
	size_t col;
	size_t next_col;
	size_t rows_to_dump;
	size_t cols_to_dump;
	unsigned long sum;
	unsigned max_itr;
	unsigned char brightness;

	max_itr = u_max(itr, rows * cols);

	rows_to_dump = (rows < DUMP_ROWS) ? rows : DUMP_ROWS;
	cols_to_dump = (cols < DUMP_COLS) ? cols : DUMP_COLS;

	while (i < rows_to_dump) {
		line = (rows * i) / rows_to_dump;
		next_line = (rows * (i + 1)) / rows_to_dump;

		if (line >= rows) {
			break;
		}

		if (next_line >= rows) {
			next_line = rows - 1;
		}

		j = 0;

		while (j < cols_to_dump) {
			col = (j * cols) / cols_to_dump;
			next_col = ((j + 1) * cols) / cols_to_dump;

			if (col >= cols) {
				break;
			}

			if (next_col >= cols) {
				next_col = cols - 1;
			}

			sum = 0;

			for (b = line; b <= next_line; b++) {
				for (a = col; a <= next_col; a++) {
					sum += itr[b * cols + a];
				}
			}

			brightness = (unsigned char)(
				255UL - ((sum * 255UL)
					/
				(((next_line - line + 1) * (next_col - col + 1)))) / max_itr
			);

			printf("\e[48;2;%u;%u;%um \e[0m", brightness, brightness, brightness);

			j++;
		}

		putchar('\n');

		i++;
	}
}

#else
#define dump_iterations(__itr, __r, __c)
#endif

static void draw_fractal(struct bmp_img *img, struct cdouble org, double r,
	iterate_fn iterate, render_fn render, const struct frg_param_set_s *params)
{
	std::vector<std::unique_ptr<std::thread>> thread_pool;
	std::vector<struct draw_lines_data_s> data;
	uint16_t i;
	uint16_t lines_per_thread;
	uint16_t lines_remainder;
	double step;
	uint16_t smaller_dimension;
	struct draw_lines_data_s new_data;
	unsigned *itrbuf;

	smaller_dimension = (img->height < img->width)
		? img->height
		: img->width;
	step = r / smaller_dimension;
	lines_per_thread = img->height / threads;
	lines_remainder = img->height % lines_per_thread;

	if (lines_remainder == 0) {
		lines_remainder = lines_per_thread;
	}

	itrbuf = (unsigned *)calloc(img->width * img->height, sizeof(itrbuf[0]));

	for (i = 0; i < threads - 1; i++) {
		new_data.spec.rows = lines_per_thread;
		new_data.spec.cols = img->width;
		new_data.spec.iterations = attempts;
		new_data.spec.from_x = org.real - step * (img->width / 2);
		new_data.spec.from_y = org.img - step * (img->height / 2) + lines_per_thread * step * i;
		new_data.spec.step = step;

		new_data.iterations = itrbuf + i * lines_per_thread * img->width;
		new_data.img = img->image + i * lines_per_thread * img->width;
		new_data.params = params;
		new_data.iterate = iterate;
		new_data.render = render;

		data.push_back(new_data);
	}

	new_data.spec.rows = lines_remainder;
	new_data.spec.cols = img->width;
	new_data.spec.iterations = attempts;
	new_data.spec.from_x = org.real - step * (img->width / 2);
	new_data.spec.from_y = org.img - step * (img->height / 2) + lines_per_thread * step * i;
	new_data.spec.step = step;

	new_data.iterations = itrbuf + i * lines_per_thread * img->width;
	new_data.params = params;
	new_data.img = img->image + i * lines_per_thread * img->width;
	new_data.iterate = iterate;
	new_data.render = render;

	data.push_back(new_data);

	for (i = 0; i < threads; i++) {
		thread_pool.push_back(std::make_unique<std::thread>(draw_lines , &data[i]));
	}

	join_all(thread_pool);

	dump_iterations(itrbuf, img->height, img->width);

	free(itrbuf);
}

static void gather_params(const int argc, const char **argv, struct frg_param_set_s *set)
{
	struct gr_dynarray params;
	struct value_s value;
	char *endptr;
	long eq_idx;
	int i;

	buf_init(&params, 1, sizeof(struct value_s));

	for (i = 0; i < argc; i++) {
		if (str_starts_with(argv[i], "-D")) {
			eq_idx = str_idxof(argv[i], '=');

			if (eq_idx < 0) {
				value.type = VALUE_NONE;
				value.name = str_copy(argv[i] + 2);
			} else {
				value.name = strn_copy(argv[i] + 2, eq_idx - 2);

				value.val.d = strtod(argv[i] + eq_idx + 1, &endptr);

				if (*endptr) {
					value.type = VALUE_STR;
					value.val.str = str_copy(argv[i] + eq_idx + 1);
				} else {
					value.type = VALUE_DOUBLE;
				}
			}

			buf_append(&params, &value);
		}
	}

	buf_trim(&params);
	set->values = (struct value_s *)params.buf;
	set->length = (int)params.used;
}

int main(int argc, char **argv)
{
	struct bmp_img *img = NULL;
	struct bmp_img *downsampled_img = NULL;
	FILE *f = NULL;
	const char *iterate_plugin_name = NULL;
	const char *render_plugin_name = NULL;
	int list_funcs = 0;
	size_t i;
	struct frg_param_set_s params;
	iterate_fn iterator_func = NULL;
	render_fn render_func = NULL;
	struct frg_render_fn_repo_s iterators;

#if defined(_WIN32) || defined(_WIN64)
	_set_fmode(_O_BINARY);
#endif

	width = get_opt_u16("-w", 1, 640, argc, argv);
	height = get_opt_u16("-h", 1, 480, argc, argv);
	origin.real = get_opt_d("-x", 1, 0.0, argc, argv);
	origin.img = get_opt_d("-y", 1, 0.0, argc, argv);
	radius = get_opt_d("-r", 1, 1.5, argc, argv);
	attempts = get_opt_ul("-a", 1, 1000, argc, argv);
	threads = get_opt_u16("-t", 1, 4, argc, argv);
	supersample_level = get_opt_u16("-s", 1, 0, argc, argv);
	iterate_plugin_name = get_opt("--iterate", 1, "mandelbrot-double", argc, argv);
	render_plugin_name = get_opt("--render", 1, "render-rgb", argc, argv);
	list_funcs = get_opt("--list", 0, NULL, argc, argv) != NULL;

	gather_params(argc, (const char **)argv, &params);

	if (list_funcs) {
		return 0;
	}

	const char *plugin_pattern = getenv("FRACTALGEN_PLUGIN_PATTERN");

	if (!plugin_pattern) {
		fputs("FRACTALGEN_PLUGIN_PATTERN environment variable not defined!", stderr);
	}

	frg_fn_repo_init(&iterators);

	if (frg_load_modules(plugin_pattern, &iterators)) {
		fputs("Cannot find required plugins\n", stderr);
		return 1;
	}

	iterator_func = frg_fn_repo_get_iterator(&iterators, iterate_plugin_name);
	render_func = frg_fn_repo_get_renderer(&iterators, render_plugin_name);

	if (iterator_func == NULL) {
		fprintf(stderr, "Cannot find iteration function %s!\n", iterate_plugin_name);
		return 1;
	}

	if (render_func == NULL) {
		fprintf(stderr, "Cannot find render function %s!\n", render_plugin_name);
		return 1;
	}

	frg_fn_repo_destroy(&iterators);

	printf("Iterating %s\nRendering %s\n", iterate_plugin_name, render_plugin_name);

	if (!threads) {
		threads = 1;
	}

	file = get_opt("-f", 1, "bitmap.bmp", argc, argv);

	if (!(f = fopen(file, "w"))) {
		fputs("Can't open file for writing!\n", stderr);
		return 1;
	}

	printf("Super-sample level %" PRIu16 "\n", supersample_level);
	printf("Base width: %" PRIu16 "\n", width * (1 << supersample_level));
	printf("Base height: %" PRIu16 "\n", height * (1 << supersample_level));
	printf("Threads: %" PRIu16 "\n", threads);
	img = bmp_new(width * (1 << supersample_level), height * (1 << supersample_level));
	draw_fractal(img, origin, radius, iterator_func, render_func, &params);
	printf("Rendering finished. Saving to %s\n", file);

	if (supersample_level) {
		downsampled_img = bmp_downsample(img, supersample_level);
		bmp_write_f(downsampled_img, f);
	} else {
		bmp_write_f(img, f);
	}

	fclose(f);
	bmp_delete(img);
	bmp_delete(downsampled_img);

	for (i = 0; (int)i < params.length; i++) {
		if (params.values[i].type == VALUE_STR) {
			free(params.values[i].val.str);
		}
	}

	return 0;
}
