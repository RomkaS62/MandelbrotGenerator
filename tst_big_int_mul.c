#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "mbarray.h"
#include "big_int.h"

struct test_case {
	uint32_t *arr1;
	size_t arr1_length;
	uint32_t *arr2;
	size_t arr2_length;
	uint32_t *expected_result;
	size_t exp_res_len;
};

static const struct test_case cases[] = {
	{
		(uint32_t[]){ 1, 2, 3}, 3,
		(uint32_t[]){ 4, 5, 6}, 3,
		(uint32_t[]){ 4, 13, 28, 27, 18}, 5
	}
};

int main(void)
{
	struct big_int *op1;
	struct big_int *op2;
	struct big_int *res;
	const struct test_case *test_case;
	size_t i;

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		test_case = &cases[i];
		op1 = bi_new_from_u32arr(test_case->arr1, test_case->arr1_length);
		op2 = bi_new_from_u32arr(test_case->arr2, test_case->arr2_length);
		res = bi_mul(op1, op2);
		if (memcmp(test_case->expected_result, res->arr.buf, test_case->exp_res_len)) {
			return 1;
		}
		bi_delete(res);
		bi_delete(op1);
		bi_delete(op2);
	}

	return 0;
}
