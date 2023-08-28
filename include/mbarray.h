#ifndef MANDELBROT_ARRAY_H
#define MANDELBROT_ARRAY_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mb_array_u32 {
	uint32_t *buf;
	size_t len;
};

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_ARRAY_H */
