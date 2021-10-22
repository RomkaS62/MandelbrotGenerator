#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include "log.h"
#include "mbarray.h"
#include "mbitr.h"
#include "parse.h"

size_t mb_num_of_digits(const char *str, unsigned radix)
{
	size_t ret = 0;
	size_t i;
	int ch;

	if (radix > 10) {
		foreach_str_l(str, ch, i) {
			if (!(isdigit(ch) || ch >= 'a' && ch < ('a' + (char)radix - 10)))
				break;
			ret++;
		}
	} else {
		foreach_str_l(str, ch, i) {
			if (!(ch >= '0' && ch < ('0' + (char)radix)))
				break;
			ret++;
		}
	}

	return ret;
}

/* In general to convert number of digits of A in base a to to number of digits
 * B in base b the following formula can be used:
 *
 *      B = ceil(A * log_b(a))
 *
 * For exaple: 21 bits in decimal would be the following number of digits:
 *
 *      D = ceil(21 * log_10(2))
 * */
size_t mb_arr_size_from_digits(size_t digits, unsigned radix)
{
	size_t ret;

	if (digits == 0)
		return 0;
	ret = ceil(digits * log2(radix) / 32.0);

	return ret;
}

unsigned mb_parse_digit(int ch, unsigned radix)
{
	if (radix <= 10) {
		return ch - '0';
	} else {
		if (isdigit(ch)) {
			return ch - '0';
		} else {
			return 10 + ch - 'a';
		}
	}

	return 0;
}

