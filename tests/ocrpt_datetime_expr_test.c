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

		/* Arithmetic tests */
		"stodt('1980-05-05') + 1",
		"stodt('1980-05-05') + 1 + 1",
		"1 + stodt('1980-05-05')",
		"1 + 1 + stodt('1980-05-05')",
		"1 + stodt('1980-05-05') + 1",

		"stodt('1980-05-05 06:00') + 1",
		"stodt('1980-05-05 06:00') + 1 + 1",
		"1 + stodt('1980-05-05 06:00')",
		"1 + 1 + stodt('1980-05-05 06:00')",
		"1 + stodt('1980-05-05 06:00') + 1",

		"stodt('06:00') + 1",
		"stodt('06:00') + 1 + 1",
		"1 + stodt('06:00')",
		"1 + 1 + stodt('06:00')",
		"1 + stodt('06:00') + 1",

		"stodt('1980-05-05') - 1",
		"stodt('1980-05-05 06:00') - 1",
		"stodt('06:00') - 1",

		"stodt('1980-05-05 23:59:59') + 1",
		"stodt('1980-05-06 00:00:00') - 1",

		/* Leap year tests */
		"stodt('1979-02-28') + 1",
		"stodt('1980-02-28') + 1",
		"stodt('1980-02-29') + 1",
		"stodt('2000-02-28') + 1",
		"stodt('2000-02-29') + 1",
		"stodt('2100-02-28') + 1",
		"stodt('2100-02-29') + 1", /* although invalid date, parsing fixes it as '2100-03-01' */

		"stodt('1979-03-01') - 1",
		"stodt('1980-03-01') - 1",
		"stodt('2000-03-01') - 1",
		"stodt('2100-03-01') - 1",

		/* Date part extraction tests */
		"year(stodt('1979-03-01'))",
		"month(stodt('1979-03-01'))",
		"day(stodt('1979-03-01'))",
		"dim(stodt('1979-02-15'))",
		"dim(stodt('1980-02-15'))",
		"dim(stodt('1980-03-15'))",
		"dim(stodt('1980-04-15'))",

		/* Invalid operand arithmetics */
		"stodt('1980-05-05') + stodt('1980-05-05')",
		"1 - stodt('1980-05-05 06:00')",
		"1 - stodt('06:00')",
	};
	int nstr = sizeof(str) / sizeof(char *);
	char *str1[] = {
		/* Parsed datetime, date part converted to string */
		"dtos(stodt('5/5/1980'))",
		"dtos(stodt('1980-05-05'))",
		"dtos(stodt('06:10:15'))",
		"dtos(stodt('06:10'))",
		"dtos(stodt('5/5/1980 06:10:15'))",
		"dtos(stodt('1980-05-05 06:10:15'))",
		"dtos(stodt('1980-05-05 06:10:15+02'))",
	};
	int nstr1 = sizeof(str1) / sizeof(char *);
	char *locales[] = {
		"C",
		"en_US.UTF-8",
		"en_GB.UTF-8",
		"fr_FR.UTF-8",
		"de_DE.UTF-8",
		"hu_HU.UTF-8",
	};
	int nlocs = sizeof(locales) / sizeof(char *);
	int i, l;

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

	for (l = 0; l < nlocs; l++) {
		o = ocrpt_init();
		ocrpt_set_locale(o, locales[l]);

		printf("\nPrint datetime date part in %s\n\n", locales[l]);

		for (i = 0; i < nstr1; i++) {
			char *err = NULL;

			printf("string: %s\n", str1[i]);
			e = ocrpt_expr_parse(o, NULL, str1[i], &err);
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
	}

	return 0;
}
