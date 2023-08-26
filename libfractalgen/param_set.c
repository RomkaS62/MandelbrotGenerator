#include <string.h>
#include "fractalgen/param_set.h"

int param_set_get_double(const struct frg_param_set_s *set, const char *name, double *val)
{
	int i;

	for (i = 0; i < set->length; i++) {
		if (strcmp(name, set->values[i].name) == 0) {
			*val = set->values[i].val.d;
			return 0;
		}
	}

	return 1;
}

int param_set_value_exists(const struct frg_param_set_s *set, const char *name)
{
	int i;

	for (i = 0; i < set->length; i++) {
		if (strcmp(name, set->values[i].name) == 0) {
			return 1;
		}
	}

	return 0;
}

double param_set_get_double_d(const struct frg_param_set_s *set, const char *name, double default_val)
{
	double ret;

	ret = default_val;
	param_set_get_double(set, name, &ret);

	return ret;
}
