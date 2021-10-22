#ifndef MANDELBROT_LOG_H
#define MANDELBROT_LOG_H

#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define __M_FORMAT(__fmt_idx, __arg_idx) __attribute__((format(printf, __fmt_idx, __arg_idx)))
#else
#define __M_FORMAT(a, b)
#endif

#ifndef NDEBUG

#define PR_ERR	"[ ERR ]"
#define PR_DBG	"[ DBG ]"
#define m_efputs(_text)	__m_efputs(PR_ERR, _text)
#define m_eprintf(_fmt, ...) __m_eprintf(PR_ERR, _fmt, __VA_ARGS__)
#define m_fputs(_text)	__m_efputs(PR_DBG, _text)
#define m_printf(_fmt, ...) __m_eprintf(PR_DBG, _fmt, __VA_ARGS__)

static inline void __m_efputs(const char *prefix, const char *msg)
{
	fprintf(stderr, "%s %s", prefix, msg);
}

__M_FORMAT(2, 3) static inline void __m_eprintf(const char *prefix, const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	fprintf(stderr, "%s ", prefix);
	vfprintf(stderr, fmt, list);
	va_end(list);
}

static inline void __m_fputs(const char *prefix, const char *msg)
{
	printf("%s %s", prefix, msg);
}

__M_FORMAT(2, 3) static inline void __m_printf(const char *prefix, const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);
	printf("%s ", prefix);
	vprintf(fmt, list);
	va_end(list);
}

static inline void dbg_hexdump(const char *buf, size_t len)
{
	size_t i;

	fprintf(stdout, "%s ", PR_DBG);
	for (i = 0; i < len; i++) {
		if (i && i % 32) {
			fputs("\n", stdout);
		}
		printf("%02X ", buf[i]);
	}
	fputs("\n", stdout);
}

static inline void dbg_hexdump_u32(const uint32_t *arr, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		printf("%10"PRIu32" ", arr[i]);
	}
	puts("");
}

#else	/* Not debug */

#define dbg_hexdump(_buf, _len)
#define dbg_hexdump_u32(_arr, _len)
#define m_efputs(_text)
#define m_eprintf(_text, ...)
#define m_fputs(_text)
#define m_printf(_text, ...)

#endif

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_LOG_H */
