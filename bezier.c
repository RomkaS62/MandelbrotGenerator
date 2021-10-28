#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct point {
	double x;
	double y;
};

struct control_point {
	struct point base;
	struct point control;
};

struct bezier {
	struct control_point cp[2];
};

static struct bezier curve;
static unsigned long samples = 1000;

static double lerp_d(double d1, double d2, double t)
{
	return d1 + (d2 - d1) * t;
}

static struct point lerp_p(struct point p1, struct point p2, double t )
{
	struct point ret;

	ret.x = lerp_d(p1.x, p2.x, t);
	ret.y = lerp_d(p1.y, p2.y, t);

	return ret;
}

/*  cp1     cp2
 *   X-------X
 *  /         \
 * X bp1       X bp2
 * */
static struct point sample_bezier(struct bezier *curve, double t)
{
	struct point first_lerp[3];
	struct point second_lerp[2];
	struct point ret;
	size_t i;

	first_lerp[0] = lerp_p(curve->cp[0].base, curve->cp[0].control, t);
	first_lerp[1] = lerp_p(curve->cp[0].control, curve->cp[1].control, t);
	first_lerp[2] = lerp_p(curve->cp[1].control, curve->cp[1].base, t);

	second_lerp[0] = lerp_p(first_lerp[0], first_lerp[1], t);
	second_lerp[1] = lerp_p(first_lerp[1], first_lerp[2], t);

	ret = lerp_p(second_lerp[0], second_lerp[1], t);

	return ret;
}

static double parse_double(const char *str)
{
	double ret;
	char *endptr;

	errno = 0;
	ret = strtod(str, &endptr);
	if (errno) {
		fputs("Failed to parse coordinate\n", stderr);
		exit(2);
	}

	return ret;
}

static unsigned long parse_ul(const char *str)
{
	unsigned long ret;
	char *endptr;

	errno = 0;
	ret = strtoul(str, &endptr, 0);
	if (errno) {
		fputs("Failed to parse sample count\n", stderr);
		exit(3);
	}

	return ret;
}

int main(int argc, char **argv)
{
	double t;
	unsigned long i;
	struct point sample;

	if (argc != 10)
		return 1;

	curve.cp[0].base.x = parse_double(argv[1]);
	curve.cp[0].base.y = parse_double(argv[2]);
	curve.cp[0].control.x = parse_double(argv[3]);
	curve.cp[0].control.y = parse_double(argv[4]);
	curve.cp[1].base.x = parse_double(argv[5]);
	curve.cp[1].base.y = parse_double(argv[6]);
	curve.cp[1].control.x = parse_double(argv[7]);
	curve.cp[1].control.y = parse_double(argv[8]);
	samples = parse_ul(argv[9]);

	for (i = 0; i < samples; i++) {
		t = (double)i / (double)(samples - 1);
		sample = sample_bezier(&curve, t);
		printf("%.20e %.20e\n", sample.x, sample.y);
	}

	return 0;
}
