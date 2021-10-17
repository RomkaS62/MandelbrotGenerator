#ifndef MADELBROT_BMP_H
#define MADELBROT_BMP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pixel {
	unsigned char r, g, b;
};

/* A bitmap image. The pixel array is a 2D one. Pixel[0, 0] represents bottom
 * left corner of the image */
struct bmp_img {
	uint16_t width;
	uint16_t height;
	struct pixel *image;
};

enum bmp_error {
	BMP_SUCCESS,
	BMP_ENOTBMP,
	BMP_ENOHDR
};

static inline struct pixel bmp_pixel(const struct bmp_img *img, uint16_t x, uint16_t y)
{
	return img->image[y * img->width + x];
}

static inline struct pixel * bmp_pixel_ref(const struct bmp_img *img, uint16_t x, uint16_t y)
{
	return &img->image[y * img->width + x];
}

struct bmp_img * bmp_new(uint16_t width, uint16_t height);
void bmp_delete(struct bmp_img *img);
struct bmp_img * bmp_downsample(const struct bmp_img *img, unsigned level);
enum bmp_error bmp_write_f(const struct bmp_img *img, FILE *f);
const char * bmp_strerror(enum bmp_error error);

#ifdef __cplusplus
}
#endif

#endif /* MADELBROT_BMP_H */
