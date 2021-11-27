#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "fixed.h"


static int div_ceil(int a, int b)
{
	return a / b + (a % b >= b / 2);
}

static int test_from_to(const int64_t min, const int64_t max, const int precision)
{
	int ret = 0;
	int intbits;
	int64_t i;
	uint64_t num;
	uint64_t sqr;
	int64_t ssqr;

	printf("<%i:%i> %"PRIi64" --- %"PRIi64"\n", intbits, precision, min, max);
	intbits = 64 - precision;
	for (i = min; i < max; i++) {
		num = u64ffromi64(i, precision);
		sqr = fsquare(num, precision);
		ssqr = u64ftoi64(sqr, precision);
		if (ssqr != i * i) {
			printf("\t! <%i:%i> %"PRIi64"^2 != %"PRIi64"\n",
					intbits, precision, i, ssqr);
			ret = 1;
		}
	}

	if (!ret)
		puts("Pass");

	return ret;
}

int main(void)
{
	static const int min_precision = 8;
	static const int max_prexision = 56;

	int ret = 0;
	int precision;
	int64_t min;
	int64_t max;
	int64_t range = ~(~0ULL << (min_precision / 2));

	for (precision = min_precision; precision <= max_prexision; precision++) {
		min = u64f_int64_min(precision) >> div_ceil(64 - precision - 1, 2) + 1;
		max = min + range;
		test_from_to(min, max, precision);

		max = u64f_int64_max(precision) >> div_ceil(64 - precision - 1, 2);
		min = max - range;
		test_from_to(min, max, precision);
	}

	return ret;
}
