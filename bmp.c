#include <inttypes.h>
#include <stdint.h>
#include "bmp.h"

#define BMP_HEADER_LENGTH	(2 + 4 + 2 + 2 + 4)
#define BMP_DIB_LENGTH		(4 + 2 + 2 + 2 + 2)
#define BMP_ZERO_OFFSET		(6)
#define BMP_ZERO_LENGTH		(4)
#define BMP_PIXEL_BYTES		(3)

static const char bmp_magic_number[] = { 'B', 'M'};

static inline void write_u16_f(uint16_t num, FILE *f)
{
	unsigned char buf[] = { num & 0xFF, ((num >> 8) & 0xFF) };

	fwrite(buf, 1, 2, f);
}

static inline void write_u32_f(uint32_t num, FILE *f)
{
	unsigned char buf[] = {
		num & 0xFF,
		(num >> 8) & 0xFF,
		(num >> 16) & 0xFF,
		(num >> 24) & 0xFF
	};

	fwrite(buf, 1, 4, f);
}

static inline void write_pixel_f(struct pixel p, FILE *f)
{
	unsigned char buf[] = { p.b, p.g, p.r };
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

struct pixel32 {
	uint32_t b;
	uint32_t g;
	uint32_t r;
};

static void p32_add_p8(struct pixel32 *p32, const struct pixel p8)
{
	p32->r += p8.r;
	p32->g += p8.g;
	p32->b += p8.b;
}

static void p32_div(struct pixel32 *p32, unsigned long divisor)
{
	p32->r /= divisor;
	p32->g /= divisor;
	p32->b /= divisor;
}

static struct pixel p32_to_p8(const struct pixel32 *p32)
{
	struct pixel p;

	p.r = (unsigned char)p32->r;
	p.g = (unsigned char)p32->g;
	p.b = (unsigned char)p32->b;

	return p;
}

static struct pixel average_pixel_area(const struct bmp_img *img,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	uint16_t i, j;
	struct pixel32 total = { 0 };
	unsigned long total_pixels;

	total_pixels = height * width;
	for (i = x; i < x + width; i++) {
		for (j = y; j < y + height; j++) {
			p32_add_p8(&total, bmp_pixel(img, i, j));
		}
	}

	p32_div(&total, total_pixels);
	return p32_to_p8(&total);
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
	struct bmp_img *downsampled_img;
	uint16_t x, y;

	divisor = 1 << level;
	downsampled_img = bmp_new(img->width / divisor, img->height / divisor);

	for (x = 0; x < downsampled_img->width; x++) {
		for (y = 0; y < downsampled_img->height; y++) {
			*bmp_pixel_ref(downsampled_img, x, y)
				= average_pixel_area(
						img,
						x * divisor,
						y * divisor,
						divisor,
						divisor);
		}
	}

	return downsampled_img;
}

static size_t align_to(size_t val, size_t alignment)
{
	return (val % alignment) ? ((val / alignment) + 1) * alignment : val;
}

static size_t bytes_to_align(size_t val ,size_t alignment)
{
	return align_to(val, alignment) - val;
}

static unsigned row_length(const struct bmp_img *img)
{
	return img->width * BMP_PIXEL_BYTES;
}

static unsigned row_padding(const struct bmp_img *img)
{
	return bytes_to_align(row_length(img), 4);
}

static unsigned padded_row_length(const struct bmp_img *img)
{
	return align_to(row_length(img), 4);
}

static uint32_t pixel_offset(const struct bmp_img *img)
{
	(void) img;
	return align_to(BMP_HEADER_LENGTH + BMP_DIB_LENGTH, 4);
}

static uint32_t pixel_buffer_length(const struct bmp_img *img)
{
	return img->height * padded_row_length(img);
}

static uint32_t file_size(const struct bmp_img *img)
{
	return pixel_offset(img) + pixel_buffer_length(img);
}

static void write_pixel(const struct bmp_img *img, uint16_t x, uint16_t y, char **buf)
{
	struct pixel p;

	p = bmp_pixel(img, x, y);
	(*buf)[0] = p.b;
	(*buf)[1] = p.g;
	(*buf)[2] = p.r;
	*buf += 3;
}

static void pad_buffer(char **buf, size_t bytes, char value)
{
	size_t i;

	for (i = 0; i < bytes; i++) {
		(*buf)[i] = value;
	}
	*buf += bytes;
}

#ifndef NDEBUG
static void log_image_parameters(const struct bmp_img *img)
{
	printf("Width: %"PRIu16"\n", img->width);
	printf("Height: %"PRIu16"\n", img->height);
	printf("Pixel offset: %"PRIu32"\n", pixel_offset(img));
	printf("Row length: %u\n", row_length(img));
	printf("Row length with padding: %u\n", padded_row_length(img));
	printf("File length: %"PRIu32"\n", file_size(img));
}
#else
#define log_image_parameters(img, row_length, row_padding, pixel_offset, bmp_file_size)
#endif

static void write_header(const struct bmp_img *img, FILE *f)
{
	write_f(bmp_magic_number, sizeof(bmp_magic_number), f);
	write_u32_f(file_size(img), f);
	write_zero_f(4, f);		/* Reserved */
	write_u32_f(pixel_offset(img), f);
}

static void write_dib(const struct bmp_img *img, FILE *f)
{
	write_u32_f(BMP_DIB_LENGTH, f);	/* Length of DIB field */
	write_u16_f(img->width, f);
	write_u16_f(img->height, f);
	write_u16_f(1, f);		/* Must be one */
	write_u16_f(24, f);		/* Bits/pixel */
}

static void write_header_and_dib(const struct bmp_img *img, FILE *f)
{
	log_image_parameters(img);
	write_header(img, f);
	write_dib(img, f);

	/* So far 26 had been written. Since I am writing pixel buffer at a 4
	 * byte aligned offset, two zeroes shall be written to bring up the
	 * bytes to 28. Therefore, length of the entire file is 28 + number of
	 * pixels * 3. */
	write_zero_f(2, f);
}

enum bmp_error bmp_write_f(const struct bmp_img *img, FILE *f)
{
	char *pixel_buf;
	char *write_head;
	unsigned padding;
	uint32_t pixel_buf_len;
	uint16_t y;
	uint16_t x;

	padding = row_padding(img);
	pixel_buf_len = pixel_buffer_length(img);

	write_header_and_dib(img, f);

	pixel_buf = malloc(pixel_buf_len);

	write_head = pixel_buf;
	for (y = 0; y < img->height; y++) {
		for (x = 0; x < img->width; x++) {
			write_pixel(img, x, y, &write_head);
		}
		pad_buffer(&write_head, padding, 0);
	}

	fwrite(pixel_buf, 1, pixel_buf_len, f);

	free(pixel_buf);
	return BMP_SUCCESS;
}

const char * bmp_strerror(enum bmp_error error)
{
	switch (error) {
	default:
		return "Unknown error!";
	}
}
