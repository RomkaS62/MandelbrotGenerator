#ifndef MANDELBROT_PARSE_H
#define MANDELBROT_PARSE_H

#include <string.h>
#include "mbarray.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t mb_num_of_digits(const char *str, unsigned radix);

/* This function returns the number of uint32_t's needed to store a number from
 * str in given radix. */
size_t mb_arr_size_from_digits(size_t digits, unsigned radix);

/* Assumes that ch is lower case and represents a digit within radix. */
unsigned mb_parse_digit(int ch, unsigned radix);

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_PARSE_H */
