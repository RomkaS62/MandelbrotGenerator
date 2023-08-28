#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "fixed.h"

static void report_fail_real(const int64_t real, const int64_t img,
		const int64_t res_real)
{
	printf("!\t%"PRIi64"^2 - %"PRIi64" != %"PRIi64"\n",
			real, img, res_real);
}

static void report_fail_img(const int64_t real, const int64_t img,
		const int64_t res_img)
{
	printf("!\t2 * %"PRIi64" * %"PRIi64" != %"PRIi64"\n",
			real, img, res_img);
}

static void report_success_real(const int64_t real, const int64_t img,
		const int64_t res_real)
{
	printf("\t%"PRIi64"^2 - %"PRIi64" == %"PRIi64"\n",
			real, img, res_real);
}

static void report_success_img(const int64_t real, const int64_t img,
		const int64_t res_img)
{
	printf("\t2 * %"PRIi64" * %"PRIi64" == %"PRIi64"\n",
			real, img, res_img);
}
static uint64_t fcmplx_sqr_real(const uint64_t real, const uint64_t img,
		const int precision)
{
	return u64ftoi64(fsquare(real, precision) - fsquare(img, precision),
				precision);
}

static uint64_t fcmplx_sqr_img(const uint64_t real, const uint64_t img,
		const int precision)
{
	return u64ftoi64(u64fmul(real, img, precision) << 1,
				precision);
}

static int test_img(const int64_t real_min, const int64_t real_max,
		const int64_t img, const int precision)
{
	int64_t i;
	int64_t expected_real;
	int64_t expected_img;
	uint64_t freal;
	uint64_t fimg;
	int64_t real_sqr;
	int64_t img_sqr;
	int ret = 0;

	fimg = u64ffromi64(img, precision);
	for (i = real_min; i < real_max; i++) {
		expected_real = i * i - img * img;
		expected_img = 2 * i * img;

		freal = u64ffromi64(i, precision);

		real_sqr = fcmplx_sqr_real(freal, fimg, precision);
		img_sqr = fcmplx_sqr_img(freal, fimg, precision);

		if (expected_real != real_sqr) {
			report_fail_real(i, img, real_sqr);
			ret = 1;
		} else {
			report_success_real(i, img, real_sqr);
		}

		if (expected_img != img_sqr) {
			report_fail_img(i, img, img_sqr);
			ret = 1;
		} else {
			report_success_img(i, img, img_sqr);
		}
	}

	return ret;
}

static int test_int64_from_to(const int64_t real_min, const int64_t real_max,
		const int64_t img_min, const int64_t img_max, const int precision)
{
	int64_t i;
	int ret = 0;

	for (i = img_min; i < img_max; i++) {
		ret |= test_img(real_min, real_max, i, precision);
	}

	return ret;
}

static int idiv_ceil(const int a, const int b)
{
	return a / b + (a % b >= b / 2);
}

static int64_t i64bits(const int amount)
{
	return ~(~0UL << amount);
}

static uint64_t total_range(const int precision)
{
	if (precision == 64)
		return UINT64_MAX;

	return (uint64_t)u64f_int64_max(precision)
		+ (uint64_t)-u64f_int64_min(precision);
}

static int intbits(const int precision)
{
	return 64 - precision;
}

#define GR_MIN(a, b) (((a) < (b)) ? (a) : (b))

static void upper_range(int precision, uint64_t range, int64_t *min, int64_t *max)
{
	precision += intbits(precision) / 2 + 1;
	*max = u64f_int64_max(precision);
	range = GR_MIN(range, total_range(precision));
	*min = *max - range;
}

static void lower_range(int precision, uint64_t range, int64_t *min, int64_t *max)
{
	precision += intbits(precision) / 2 + 1;
	*min = u64f_int64_min(precision);
	range = GR_MIN(range, total_range(precision));
	*max = *min + range;
}

static const int64_t test_range = 16;

static int test_upper_range(const int precision)
{
	int64_t min;
	int64_t max;

	upper_range(precision, test_range, &min, &max);
	return test_int64_from_to(min, max, min, max, precision);
}

static int test_lower_range(const int precision)
{
	int64_t min;
	int64_t max;

	lower_range(precision, test_range, &min, &max);
	return test_int64_from_to(min, max, min, max, precision);
}

static int test_precision(const int precision)
{
	return test_upper_range(precision)
		| test_lower_range(precision);
}

int main(void)
{
	int ret = 0;
	int precision;

	for (precision = 4; precision <= 58; precision++) {
		printf("--- <%i:%i> ---\n", intbits(precision), precision);
		ret |= test_precision(precision);
	}

	return ret;
}
