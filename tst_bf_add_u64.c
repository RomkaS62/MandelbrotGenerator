#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "big_float.h"

static const uint64_t val = 0x100020000
static const uint32_t expected_mantissa[] = { val >> 33, (val & 0xFFFFFFFF) >> 1 };

int main(void)
{
	struct mb_big_float *bf_num;	/* Zero at start. */
	struct mb_big_float *sum;

	bf_num = mb_bf_new(2);
	sum = mb_bf_add_u64(&bf_num, &val);

	mb_bf_felete(bf_num);
	mb_bf_felete(sum);
	return 0;
}
