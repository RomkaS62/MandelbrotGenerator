#ifndef MANDELBROT_ITR_H
#define MANDELBROT_ITR_H

#include <string.h>
#include <ctype.h>

#define get_wb(__arr, __len, __idx) ((__idx) < (__len) ? (__arr)[__idx] : (__arr)[0])

/* Iterates over entire string __str. Stores characters cast to lower case in
 * __ch. __idx keeps index of current character. */
#define foreach_str_l(__str, __ch, __idx)				\
	for ((__idx) = 0, (__ch) = tolower((__str)[__idx]);		\
			(__ch);						\
			(__idx)++, (__ch) = tolower((__str)[__idx]))

/* Iterates over string __str for __n characters. Stores characters in __ch.
 * __ch is always cast to lower case. Keeps index of current character in
 * __idx.  */
#define foreach_strn_l(__str, __ch, __idx, __n)				\
	for ((__idx) = 0, (__ch) = tolower((__str)[0]);			\
			(__idx) < (__n);				\
			++(__idx), (__ch) = tolower(get_wb(__str, __n, __idx)))

/* Iterates over string __str of length __n * __blksz in blocks of __blksz.
 * Keeps an index of the current block in __i. */
#define foreach_strn_b(__str, __block, __blksz, __i, __n)			\
	for ((__i) = 0, memcpy((__block), (&(__str)[(__i) * (__blksz)]),	\
				(__blksz));					\
										\
			(__i) < (__n);						\
										\
			(__i)++, ((__i) < (__n))				\
				? memcpy((__block),				\
					&(__str)[(__i) * (__blksz)],		\
					(__blksz)), 0				\
				: 0)

/* Iterates over string __str starting at __n for __n characters in reverse.
 * Stores characters in __ch.  __ch is always cast to lower case. Keeps index
 * of current character in __idx.  */
#define foreach_strn_lr(__str, __ch, __idx, __n)				\
	for ((__idx) = 0, (__ch) = tolower((__str)[__n - __idx - 1]);		\
			(__idx) < (__n);					\
			(__ch) = tolower((__str)[__n - __idx - 1]), (__idx)++)

/* Iterates over string __str of length __n * __blksz in blocks of __blksz in
 * reverse.  Keeps a reverse index of the current block in __i. */
#define foreach_strn_br(__str, __block, __blksz, __i, __n)				\
	for ((__i) = 0, memcpy((__block), (&(__str)[((__n) - (__i) - 1) * (__blksz)]),	\
				(__blksz));						\
											\
			(__i) < (__n);							\
											\
			(__i)++, ((__i) < (__n))					\
				? memcpy((__block),					\
					&(__str)[((__n) - (__i) - 1) * (__blksz)],	\
					(__blksz)), 0					\
				: 0)

/* Generic reverse for loop. Figuring it out was harder than expected. Shoutout
 * to lads at stackoverflow.com. */
#define foreach_reverse(__arr, __len, __idx, __e)		\
	for ((__idx) = (__len);					\
		(__e) = (__arr)[(__idx) ? (__idx) - 1 : 0],	\
		(__idx) --> 0;					\
		)

#define foreach_reverse_idx(__idx, __size)	\
	for ((__idx) = (__size); (__idx) --> 0;)

#endif /* MANDELBROT_ITR_H */
