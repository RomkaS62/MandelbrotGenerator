#include <stdlib.h>

#include "fractalgen/plugin.h"
#include "render-rgb.h"

struct point {
	double x;
	double y;
};

struct line {
	struct point p1;
	struct point p2;
};

static double interpolate(const struct line *l, double x)
{
	double dydx;

	dydx = (l->p2.y - l->p1.y) / (l->p2.x - l->p1.x);
	return (x - l->p1.x) * dydx + l->p1.y;
}

static double hue_r(double hue)
{
	static const struct line falling_edge = {
		{ 1.0 / 6.0, 1.0 },
		{ 1.0 / 3.0, 0.0 }
	};
	static const struct line rising_edge = {
		{ 2.0 / 3.0, 0.0 },
		{ 5.0 / 6.0, 1.0 }
	};

	if (hue <= 1.0 / 6.0 && hue >= 0.0) {
		return 1.0;
	} else if (hue > 1.0 / 6.0 && hue < 1.0 / 3.0) {
		return interpolate(&falling_edge, hue);
	} else if (hue >= 1.0 / 3.0 && hue <= 2.0 / 3.0) {
		return 0.0;
	} else if (hue > 2.0 / 3.0 && hue < 5.0 / 6.0) {
		return interpolate(&rising_edge, hue);
	} else if (hue >= 5.0 / 6.0 && hue < 1.0) {
		return 1.0;
	}

	return 1.0;
}

static double hue_g(double hue)
{
	static const struct line rising_edge = {
		{ 0.0, 0.0},
		{ 1.0 / 6.0, 1.0 }
	};
	static const struct line falling_edge = {
		{ 0.5, 1.0 },
		{ 2.0 / 3.0, 0.0 }
	};

	if (hue >= 0.0 && hue < 1.0 / 6.0) {
		return interpolate(&rising_edge, hue);
	} else if (hue >= 1.0 / 6.0 && hue <= 0.5) {
		return 1.0;
	} else if (hue >= 0.5 && hue < 2.0 / 3.0) {
		return interpolate(&falling_edge, hue);
	} else if (hue >= 2.0 / 3.0 && hue < 1.0) {
		return 0.0;
	}

	return 0.0;
}

static double hue_b(double hue)
{
	static const struct line rising_edge = {
		{ 1.0 / 3.0, 0.0 },
		{ 0.5, 1.0 }
	};
	static const struct line falling_edge = {
		{ 5.0 / 6.0, 1.0 },
		{ 1.0, 0.0 }
	};

	if (hue >= 0.0 && hue <= 1.0 / 3.0) {
		return 0.0;
	} else if (hue > 1.0 / 3.0 && hue < 0.5) {
		return interpolate(&rising_edge, hue);
	} else if (hue >= 0.5 && hue <= 5.0 / 6.0) {
		return 1.0;
	} else if (hue >= 5.0 / 6.0 && hue < 1.0) {
		return interpolate(&falling_edge, hue);
	}

	return 0.0;
}

struct pallette_s {
	unsigned char *r;
	unsigned char *g;
	unsigned char *b;
	unsigned char *a;
	size_t length;
};

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

static void pallette_init(struct pallette_s *p, size_t length)
{
	size_t i;
	float ratio;

	p->length = length;
	p->r = calloc(sizeof(p->r[0]), length);
	p->g = calloc(sizeof(p->g[0]), length);
	p->b = calloc(sizeof(p->b[0]), length);
	p->a = calloc(sizeof(p->a[0]), length);

	for (i = 0; i < length; i++) {
		ratio = (float)i / (float)length;
		p->r[i] = hue_r(ratio) * (float)255;
		p->g[i] = hue_g(ratio) * (float)255;
		p->b[i] = hue_b(ratio) * (float)255;
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

static void draw_pixels(
		const struct iteration_spec_s *spec,
		unsigned * restrict iterations,
		struct pixel *img,
		const struct param_set_s *set)
{
	struct pallette_s pallette;
	size_t line;
	size_t col;
	size_t index;
	unsigned itr;
	int is_black;

	pallette_init(&pallette, spec->iterations);

	for (line = 0; line < spec->rows; line++) {
		for (col = 0; col < spec->cols; col++) {
			index = line * spec->cols + col;
			itr = iterations[index];
			is_black = itr >= spec->iterations;
			img[index].r = pallette_red(&pallette, itr) * !is_black;
			img[index].g = pallette_green(&pallette, itr) * !is_black;
			img[index].b = pallette_blue(&pallette, itr) * !is_black;
		}
	}

	pallette_free(&pallette);
}

static const struct render_func_s renderers[] = {
	{ "render-rgb", draw_pixels }
};

const struct fractal_iterator_s iterators = {
	0, NULL, 1, renderers
};
