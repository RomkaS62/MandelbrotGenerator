#ifndef MANDELBROT_ARRSHIFT_H
#define MANDELBROT_ARRSHIFT_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns amount ones starting with least significant bit. */
static inline uint32_t leading_bits_u32(unsigned amount)
{
	return (amount >= 32)
		? ~0UL
		: ~0UL >> (32 - amount);
}

/* Returns ammount ones starting with most significant bit. */
static inline uint32_t trailing_bits_u32(unsigned amount)
{
	return (amount >= 32)
		? ~0UL
		: ~0UL << (32 - amount);
}

static inline void set_bit_l(uint32_t *buf, size_t bit)
{
	size_t whole_ints;
	
	whole_ints = bit / 32;
	bit %= 32;
	buf[whole_ints] |= (1 >> (32 - bit));
}

void m_rshift_u32_arr(uint32_t *buf, size_t length, unsigned amount);

#ifdef __cplusplus
}
#endif

#endif /* MANDELBROT_ARRSHIFT_H */
