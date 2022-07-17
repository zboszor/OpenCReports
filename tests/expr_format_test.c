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
		/* Legacy string formats */

		"format('apples and oranges', '')",
		"format('apples and oranges', nulls())",

		"format('apples and oranges', '%s')",
		"format('apples and oranges', 'You have some %s')",
		"format('apples and oranges', 'You have some %s and pears')",
		"format('apples and oranges', 'You have some: %s')",
		"format('apples and oranges', '% 10s')",
		"format('apples and oranges', 'You have some % 10s pears')",
		"format('apples', '% 10s')",
		"format('árvíztűrő tükörfúrógép', '%9s')",
		"format('árvíztűrő tükörfúrógép', '% 9s')",
		"format('hülye', '% 9s')",

		/* Legacy number formats */

		"format(12345.56789, '%d')",
		"format(-12345.56789, '%d')",

		"format(12345.56789, '%6.3d')",
		"format(-12345.56789, '%6.3d')",

		"format(12345.56789, '%16.3d')",
		"format(-12345.56789, '%16.3d')",

		"format(12345.56789, '% 16.3d')",
		"format(-12345.56789, '% 16.3d')",

		"format(12345.56789, '%-16.3d')",
		"format(-12345.56789, '%-16.3d')",

		"format(6, 'You have % 10d oranges')",

		/* Legacy datetime formats */

		"format(stodt('2022-05-08'), '')", /* default formatting */
		"format(stodt('2022-05-08'), '%Y-%m-%d')",

		"dtosf(stodt('2022-05-08'), '')", /* default formatting */
		"dtosf(stodt('2022-05-08'), '%Y-%m-%d')",

		/* New style string formats */

		"format('apples and oranges', 'You have some !&% 10s pears')",
		"format('apples and oranges', 'You have some !&{% 10s} pears')",

		/* New style number formats */

		"format(12345.56789, '!#%16.3d')",
		"format(-12345.56789, '!#%16.3d')",

		"format(12345.56789, '!#{%16.3d}')",
		"format(-12345.56789, '!#{%16.3d}')",

		"format(6, 'You have !#% 10d oranges')",
		"format(6, 'You have !#{% 10d} oranges')",

		/* printf(), yay!!! */

		"printf('You had %d %s on !@{%Y-%m-%d} and %d %s on !@{%Y-%m-%d} in your pocket.', 6, 'apples', stodt('2022-05-01'), 2, 'oranges', stodt('2022-05-02'))",

		/*
		 * NULL data is treated as empty string,
		 * regardless of the optional width specifier.
		 * Literals from the format string are printed as is.
		 */
		"printf('You had %d %s on !@{%Y-%m-%d} and %d %s on !@{%Y-%m-%d} in your pocket.', nulln(), nulls(), nulldt(), nulln(), nulls(), nulldt())",

		/*
		 * This will throw a "format error".
		 * Data has a type first, then the data may be NULL.
		 */
		"printf('You had %d %s on !@{%Y-%m-%d} and %d %s on !@{%Y-%m-%d} in your pocket.', nulln(), nulls(), nulldt(), nulln(), nulls(), nulls())",

		/* Invalid formats */

		/* Numeric format for a string */
		"format('apples and oranges', '%6.3d')",

		/* String format for a number */
		"format(-12345.56789, '% 10s')",
	};
	int nstr = sizeof(str) / sizeof(char *);
	/* Locale aware formats */
	char *str1[] = {
		/* Legacy number format with legacy thousand separator flag */
		"format(12345.56789, '%$16.3d')",
		"format(-12345.56789, '%$16.3d')",

		/* Legacy number format with standard thousand separator flag */
		"format(12345.56789, '%''16.3d')",
		"format(-12345.56789, '%''16.3d')",

		/* New style number format */

		"format(12345.56789, '!#%''16.3d')",
		"format(-12345.56789, '!#%''16.3d')",

		"format(12345.56789, '!#{%''16.3d}')",
		"format(-12345.56789, '!#{%''16.3d}')",

		/* New style monetary format */

		"format(12345.56789, '!$%=*#60n')",
		"format(-12345.56789, '!$%=*#60n')",

		"format(12345.56789, '!${%=*#60n}')",
		"format(-12345.56789, '!${%=*#60n}')",

		/* Legacy datetime format */

		"format(stodt('2022-05-08'), '')",
		"format(stodt('2022-05-08'), '%A')",

		"dtosf(stodt('2022-05-08'), '')",
		"dtosf(stodt('2022-05-08'), '%A')",

		/* New style datetime format */

		"format(stodt('2022-05-08'), '!@%Y-%m-%d')",
		"format(stodt('2022-05-08'), '!@%A')",

		"format(stodt('2022-05-08'), '!@{%Y-%m-%d}')",
		"format(stodt('2022-05-08'), '!@{%A}')",

		"dtosf(stodt('2022-05-08'), '!@%Y-%m-%d')",
		"dtosf(stodt('2022-05-08'), '!@%A')",

		"dtosf(stodt('2022-05-08'), '!@{%Y-%m-%d}')",
		"dtosf(stodt('2022-05-08'), '!@{%A}')",
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

		printf("\nPrint formats in %s\n\n", locales[l]);

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
