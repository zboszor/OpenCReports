/*
 * OpenCReports test
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_expr *e;

	char *str[] = {
		/* Random */
		"random()"
	};
	int nstr = sizeof(str) / sizeof(char *);
	int i;

	for (i = 0; i < nstr; i++) {
		char *err = NULL;

		printf("string: %s\n", str[i]);
		e = ocrpt_expr_parse(o, str[i], &err);
		if (e) {
			ocrpt_result *r;

			printf("expr reprinted: ");
			ocrpt_expr_print(o, e);
			printf("expr nodes: %d\n", ocrpt_expr_nodes(e));

			ocrpt_expr_optimize(o, e);
			printf("expr optimized: ");
			ocrpt_expr_print(o, e);
			printf("expr nodes: %d\n", ocrpt_expr_nodes(e));

			r = ocrpt_expr_eval(o, e);
			ocrpt_result_print(r);
		} else {
			printf("%s\n", err);
			ocrpt_strfree(err);
		}
		ocrpt_expr_free(e);
		printf("\n");
	}

	ocrpt_free(o);

	return 0;
}
