/*
 * OpenCReports test
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdio.h>

#include "opencreport.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_expr *e;
	ocrpt_flat_expr *f = NULL;
	uint32_t *fidxs = NULL, n_fexpr, n_fidxs;

	char *str[] = {
		"1",
		"1",
		"1 + 1",
		"1 + 2 + 3",
		"'apple'",
		"'apple'",
		"1 + 2 + 3",
	};
	int nstr = sizeof(str) / sizeof(char *);
	int i;

	for (i = 0; i < nstr; i++) {
		char *err = NULL;

		printf("string: %s\n", str[i]);
		e = ocrpt_parse_expression(o, str[i], &err);
		if (e) {
			printf("expr reprinted: ");
			ocrpt_print_expression(e);

			ocrpt_flatten_expression(o, e, &f, &n_fexpr, &fidxs, &n_fidxs);
			printf("flattened expr array elements: %u, e idx: %u\n", n_fexpr, fidxs[n_fidxs - 1]);
		} else {
			printf("%s\n", err);
			ocrpt_strfree(err);
		}
		ocrpt_free_expr(e);
		printf("\n");
	}

#if 0
	for (i = 0; i < n_fidxs; i++) {
		printf("flattened expr reprinted: ");
		ocrpt_print_flat_expression(f, fidx);
	}
#endif

	ocrpt_free(o);

	return 0;
}
