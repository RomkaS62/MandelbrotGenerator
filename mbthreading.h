#ifndef MANDELBROT_THREADING_H
#define MANDELBROT_THREADING_H

#include "bmp.h"

struct draw_lines_data {
	struct bmp_img *img;
	uint16_t ln_from;
	uint16_t ln_to;
	double from_x;
	double from_y;
	double step;
};


#endif /* MANDELBROT_THREADING_H */
