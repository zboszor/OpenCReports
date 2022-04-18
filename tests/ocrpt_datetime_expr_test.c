/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_expr *e;

	char *str[] = {
		/* NULL tests */
		"nulldt()",
		"stodt(nulls())",

		/* Datetime parser test */
		"stodt('5/5/1980')",
		"stodt('1980-05-05')",
		"stodt('06:10:15')",
		"stodt('06:10')",
		"stodt('5/5/1980 06:10:15')",
		"stodt('1980-05-05 06:10:15')",
		"stodt('1980-05-05 06:10:15+02')",

		/* Legacy RLIB formats */
		"stodt('19800505061015')",
		"stodt('061015')",
		"stodt('061015p')",
		"stodt('06:10:15p')",
		"stodt('0610')",
		"stodt('0610p')",
		"stodt('06:10p')",

		/* Logic checks */
		"stodt('5/5/1980') < stodt('1980-05-05')",
		"stodt('5/5/1980') <= stodt('1980-05-05')",
		"stodt('5/5/1980') = stodt('1980-05-05')",
		"stodt('5/5/1980') != stodt('1980-05-05')",
		"stodt('5/5/1980') > stodt('1980-05-05')",
		"stodt('5/5/1980') >= stodt('1980-05-05')",

		"stodt('06:30:17') < stodt('06:30:17')",
		"stodt('06:30:17') <= stodt('06:30:17')",
		"stodt('06:30:17') = stodt('06:30:17')",
		"stodt('06:30:17') != stodt('06:30:17')",
		"stodt('06:30:17') > stodt('06:30:17')",
		"stodt('06:30:17') >= stodt('06:30:17')",

		"stodt('06:30') < stodt('06:30:00')",
		"stodt('06:30') <= stodt('06:30:00')",
		"stodt('06:30') = stodt('06:30:00')",
		"stodt('06:30') != stodt('06:30:00')",
		"stodt('06:30') > stodt('06:30:00')",
		"stodt('06:30') >= stodt('06:30:00')",
	};
	int nstr = sizeof(str) / sizeof(char *);
	int i;

	for (i = 0; i < nstr; i++) {
		char *err = NULL;

		printf("string: %s\n", str[i]);
		e = ocrpt_expr_parse(o, NULL, str[i], &err);
		if (e) {
			ocrpt_result *r;

			printf("expr reprinted: ");
			ocrpt_expr_print(o, e);
			printf("expr nodes: %d\n", ocrpt_expr_nodes(e));

			ocrpt_expr_optimize(o, NULL, e);
			printf("expr optimized: ");
			ocrpt_expr_print(o, e);
			printf("expr nodes: %d\n", ocrpt_expr_nodes(e));

			r = ocrpt_expr_eval(o, NULL, e);
			ocrpt_result_print(r);
		} else {
			printf("%s\n", err);
			ocrpt_strfree(err);
		}
		ocrpt_expr_free(o, NULL, e);
		printf("\n");
	}

	ocrpt_free(o);

	return 0;
}
