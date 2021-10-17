#include <inttypes.h>
#include <stdint.h>
#include "bmp.h"

#define BMP_HEADER_LENGTH	(2 + 4 + 2 + 2 + 4)
#define BMP_DIB_LENGTH		(4 + 2 + 2 + 2 + 2)
#define BMP_ZERO_OFFSET		(6)
#define BMP_ZERO_LENGTH		(4)
#define BMP_PIXEL_BYTES		(3)

static const char bmp_hdr_type[] = { 'B', 'M'};

static inline void write_u16_f(uint16_t num, FILE *f)
{
	char buf[] = { num & 0xFF, ((num >> 8) & 0xFF) };

	fwrite(buf, 1, 2, f);
}

static inline void write_u32_f(uint32_t num, FILE *f)
{
	char buf[] = {
		num & 0xFF,
		(num >> 8) & 0xFF,
		(num >> 16) & 0xFF,
		(num >> 24) & 0xFF
	};

	fwrite(buf, 1, 4, f);
}

static inline void write_pixel_f(struct pixel p, FILE *f)
{
	char buf[] = { p.b, p.g, p.r };
	fwrite(buf, 1, sizeof(buf), f);
}

static inline void write_f(const char *buf, size_t length, FILE *f)
{
	fwrite(buf, 1, length, f);
}

static inline void write_zero_f(size_t amount, FILE *f)
{
	while (amount --> 0)
		fputc(0, f);
}

static struct pixel average_pixel_area(const struct bmp_img *img,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	uint16_t i, j;
	unsigned long total_r = 0;
	unsigned long total_g = 0;
	unsigned long total_b = 0;
	unsigned long total_pixels;
	struct pixel ret;

	total_pixels = height * width;
	for (i = x; i < x + width; i++) {
		for (j = y; j < y + height; j++) {
			total_r += bmp_pixel(img, i, j).r;
			total_g += bmp_pixel(img, i, j).g;
			total_b += bmp_pixel(img, i, j).b;
		}
	}

	ret.r = total_r / total_pixels;
	ret.g = total_g / total_pixels;
	ret.b = total_b / total_pixels;

	return ret;
}

struct bmp_img * bmp_new(uint16_t width, uint16_t height)
{
	struct bmp_img *ret;

	ret = malloc(sizeof(*ret));
	ret->width = width;
	ret->height = height;
	ret->image = malloc(sizeof(ret->image[0]) * width * height);

	return ret;
}

void bmp_delete(struct bmp_img *img)
{
	if (!img)
		return;
	free(img->image);
	free(img);
}

struct bmp_img * bmp_downsample(const struct bmp_img *img, unsigned level)
{
	unsigned divisor;
	struct bmp_img *ret;
	uint16_t x, y;

	divisor = 1 << level;
	ret = bmp_new(img->width / divisor, img->height / divisor);

	for (x = 0; x < ret->width; x++) {
		for (y = 0; y < ret->height; y++) {
			*bmp_pixel_ref(ret, x, y)
				= average_pixel_area(
						img,
						x * divisor,
						y * divisor,
						divisor,
						divisor);
		}
	}

	return ret;
}

enum bmp_error bmp_write_f(const struct bmp_img *img, FILE *f)
{
	uint32_t pixel_offset;
	uint32_t bmp_file_size;
	unsigned row_length;
	unsigned row_padding;
	uint16_t y;
	uint16_t x;

	/* Pixel buffer offset begins at an index that is aligned to 4 bytes. */
	pixel_offset = BMP_HEADER_LENGTH + BMP_DIB_LENGTH;
	pixel_offset += (pixel_offset % 4)
		? 4 - (pixel_offset % 4)
		: pixel_offset;
	row_length = img->width * BMP_PIXEL_BYTES;
	row_padding = (row_length & 3)	/* Is row length a multiple of 4? */
		? 4 - (row_length & 3)
		: 0;
	bmp_file_size = pixel_offset + (row_length + row_padding) * img->height;

#ifndef NDEBUG
	printf("Width: %"PRIu16"\n", img->width);
	printf("Height: %"PRIu16"\n", img->height);
	printf("Pixel offset: %"PRIu32"\n", pixel_offset);
	printf("Row length: %u\n", row_length);
	printf("Row length with padding: %u\n", row_length + row_padding);
	printf("File length: %"PRIu32"\n", bmp_file_size);
#endif

	/* Bitmap header write. */
	write_f(bmp_hdr_type, sizeof(bmp_hdr_type), f);
	write_u32_f(bmp_file_size, f);	/* Size of bitmap file in bytes */
	write_zero_f(4, f);		/* Reserved */
	write_u32_f(pixel_offset, f);

	/* DIB block write. */
	write_u32_f(BMP_DIB_LENGTH, f);	/* Length of DIB field */
	write_u16_f(img->width, f);
	write_u16_f(img->height, f);
	write_u16_f(1, f);		/* Must be one */
	write_u16_f(24, f);		/* Bits/pixel */

	/* So far 26 had been written. Since I am writing pixel buffer at a 4
	 * byte aligned offset, two zeroes shall be written to bring up the
	 * bytes to 28. Therefore, length of the entire file is 28 + number of
	 * pixels * 3. */
	write_zero_f(2, f);

	/* Pixels are stored in rows to form a pixel array. Each y stores all
	 * bits of its pixels and adds padding to make sure the y length is a
	 * multiple of 4 bytes. */
	for (y = 0; y < img->height; y++) {
		for (x = 0; x < img->width; x++) {
			write_pixel_f(bmp_pixel(img, x, y), f);
		}
		write_zero_f(row_padding, f);
	}

	return BMP_SUCCESS;
}

const char * bmp_strerror(enum bmp_error error)
{
	switch (error) {
	default:
		return "Unknown error!";
	}
}
