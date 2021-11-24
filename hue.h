#ifndef MANDELBROT_HUE_H
#define MANDELBROT_HUE_H

#include <stdlib.h>
#include "bmp.h"

#ifdef __cplusplus
#define restrict
extern "C" {
#endif


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

void pallette_init(struct pallette_s *p, size_t length);
void pallette_free(struct pallette_s *p);

void draw_pixels(
		const uint16_t ln_from,
		const uint16_t ln_to,
		const unsigned *restrict iterations,
		struct bmp_img *img,
		const struct pallette_s *pallette);

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_HUE_H */
