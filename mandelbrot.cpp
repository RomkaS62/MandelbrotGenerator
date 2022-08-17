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
#include "iteratef.h"
#include "iterated.h"
#include "mbthreading.h"
#include "fixed.h"
#include "global.h"

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#endif

static const double float_to_doble_cutoff = 1e-3;

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

static void draw_lines(struct draw_lines_data *ld)
{
	if (use_fixed) {
		draw_lines_u64f4(ld);
	} else {
		if (radius < float_to_doble_cutoff)
			draw_lines_d(ld);
		else
			draw_lines_f(ld);
	}
}

static void join_all(std::vector<std::unique_ptr<std::thread>> &thread_pool)
{
	for (size_t i = 0; i < thread_pool.size(); i++) {
		thread_pool[i].get()->join();
	}
}

static void draw_mandelbrot(struct bmp_img *img, struct cdouble org, double r)
{
	std::vector<std::unique_ptr<std::thread>> thread_pool;
	std::vector<struct draw_lines_data> data;
	uint16_t i;
	uint16_t lines_per_thread;
	uint16_t lines_remainder;
	double step;
	uint16_t smaller_dimension;

	smaller_dimension = (img->height < img->width)
		? img->height
		: img->width;
	step = r / smaller_dimension;
	lines_per_thread = img->height / threads;
	lines_remainder = img->height % lines_per_thread;

	for (i = 0; i < threads - 1; i++) {
		struct draw_lines_data new_data = {
			img,
			(uint16_t)(i * lines_per_thread),
			(uint16_t)((i + 1) * lines_per_thread),
			org.real - (img->width / 2) * step,
			org.img - (img->height / 2) * step,
			step
		};
		data.push_back(draw_lines_data(new_data));
	}

	struct draw_lines_data new_data = {
		img,
		(uint16_t)((threads - 1) * lines_per_thread),
		(uint16_t)(threads * lines_per_thread + lines_remainder),
		org.real - (img->width / 2) * step,
		org.img - (img->height / 2) * step,
		step
	};

	data.push_back(draw_lines_data(new_data));

	for (i = 0; i < threads; i++) {
		thread_pool.push_back(std::make_unique<std::thread>(draw_lines , &data[i]));
	}

	join_all(thread_pool);
}

int main(int argc, char **argv)
{
	struct bmp_img *img = NULL;
	struct bmp_img *downsampled_img = NULL;
	FILE *f = NULL;

#if defined(_WIN32) || defined(_WIN64)
	_set_fmode(_O_BINARY);
#endif

	use_fixed = opt_is_set("--fixed", 1, 0, argc, argv);
	width = get_opt_u16("-w", 1, 640, argc, argv);
	height = get_opt_u16("-h", 1, 480, argc, argv);
	origin.real = get_opt_d("-x", 1, 0.0, argc, argv);
	origin.img = get_opt_d("-y", 1, 0.0, argc, argv);
	radius = get_opt_d("-r", 1, 1.5, argc, argv);
	attempts = get_opt_ul("-a", 1, 1000, argc, argv);
	threads = get_opt_u16("-t", 1, 4, argc, argv);
	supersample_level = get_opt_u16("-s", 1, 0, argc, argv);
	pallette_length = get_opt_ul("-p", 1, 1000, argc, argv);

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
	if (use_fixed) {
		printf("Using %i-bit precision fixed point arithmetic.\n", fixed_precision);
	} else {
		if (radius < float_to_doble_cutoff) {
			puts("Using double precision arihmetic");
		} else {
			puts("Using single precision arithmetic");
		}
	}
	img = bmp_new(width * (1 << supersample_level), height * (1 << supersample_level));
	draw_mandelbrot(img, origin, radius);
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

	return 0;
}
