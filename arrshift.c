#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "arrshift.h"

/* Right shifts an array of uint32_t of size length by amount bits. */
void m_rshift_u32_arr(uint32_t *buf, size_t length, unsigned amount)
{
	size_t i;
	unsigned shift_u32;
	unsigned shift_bits;
	uint32_t low_bits;

	shift_u32 = amount / 32;
	shift_bits = amount % 32;

	if (shift_u32 >= length) {
		memset(buf, 0, length * sizeof(*buf));
		return;
	}

	if (shift_u32 > 0) {
		memmove(&buf[shift_u32], buf, (length - shift_u32) * 8);
		memset(buf, 0, shift_u32);
	}

	for (i = length - 2; i > 0; i--) {
		buf[i + 1] >>= shift_bits;
		low_bits = buf[i] & leading_bits_u32(shift_bits);
		buf[i + 1] |= low_bits << (32 - shift_bits);
	}
}