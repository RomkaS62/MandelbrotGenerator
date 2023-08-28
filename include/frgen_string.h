#ifndef FRACTALGEN_STRING_H
#define FRACTALGEN_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

long str_idxof(const char *str, char ch);
char * strn_copy(const char *str, long length);
char * str_copy(const char *str);
int str_starts_with(const char *str, const char *prefix);

#ifdef __cplusplus
}
#endif

#endif /* FRACTALGEN_STRING_H */
