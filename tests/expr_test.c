/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_expr *e;

	char *str[] = {
		/* Various numeric functions */
		"|1|",
		"|nulln()|",
		"-1",
		"-nulln()",
		"factorial(nulln())",
		"factorial(-1)",
		"factorial(0)",
		"factorial(1)",
		"factorial(3)",
		"3!",
		"!3!",
		"5 % 3",
		"4.9 % 2.9",
		"4.9 % 3.1",
		"fmod(5, 3)",
		"fmod(4.9, 2.9)",
		"fmod(4.9, 3.1)",
		"remainder(5, 3)",
		"remainder(4.9, 2.9)",
		"remainder(4.9, 3.1)",

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

		/* Unary operators */
		"+1",
		"-1",
		"1 + +1",
		"1 + -1",

		/* Increment/decrement */
		"1++",
		"++1",
		"1--",
		"--1",
		"a+++++b",
		"a-++++b",
		"a--+++b",
		"a---++b",
		"a----+b",
		"a-----b",
		"a++++-b",
		"a+++--b",
		"a++---b",
		"a+----b",
		"a+--++b",
		"a++-++b",

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

		/* Logical operators */
		"0 && 0",
		"0 && 1",
		"1 && 0",
		"1 && 1",
		"0 && 0.5",
		"0.5 && 0.5",
		"0 && 0.4",
		"0.4 && 0.4",

		"0 || 0",
		"0 || 1",
		"1 || 0",
		"1 || 1",
		"0 || 0.5",
		"0.5 || 0.5",
		"0 || 0.4",
		"0.4 || 0.4",

		"!0",
		"!1",
		"!0.4",

		/* Binary operators */
		"1 | 2",
		"1 & 2",
		"3 & 2",
		"1 ^ 2",
		"3 ^ 2",
		"~0",
		"~1",
		"1 << 2",
		"12 >> 2",

		/* String arithmetics tests */
		"'a' + 'b'",			/* should be 'ab' */
		"'a' + nulls()",		/* should be NULL */
		"concat('a', 'b')",		/* same */
		"'a' - 'b'",			/* should be an error */
		"left('árvíztűrő tükörfúrógép', 9)",
		"mid('árvíztűrő tükörfúrógép', 0, 9)",	/* start index 0 and 1 are the same */
		"mid('árvíztűrő tükörfúrógép', 1, 9)",
		"mid('árvíztűrő tükörfúrógép', 6, 10)",
		"mid('árvíztűrő tükörfúrógép', -12, 9)",
		"right('árvíztűrő tükörfúrógép', 7)",
		"lower('ÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP')",
		"upper('árvíztűrő tükörfúrógép')",
		"proper('ÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP')",
		"proper('árvíztűrő tükörfúrógép')",
		"strlen('árvíztűrő tükörfúrógép')",

		/* String arithmetics NULL tests */
		"left('a', nulln())",
		"left(nulls(), 1)",
		"left(nulls(), nulln())",
		"right('a', nulln())",
		"right(nulls(), 1)",
		"right(nulls(), nulln())",
		"strlen(nulls())",

		/* val() conversions */
		"val('1.5')",
		"val(1.5)",
		"val('yes')",
		"val(yes)",

		/* fxpval() conversion */
		"fxpval('123456', 2)",
		"fxpval(val('123456'), 2)",

		/* str() conversion */
		"str(1234.56, 6, 4)",
		"str(1234.56, 10, 4)",

		/* NULL tests */
		"null(1)",
		"null('a')",
		"nulldt()",
		"nulln()",
		"nulls()",

		/* Embedded expression string evaluation tests */
		"eval('1+2')",
		"eval('1+2') + 3",
		"3 + eval('1+2')",
		"eval('1+2') * 3",
		"3 * eval('1+2')",
		"eval('upper(\"árvíztűrő tükörfúrógép\")')",
		"eval('identifier')",
		"eval('1 1')", /* intentional syntax error */

		"eval ('1+2')",
		"eval ('1+2') + 3",
		"3 + eval ('1+2')",
		"eval ('1+2') * 3",
		"3 * eval ('1+2')",
		"eval ('upper(\"árvíztűrő tükörfúrógép\")')",
		"eval ('identifier')",
		"eval ('1 1')", /* intentional syntax error */

		/* Vector matching tests */
		"[] = [1]", /* intentional syntax error */
		"[1] = []", /* intentional syntax error */
		"[1,2] = [1]", /* intentional syntax error */
		"[1] = [1,2]", /* intentional syntax error */
		"[1] = [1]",
		"[1,2] = [1,2]", /* true */
		"[1,2] = [1,3]", /* false */
		"[1,2] != [1,3]", /* false */
		"[1,2] <= [1,3]", /* true */

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
			ocrpt_expr_print(e);
			printf("expr nodes: %d\n", ocrpt_expr_nodes(e));

			ocrpt_expr_optimize(e);
			printf("expr optimized: ");
			ocrpt_expr_print(e);
			printf("expr nodes: %d\n", ocrpt_expr_nodes(e));

			r = ocrpt_expr_eval(e);
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
