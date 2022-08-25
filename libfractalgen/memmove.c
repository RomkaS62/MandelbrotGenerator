#include <string.h>
#include "fractalgen/memmove.h"

void frg_pack_matrix_blocks(const struct pack_matrix_blocks_args_s *args)
{
	size_t dest_row;
	size_t src_row_offset;
	size_t src_row_length;
	size_t src_row_idx;
	size_t src_block_row_idx;
	size_t block_offset;
	size_t src_offset;

	src_row_length = args->src_cols * args->src_stride;

	for (dest_row = 0; dest_row < args->dest_rows; dest_row++) {
		src_row_idx = dest_row / args->src_block_rows;
		src_row_offset = src_row_length * src_row_idx;

		src_block_row_idx = dest_row % args->src_block_rows;
		block_offset = src_block_row_idx * args->src_block_cols;

		src_offset = block_offset + src_row_offset;

		frg_pack_mem(
			args->dest + args->dest_cols * dest_row,
			args->src + src_offset,
			args->dest_cols,
			args->src_block_cols,
			args->src_stride);
	}
}

void frg_pack_mem(char *dest, const char *src,
	size_t length, size_t block_size, size_t stride)
{
	while (length >= block_size) {
		memcpy(dest, src, block_size);
		dest += block_size;
		src += stride;
		length -= block_size;
	}

	if (length) {
		memcpy(dest, src, length);
	}
}
