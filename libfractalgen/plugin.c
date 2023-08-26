#include <stdlib.h>
#include <string.h>

#include "fractalgen/plugin.h"

void frg_fn_repo_init(struct frg_render_fn_repo_s *itr)
{
	ptr_arr_init(&itr->iterate_funcs, 16);
	ptr_arr_init(&itr->render_funcs, 16);
}

void frg_fn_repo_destroy(struct frg_render_fn_repo_s *itr)
{
	size_t i;
	struct frg_render_func_s *render_func;
	struct frg_iterate_func_s *iterate_func;

	for (i = 0; i < itr->iterate_funcs.used; i++) {
		iterate_func = itr->iterate_funcs.arr[i];
		free(iterate_func->name);
		free(iterate_func);
	}

	ptr_arr_delete(&itr->iterate_funcs);

	for (i = 0; i < itr->render_funcs.used; i++) {
		render_func = itr->render_funcs.arr[i];
		free(render_func->name);
		free(render_func);
	}

	ptr_arr_delete(&itr->render_funcs);
}

iterate_fn frg_fn_repo_get_iterator(struct frg_render_fn_repo_s *itr, const char *name)
{
	size_t i;
	struct frg_iterate_func_s *fn;

	for (i = 0; i < itr->iterate_funcs.used; i++) {
		fn = itr->iterate_funcs.arr[i];

		if (strcmp(fn->name, name) == 0) {
			return fn->iterate;
		}
	}

	return NULL;
}

render_fn frg_fn_repo_get_renderer(struct frg_render_fn_repo_s *itr, const char *name)
{
	size_t i;
	struct frg_render_func_s *fn;

	for (i = 0; i < itr->render_funcs.used; i++) {
		fn = itr->render_funcs.arr[i];

		if (strcmp(fn->name, name) == 0) {
			return fn->render;
		}
	}

	return NULL;
}

static char * string_copy(const char *str)
{
	char *ret;
	size_t len;

	len = strlen(str);
	ret = malloc(len + 1);
	memcpy(ret, str, len);
	ret[len] = '\0';

	return ret;
}

void frg_fn_repo_register_iterator(
		struct frg_render_fn_repo_s *itr,
		const char *name,
		iterate_fn fn)
{
	struct frg_iterate_func_s *itr_func;

	itr_func = malloc(sizeof(*itr_func));
	itr_func->name = string_copy(name);
	itr_func->iterate = fn;

	ptr_arr_add(&itr->iterate_funcs, itr_func);
}

void frg_fn_repo_register_renderer(
		struct frg_render_fn_repo_s *itr,
		const char *name, render_fn fn)
{
	struct frg_render_func_s *render_func;

	render_func = malloc(sizeof(*render_func));
	render_func->name = string_copy(name);
	render_func->render = fn;

	ptr_arr_add(&itr->render_funcs, render_func);
}
