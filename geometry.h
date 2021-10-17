#ifndef MANDELBROT_GEOMETRY_H
#define MANDELBROT_GEOMETRY_H

struct point {
	double x;
	double y;
};

struct line {
	struct point p1;
	struct point p2;
};

#endif /* MANDELBROT_GEOMETRY_H */
