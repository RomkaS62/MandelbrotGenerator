#include "geometry.h"
#include "hue.h"

static double interpolate(const struct line *l, double x)
{
	double dydx;

	dydx = (l->p2.y - l->p1.y) / (l->p2.x - l->p1.x);
	return (x - l->p1.x) * dydx + l->p1.y;
}


double hue_r(double hue)
{
	static const struct line falling_edge = {
		{ 1.0 / 6.0, 1.0 },
		{ 1.0 / 3.0, 0.0 }
	};
	static const struct line rising_edge = {
		{ 2.0 / 3.0, 0.0 },
		{ 5.0 / 6.0, 1.0 }
	};

	if (hue <= 1.0 / 6.0 && hue >= 0.0) {
		return 1.0;
	} else if (hue > 1.0 / 6.0 && hue < 1.0 / 3.0) {
		return interpolate(&falling_edge, hue);
	} else if (hue >= 1.0 / 3.0 && hue <= 2.0 / 3.0) {
		return 0.0;
	} else if (hue > 2.0 / 3.0 && hue < 5.0 / 6.0) {
		return interpolate(&rising_edge, hue);
	} else if (hue >= 5.0 / 6.0 && hue < 1.0) {
		return 1.0;
	}

	return 1.0;
}

double hue_g(double hue)
{
	static const struct line rising_edge = {
		{ 0.0, 0.0},
		{ 1.0 / 6.0, 1.0 }
	};
	static const struct line falling_edge = {
		{ 0.5, 1.0 },
		{ 2.0 / 3.0, 0.0 }
	};

	if (hue >= 0.0 && hue < 1.0 / 6.0) {
		return interpolate(&rising_edge, hue);
	} else if (hue >= 1.0 / 6.0 && hue <= 0.5) {
		return 1.0;
	} else if (hue >= 0.5 && hue < 2.0 / 3.0) {
		return interpolate(&falling_edge, hue);
	} else if (hue >= 2.0 / 3.0 && hue < 1.0) {
		return 0.0;
	}

	return 0.0;
}

double hue_b(double hue)
{
	static const struct line rising_edge = {
		{ 1.0 / 3.0, 0.0 },
		{ 0.5, 1.0 }
	};
	static const struct line falling_edge = {
		{ 5.0 / 6.0, 1.0 },
		{ 1.0, 0.0 }
	};

	if (hue >= 0.0 && hue <= 1.0 / 3.0) {
		return 0.0;
	} else if (hue > 1.0 / 3.0 && hue < 0.5) {
		return interpolate(&rising_edge, hue);
	} else if (hue >= 0.5 && hue <= 5.0 / 6.0) {
		return 1.0;
	} else if (hue >= 5.0 / 6.0 && hue < 1.0) {
		return interpolate(&falling_edge, hue);
	}

	return 0.0;
}
