#include <stdlib.h>
#include <string.h>
#include "frgen_string.h"

long str_idxof(const char *str, char ch)
{
	size_t i;

	for (i = 0; *str; i++, str++) {
		if (*str == ch) {
			return i;
		}
	}

	return -1;
}

char * strn_copy(const char *str, long length)
{
	char *ret;

	if (length < 1) {
		return NULL;
	}

	ret = malloc(length);
	memcpy(ret, str, length);

	return ret;
}

char * str_copy(const char *str)
{
	char *ret;
	size_t length;
	size_t i;

	length = strlen(str) + 1;
	ret = malloc(length);

	for (i = 0; i < length; i++) {
		ret[i] = str[i];
	}

	return ret;
}
