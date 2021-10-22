#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "big_int.h"

enum operation_t {
	OP_ADD,
	OP_SUB,
	OP_MUL
};

int main(int argc, char **argv)
{
	struct big_int *op1;
	struct big_int *op2;
	struct big_int *res;
	enum operation_t op;
	char *str;

	if (argc == 2) {
		res = bi_new_from_str(argv[1], 10);
		if (!res) {
			return 5;
		}
		str = bi_tostr(res);
		puts(str);
		free(str);
		return 0;
	}

	if (argc != 4) {
		fputs("Expected 3 arguments", stderr);
		return 1;
	}

	if ((op1 = bi_new_from_str(argv[1], 10)) == NULL) {
		fprintf(stderr, "Not an integer: \"%s\"", argv[1]);
		return 2;
	}

	if (strcmp(argv[2], "+") == 0) {
		op = OP_ADD;
	} else if (strcmp(argv[2], "-") == 0) {
		op = OP_SUB;
	} else if (strcmp(argv[2], "*") == 0) {
		op = OP_MUL;
	} else {
		fprintf(stderr, "Unrecognized operator: \"%s\"\n", argv[2]);
		return 3;
	}

	if ((op2 = bi_new_from_str(argv[3], 10)) == NULL) {
		fprintf(stderr, "Not an integer: \"%s\"\n", argv[3]);
		return 4;
	}

	switch (op) {
	case OP_ADD:
		res = bi_add(op1, op2);
		break;
	case OP_SUB:
		res = bi_sub(op1, op2);
		break;
	case OP_MUL:
		res = bi_mul(op1, op2);
		break;
	}

	puts(str = bi_tostr(res));

	free(str);
	bi_delete(op1);
	bi_delete(op2);
	bi_delete(res);

	return 0;
}
