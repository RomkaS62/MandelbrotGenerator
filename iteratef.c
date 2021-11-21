#ifndef MANDELBROT_ITERATEF_C
#define MANDELBROT_ITERATEF_C

#define DRAW_FUNC_NAME draw_lines_f
#define CPLX_T float
#include "iterate.tpl.c"
#undef CPLX_T
#undef DRAW_FUNC_NAME

#endif /* MANDELBROT_ITERATEF_C */
