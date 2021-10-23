#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "mbarray.h"
#include "mbitr.h"
#include "big_int.h"
#include "log.h"
#include "mbutil.h"

struct test_case {
	const uint32_t *arr1;
	const size_t arr1_length;
	const uint32_t *arr2;
	const size_t arr2_length;
	const uint32_t *expected_result;
	const size_t exp_res_len;
};

/*
 *          1  2  3
 *          4  5  6
 *          -------
 *          6 12 18
 *       5 10 15
 *    4  8 12
 *    -------------
 *    4 13 28 27 18
 * */
static const uint32_t op1_1[] = { 3, 2, 1 };
static const uint32_t op2_1[] = { 6, 5, 4 };
static const uint32_t res_1[] = { 18, 27, 28, 13, 4 };

/*
 * 999 * 999 = 998 001
 */
static const uint32_t op1_2[] = { UINT32_MAX, UINT32_MAX, UINT32_MAX };
static const uint32_t op2_2[] = { UINT32_MAX, UINT32_MAX, UINT32_MAX };
static const uint32_t res_2[] = { 1, 0, 0, UINT32_MAX - 1, UINT32_MAX, UINT32_MAX};

static const uint32_t op1_3[] = { 1, 2, 3 };
static const uint32_t op2_3[] = { 0 };
static const uint32_t res_3[] = { 0 };

static const uint32_t op1_4[] = { 0 };
static const uint32_t op2_4[] = { 1, 2, 3 };
static const uint32_t res_4[] = { 0 };

static const struct test_case cases[] = {
	{	/* No overflow */
		op1_1, 3,
		op2_1, 3,
		res_1, 5
	}, {	/* Maximum overflow. */
		op1_2, 3,
		op2_2, 3,
		res_2, 6
	}, {	/* Left zero */
		op1_3, 3,
		op2_3, 1,
		res_3, 1
	}, {	/* Right zero */
		op1_4, 1,
		op2_4, 3,
		res_4, 1
	}
};

static const uint32_t sqr_arr[] = { UINT32_MAX, UINT32_MAX, UINT32_MAX };
static const uint32_t res_arr[] = { 1, 0, 0, UINT32_MAX - 1, UINT32_MAX, UINT32_MAX };

int main(void)
{
	struct big_int *op1;
	struct big_int *op2;
	struct big_int *res;
	const struct test_case *test_case;
	size_t i;

	for (i = 0; i < MB_ARR_SIZE(cases); i++) {
		test_case = &cases[i];
		op1 = bi_new_from_u32arr(test_case->arr1, test_case->arr1_length);
		op2 = bi_new_from_u32arr(test_case->arr2, test_case->arr2_length);
		res = bi_mul(op1, op2);
		print_bi(op1);
		print_bi(op2);
		print_biu32(test_case->expected_result, test_case->exp_res_len);
		print_bi(res);
		if (memcmp(test_case->expected_result, res->arr.buf, test_case->exp_res_len * sizeof(uint32_t))) {
			return 1;
		}
		bi_delete(res);
		bi_delete(op1);
		bi_delete(op2);
		puts("Pass\n");
	}

	op1 = bi_new_from_u32arr(sqr_arr, MB_ARR_SIZE(sqr_arr));
	op2 = bi_new_from_u32arr(sqr_arr, MB_ARR_SIZE(sqr_arr));
	print_bi(op1);
	bi_mul_i(op1, op2);
	print_bi(op1);
	print_biu32(res_arr, MB_ARR_SIZE(res_arr));
	if (memcmp(res_arr, op1->arr.buf, sizeof(res_arr))) {
		puts("Fail\n");
		return 2;
	}

	puts("Pass\n");

	return 0;
}
