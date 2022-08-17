#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <inttypes.h>

#include <glob.h>
#include <dlfcn.h>

#include <vector>
#include <thread>
#include <memory>

#include "bmp.h"
#include "hue.h"
#include "cdouble.h"
#include "fixed.h"
#include "global.h"
#include "frgen_string.h"

#include "fractalgen/plugin.h"
#include "fractalgen/param_set.h"

#include <gramas/dynarray.h>

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
	struct iteration_spec_s spec;
	unsigned * iterations;
	struct pixel *img;
	const struct param_set_s *params;
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

static void draw_fractal(struct bmp_img *img, struct cdouble org, double r,
	iterate_fn iterate, render_fn render, const struct param_set_s *params)
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
		new_data.spec.from_y = org.img + step * (img->height / 2) - lines_per_thread * step * i;
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
	new_data.spec.from_y = org.img + step * (img->height / 2) - lines_per_thread * step * i;
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

	free(itrbuf);
}

static std::vector<const struct iterator_func_s *> iterators;
static std::vector<const struct render_func_s *> renderers;

static iterate_fn get_iterate_func(const char *name)
{
	size_t i;

	for (i = 0; i < iterators.size(); i++) {
		if (strcmp(iterators[i]->name, name) == 0) {
			return iterators[i]->iterate;
		}
	}

	fprintf(stderr, "Cannot find iteration function %s\n", name);
	exit(3);

	return NULL;
}

static render_fn get_render_func(const char *name)
{
	size_t i;

	for (i = 0; i < renderers.size(); i++) {
		if (strcmp(renderers[i]->name, name) == 0) {
			return renderers[i]->render;
		}
	}

	fprintf(stderr, "Cannot find render function %s\n", name);
	exit(4);

	return NULL;
}

static int on_error(const char *epath, int _errno)
{
	return _errno;
}

static void load_plugins()
{
	glob_t g;
	const char *plugin_pattern;
	void *lib;
	struct fractal_iterator_s *iterator;
	size_t i;
	int j;

	plugin_pattern = getenv("FRACTALGEN_PLUGIN_PATTERN");

	if (plugin_pattern == NULL) {
		fputs("FRACTALGEN_PLUGIN_PATTERN environment variable not set!\n", stderr);
		exit(1);
	}

	if (glob(plugin_pattern, 0, on_error, &g)) {
		exit(2);
	}

	for (i = 0; i < g.gl_pathc; i++) {
		if ((lib = dlopen(g.gl_pathv[i], RTLD_NOW)) != NULL) {
			iterator = (struct fractal_iterator_s *)dlsym(lib, "iterators");

			if (iterator == NULL) {
				dlclose(lib);
			}

			for (j = 0; j < iterator->iterate_func_count; j++) {
				iterators.push_back(&iterator->iterate_funcs[j]);
			}

			for (j = 0; j < iterator->render_func_count; j++) {
				renderers.push_back(&iterator->render_funcs[j]);
			}
		}
	}

	globfree(&g);
}

static void gather_params(const int argc, const char **argv, struct param_set_s *set)
{
	struct gr_dynarray params;
	struct value_s value;
	char *endptr;
	long eq_idx;
	int i;

	buf_init(&params, 1, sizeof(struct value_s));

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-D") == 0) {
			eq_idx = str_idxof(argv[i], '=');

			if (eq_idx < 0) {
				value.type = VALUE_NONE;
				value.name = str_copy(argv[i] + 2);
			} else {
				value.name = strn_copy(argv[i] + 2, eq_idx - 1);

				value.val.d = strtod(argv[i], &endptr);

				if (*endptr) {
					value.type = VALUE_STR;
					value.val.str = str_copy(argv[i] + eq_idx);
				} else {
					value.type = VALUE_DOUBLE;
				}
			}
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
	iterate_fn itr_func = NULL;
	render_fn render_func = NULL;
	int list_funcs = 0;
	size_t i;
	struct param_set_s params;

	load_plugins();

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
		puts("Iteration functions:");
		for (i = 0; i < iterators.size(); i++) {
			printf("\t%s\n", iterators[i]->name);
		}

		puts("\nRender functions:");
		for (i = 0; i < renderers.size(); i++) {
			printf("\t%s\n", renderers[i]->name);
		}

		return 0;
	}

	itr_func = get_iterate_func(iterate_plugin_name);
	render_func = get_render_func(render_plugin_name);

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
	draw_fractal(img, origin, radius, itr_func, render_func, &params);
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
