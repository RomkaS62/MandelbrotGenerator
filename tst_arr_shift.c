#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "arrshift.h"

static const unsigned shift_bits = 36;
static uint32_t data[] = { 1, 2, 3, 4};
static const uint32_t expeced_result[] = { 0, 1 << 28, 2 << 28, 3 << 28};

int main(void)
{
	m_rshift_u32_arr(data, sizeof(data) / sizeof(data[0]), shift_bits);
	return memcmp(data, expeced_result, sizeof(data) / sizeof(data[0]));
}