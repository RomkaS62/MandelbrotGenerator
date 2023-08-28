#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include "fixed.h"

static const int precision = 47;

int main()
{
	int64_t min = -(1ULL << (64 - precision - 1)) >> precision / 2;
	int64_t max = (1ULL << (64 - precision - 1)) >> precision / 2;
	uint64_t a;
	uint64_t b;
	uint64_t uprod;
	int64_t prod;

	for (int64_t i = min; i < max; i++) {
		for (int64_t j = min; j < max; j++) {
			a = u64ffromi64(i, precision);
			b = u64ffromi64(j, precision);
			uprod = u64fmul(a, b, precision);
			prod = u64ftoi64(uprod, precision);
			if (prod != i * j) {
				printf("%"PRIi64" * %"PRIi64" != %"PRIi64"\n",
						i, j, prod);
				return 1;
			}
		}
	}

	return 0;
}
