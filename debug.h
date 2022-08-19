#ifndef FRACTALGEN_DEBUG_H
#define FRACTALGEN_DEBUG_H

#if defined(FRGEN_DEBUG_MALLOC) || !defined(NDEBUG)
	#include <stdlib.h>
	#include <stdio.h>

	static inline void * __dbg_malloc(size_t size,
		const char *file, const char *func, int line)
	{
		void * ret = malloc(size);

		if (ret == NULL) {
			fprintf(stderr, "malloc() failed at %s %s %i\n", file, func, line);
			abort();
		}

		return ret;
	}

	#define malloc(__size) __dbg_malloc(__size, __FILE__, __func__, __LINE__)
#endif

#ifndef NDEBUG
	#define dbg_printf(__fmt, ...) printf("%s:%s():%i:"	\
		__fmt, __FILE__, __func__, __LINE__, __VA_ARGS__)
	#define dbg_puts(__str) puts(__str)
#else
	#define dbg_printf(__fmt, ...)
	#define dbb_puts(__str)
#endif

#endif /* FRACTALGEN_DEBUG_H */
