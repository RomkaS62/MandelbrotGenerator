#include <stdlib.h>
#include <stdint.h>
#include "cdouble.h"

#ifdef __cplusplus
extern "C"
{
#endif

uint16_t width = 240;
uint16_t height = 320;
double radius = 1.5;
struct cdouble origin = { 0.0, 0.0 };
const char *file = "bitmap.bmp";
unsigned long attempts = 1000;
uint16_t threads = 4;
uint16_t supersample_level = 0;
size_t pallette_length = 1000;
int use_fixed = 0;

#ifdef __cplusplus
}
#endif
