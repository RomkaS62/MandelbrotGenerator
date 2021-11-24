#ifndef MANDELBROT_GLOBAL_H
#define MANDELBROT_GLOBAL_H

#include <stdlib.h>
#include <stdint.h>
#include "cdouble.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern uint16_t width;
extern uint16_t height;
extern double radius;
extern struct cdouble origin;
extern const char *file;
extern unsigned long attempts;
extern uint16_t threads;
extern uint16_t supersample_level;
extern size_t pallette_length;
extern int use_fixed;

static const int fixed_precision = 58;

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_GLOBAL_H */
