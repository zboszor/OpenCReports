/*
 * OpenCReports test
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_expr *e;

	char *str[] = {
		"|1|",
		"-1",

		/* Left-associative operator sequence */
		"1 + 2 + 3",
		"1 * 2 * 3",
		"1 * 2 / 3",
		"1 / 2 * 3",

		/* Operator precedence without parenthesis */
		"1 + 2 * 3",
		"1 - 2 * 3",
		"-1 + -2 * -3",
		"-1 - -2 * -3",

		/* Operator precedence with parenthesis */
		"(1 + 2) * 3",
		"(1 - 2) * 3",
		"(-1 + -2) * -3",
		"(-1 - -2) * -3",

		/* Increment/decrement */
		"1++",
		"++1",
		"1--",
		"--1",

		/* Identical operators (optimizer test) */
		"6 / 3 / 2 ",	/* 1 */
		"(6 / 3) / 2",	/* 1 */
		"6 / (3 / 2)",	/* 4 */
		"a / b / c",	/* can be same level of a single div() call */
		"(a / b) / c",	/* can be same level of a single div() call */
		"a / (b / c)",	/* b/c must stay as one subexpression */

		/* Facebook challange with implicit multiplication */
		"1/(1+1)(2+2)",
		"1/(1+1)*(2+2)",

		/* Misc. expressions */
		"(2+2).2",
		"1/2x",
		"1/2e",
		"2e",
		"add(1, 2, 3, 4, 5, (6))",
		"add(a, 1, b, 2)",
		"a + 1 + b + 2",

		/* String comparison expressions */
		"'a' = 'a'",
		"'a' = 'b'",
		"'a' <> 'a'",
		"'a' <> 'b'",
		"'a' < 'a'",
		"'a' < 'b'",
		"'a' <= 'a'",
		"'a' <= 'b'",
		"'a' > 'a'",
		"'a' > 'b'",
		"'a' >= 'a'",
		"'a' >= 'b'",

		/* String arithmetics */
		"'a' + 'b'",			/* should be 'ab' */
		"concat('a', 'b')",		/* same */
		"'a' - 'b'",			/* should be an error */

		/* val() conversions */
		"val('1.5')",
		"val(1.5)",
		"val('yes')",
		"val(yes)",

		/* NULL tests */
		"null(1)",
		"null('a')",
		"nulldt()",
		"nulln()",
		"nulls()",

		/* Error tests */
		"error('custom error')",
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
			ocrpt_expr_result_print(r);
		} else {
			printf("%s\n", err);
			ocrpt_strfree(err);
		}
		ocrpt_free_expr(e);
		printf("\n");
	}

	ocrpt_free(o);

	return 0;
}
