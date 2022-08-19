#ifndef FRACTALGEN_VALUE_SET_H
#define FRACTALGEN_VALUE_SET_H

#ifdef __cplusplus
extern "C" {
#endif

#define VALUE_NONE		(0)
#define VALUE_STR		(1)
#define VALUE_DOUBLE	(2)

union value_u {
	double d;
	char *str;
};

struct value_s {
	char *name;
	union value_u val;
	int type;
};

struct param_set_s {
	int length;
	struct value_s *values;
};

int param_set_get_double(const struct param_set_s *set, const char *name, double *val);
int param_set_value_exists(const struct param_set_s *set, const char *name);
double param_set_get_double_d(const struct param_set_s *set, const char *name, double default_val);

#ifdef __cplusplus
}
#endif

#endif /* FRACTALGEN_VALUE_SET_H */
