#ifndef MADELBROT_BMP_H
#define MADELBROT_BMP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pixel {
	unsigned char b, g, r;
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

static inline size_t bmp_idx(const struct bmp_img *img, uint16_t x, uint16_t y)
{
	size_t ret = img->width * y + x;

#ifndef NDEBUG
	if (ret > img->width * img->height) {
		fprintf(stderr, "Invalid access at [%" PRIu16 ", %" PRIu16 "] in image of size [%" PRIu16 ", %" PRIu16 "]\n",
				x, y, img->width, img->height);
		abort();
	}
#endif

	return ret;
}

static inline struct pixel bmp_pixel(const struct bmp_img *img, uint16_t x, uint16_t y)
{
	return img->image[bmp_idx(img, x, y)];
}

static inline struct pixel * bmp_pixel_ref(const struct bmp_img *img, uint16_t x, uint16_t y)
{
	return &img->image[bmp_idx(img, x, y)];
}

static inline void bmp_set_r(struct bmp_img *img, uint16_t x, uint16_t y, unsigned char val)
{
	img->image[bmp_idx(img, x, y)].r = val;
}

static inline void bmp_set_g(struct bmp_img *img, uint16_t x, uint16_t y, unsigned char val)
{
	img->image[bmp_idx(img, x, y)].g = val;
}

static inline void bmp_set_b(struct bmp_img *img, uint16_t x, uint16_t y, unsigned char val)
{
	img->image[bmp_idx(img, x, y)].b = val;
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
