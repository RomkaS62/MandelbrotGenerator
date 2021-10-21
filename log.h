#ifndef MANDELBROT_LOG_H
#define MANDELBROT_LOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define m_efputs(_text)	__m_efputs(__FILE__, __LINE__, "[DBG] ", _text)

static inline void __m_efputs(const char *file, int line, const char *prefix, const char *msg)
{
	fprintf(stderr, "%s [ %s : %i ] %s", prefix, file, line, msg);
}

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_LOG_H */