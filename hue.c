#include "geometry.h"
#include "hue.h"
#include "global.h"

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

void pallette_init(struct pallette_s *p, size_t length)
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

void pallette_free(struct pallette_s *p)
{
	free(p->r);
	free(p->g);
	free(p->b);
	free(p->a);
}

void draw_pixels(
		const uint16_t ln_from,
		const uint16_t ln_to,
		const unsigned *restrict iterations,
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
			BMP_AT(img, col, line).r = pallette_red(pallette, itr) * !is_black;
			BMP_AT(img, col, line).g = pallette_green(pallette, itr) * !is_black;
			BMP_AT(img, col, line).b = pallette_blue(pallette, itr) * !is_black;
		}
	}
}

