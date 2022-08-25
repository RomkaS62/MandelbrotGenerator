#ifndef FRACTALGEN_MEMMOVE_H
#define FRACTALGEN_MEMMOVE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pack_matrix_blocks_args_s {
	char *restrict dest;		/* One contiguous matrix */
	const char *restrict src;	/* Matrix of smaller regular-sized matrices */

	/* Destination dimensions */
	size_t dest_rows;		/* Number of rows in destination matrix */
	size_t dest_cols;		/* Length of a single row in bytes */

	/* Source description */
	size_t src_rows;		/* Number of rows of source blocks */
	size_t src_cols;		/* Number of blocks in source row. */
	size_t src_block_rows;	/* Rows in a block in src */
	size_t src_block_cols;	/* Source block row length in bytes */
	size_t src_stride;		/* Distance between blocks in bytes */
};

/* Aggregates a matrix of smaller regular-sized matrices into a single,
 * contiguous matrix. */
void frg_pack_matrix_blocks(const struct pack_matrix_blocks_args_s *args);

/* Copies blocks of length _block_size_ spaced evenly every _stride_ bytes from
 * _src_ into a contiguous block _dest_ of length _length_. */
void frg_pack_mem(char *dest, const char *src,
	size_t length, size_t block_size, size_t stride);

#ifdef __cplusplus
}
#endif

#endif /* FRACTALGEN_MEMMOVE_H */
