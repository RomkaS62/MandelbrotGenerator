#include <dlfcn.h>
#include <glob.h>
#include <string.h>

#include "fractalgen/plugin.h"

#include "debug.h"

static int on_error(const char *epath, int _errno)
{
	(void) epath;
	return _errno;
}

struct glob_iterator_s {
	glob_t g;
	size_t i;
};

static int glob_iterator_init(struct glob_iterator_s *itr, const char *pattern)
{
	int ret;

	itr->i = 0;

	if ((ret = glob(pattern, 0, on_error, &itr->g))) {
		switch (ret) {
		case GLOB_NOMATCH:
			fputs("Couldn't find a single plugin library!\n", stderr);
			break;
		case GLOB_ABORTED:
			fputs("glob() read error!\n", stderr);
			break;
		case GLOB_NOSPACE:
			fputs("glob() ran out of memory!\n", stderr);
			break;
		default:
			fputs("Unknown glob() error!\n", stderr);
			break;
		}

		fprintf(stderr, "Search pattern: %s\n", pattern);
	}

	return ret;
}

static int glob_iterator_has_next(const struct glob_iterator_s *itr)
{
	return itr->i < itr->g.gl_pathc;
}

static void glob_iterator_next(struct glob_iterator_s *itr)
{
	itr->i++;
}

static const char * glob_iterator_get(struct glob_iterator_s *itr)
{
	return itr->g.gl_pathv[itr->i];
}

static void glob_iterator_destroy(struct glob_iterator_s *itr)
{
	globfree(&itr->g);
}

#define FOREACH_GLOB_PATH(__itr, __path)	\
	for (__path = glob_iterator_get(__itr);	\
		glob_iterator_has_next(__itr);	\
			glob_iterator_next(__itr), __path = glob_iterator_get(__itr))

int frg_load_modules(
		const char *plugin_glob_pattern,
		struct frg_render_fn_repo_s *iterators)
{
	struct glob_iterator_s gitr;
	const char *path;
	frg_module_init_fn module_init_fn;
	void *lib;

	if (glob_iterator_init(&gitr, plugin_glob_pattern)) {
		return 1;
	}

	FOREACH_GLOB_PATH(&gitr, path) {
		dbg_printf("Trying to load %s...\n", path);

		if ((lib = dlopen(path, RTLD_NOW)) == NULL) {
			dbg_printf("[dlerror(): %s]Failed to open file %s\n", dlerror(), path);
			continue;
		}

		module_init_fn = (frg_module_init_fn)dlsym(lib, "frg_module_init");

		if (module_init_fn == NULL) {
			dbg_printf("%s does not contain symbol frg_module_init\n", path);
			dlclose(lib);
			continue;
		}

		module_init_fn(iterators);
	}

	glob_iterator_destroy(&gitr);

	return 0;
}
