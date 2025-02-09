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

		/* Legacy RLIB formats */
		"stodt('19800505061015')",
		"stodt('061015')",
		"stodt('061015p')",
		"stodt('06:10:15p')",
		"stodt('0610')",
		"stodt('0610p')",
		"stodt('06:10p')",

		/* Interval function */
		"interval(1,2,3,4,5,6)",
		"interval('1 years 2 months 3 days 4 hours 5 minutes 6 seconds')",
		"interval('1 yrs 2 mons 3 days 4 hrs 5 mins 6 secs')",
		"interval('1 y 2 mo 3 d 4 h 5 m 6 s')",
		"interval('1y 2mo 3d 4h 5m 6s')",

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
		"stodt('1980-05-05')++",
		"stodt('1980-05-05') + 1 + 1",
		"stodt('1980-05-05')++++",
		"1 + stodt('1980-05-05')",
		"++stodt('1980-05-05')",
		"1 + 1 + stodt('1980-05-05')",
		"++++stodt('1980-05-05')",
		"1 + stodt('1980-05-05') + 1",
		"++stodt('1980-05-05')++",

		"stodt('1980-05-05 06:00') + 1",
		"stodt('1980-05-05 06:00')++",
		"stodt('1980-05-05 06:00') + 1 + 1",
		"stodt('1980-05-05 06:00')++++",
		"1 + stodt('1980-05-05 06:00')",
		"++stodt('1980-05-05 06:00')",
		"1 + 1 + stodt('1980-05-05 06:00')",
		"++++stodt('1980-05-05 06:00')",
		"1 + stodt('1980-05-05 06:00') + 1",
		"++stodt('1980-05-05 06:00')++",

		"stodt('06:00') + 1",
		"stodt('06:00')++",
		"stodt('06:00') + 1 + 1",
		"stodt('06:00')++++",
		"1 + stodt('06:00')",
		"++stodt('06:00')",
		"1 + 1 + stodt('06:00')",
		"++++stodt('06:00')",
		"1 + stodt('06:00') + 1",
		"++stodt('06:00')++",

		"stodt('1980-05-05') - 1",
		"stodt('1980-05-05')--",
		"stodt('1980-05-05 06:00') - 1",
		"stodt('1980-05-05 06:00')--",
		"stodt('06:00') - 1",
		"stodt('06:00')--",

		"stodt('1980-05-05 23:59:59') + 1",
		"stodt('1980-05-05 23:59:59')++",
		"stodt('1980-05-06 00:00:00') - 1",
		"stodt('1980-05-06 00:00:00')--",

		"stodt('1980-01-31 23:59:59') + interval(0,0,0,0,0,1)",
		"stodt('1980-02-01 00:00:00') - interval(0,0,0,0,0,1)",
		"stodt('1980-01-31 23:59:59') + interval(0,0,0,0,1,0)",
		"stodt('1980-02-01 00:00:00') - interval(0,0,0,0,1,0)",
		"stodt('1980-01-31 23:59:59') + interval(0,0,0,1,0,0)",
		"stodt('1980-02-01 00:00:00') - interval(0,0,0,1,0,0)",
		"stodt('1980-01-31 23:59:59') + interval(0,0,1,0,0,0)",
		"stodt('1980-02-01 00:00:00') - interval(0,0,1,0,0,0)",
		"stodt('1980-01-31 23:59:59') + interval(0,1,0,0,0,0)",
		"stodt('1980-02-01 00:00:00') - interval(0,1,0,0,0,0)",
		"stodt('1980-01-31 23:59:59') + interval(1,0,0,0,0,0)",
		"stodt('1980-02-01 00:00:00') - interval(1,0,0,0,0,0)",

		"stodt('1980-01-31') + interval(0,0,0,0,0,1)",
		"stodt('1980-02-01') - interval(0,0,0,0,0,1)",
		"stodt('1980-01-31') + interval(0,0,0,0,1,0)",
		"stodt('1980-02-01') - interval(0,0,0,0,1,0)",
		"stodt('1980-01-31') + interval(0,0,0,1,0,0)",
		"stodt('1980-02-01') - interval(0,0,0,1,0,0)",
		"stodt('1980-01-31') + interval(0,0,1,0,0,0)",
		"stodt('1980-02-01') - interval(0,0,1,0,0,0)",
		"stodt('1980-01-31') + interval(0,1,0,0,0,0)",
		"stodt('1980-02-01') - interval(0,1,0,0,0,0)",
		"stodt('1980-01-31') + interval(1,0,0,0,0,0)",
		"stodt('1980-02-01') - interval(1,0,0,0,0,0)",

		"interval(1,0,0,0,0,0) + 1",
		"interval(1,0,0,0,0,0)++",
		"1 + interval(1,0,0,0,0,0)",
		"++interval(1,0,0,0,0,0)",
		"interval(1,0,0,0,0,0) - 1",
		"interval(1,0,0,0,0,0)--",
		"1 - interval(1,0,0,0,0,0)",
		"--interval(1,0,0,0,0,0)",

		"interval(1,0,0,0,0,0) + interval(0,1,0,0,0,0)",
		"interval(1,0,0,0,0,0) - interval(0,1,0,0,0,0)",

		/* Datetime - datetime */
		"stodt('1980-03-31') - stodt('1980-01-15')",
		"stodt('1980-03-31') - stodt('1980-01-15 12:00:00')",
		"stodt('1980-03-31 12:00:00') - stodt('1980-01-15')",
		"stodt('1980-03-31 12:00:00') - stodt('1980-01-15 12:00:00')",

		/* Datetime - datetime, resulting in negative interval */
		"stodt('1980-01-15') - stodt('1980-03-31')",
		"stodt('1980-01-15') - stodt('1980-03-31 12:00:00')",
		"stodt('1980-01-15 12:00:00') - stodt('1980-03-31')",
		"stodt('1980-01-15 12:00:00') - stodt('1980-03-31 12:00:00')",

		/* Transitivity proof */
		"stodt('1979-01-31') + interval(0,1,0,0,0,0)",
		"stodt('1979-01-31') + interval(0,1,0,0,0,0) + interval(0,1,0,0,0,0)",
		"stodt('1979-01-31') + interval(0,2,0,0,0,0)",
		"stodt('1979-03-31') - interval(0,1,0,0,0,0)",
		"stodt('1979-03-31') - interval(0,1,0,0,0,0) - interval(0,1,0,0,0,0)",
		"stodt('1979-03-31') - interval(0,2,0,0,0,0)",
		"stodt('1980-01-31') + interval(0,1,0,0,0,0)",
		"stodt('1980-01-31') + interval(0,1,0,0,0,0) + interval(0,1,0,0,0,0)",
		"stodt('1980-01-31') + interval(0,2,0,0,0,0)",
		"stodt('1980-03-31') - interval(0,1,0,0,0,0)",
		"stodt('1980-03-31') - interval(0,1,0,0,0,0) - interval(0,1,0,0,0,0)",
		"stodt('1980-03-31') - interval(0,2,0,0,0,0)",

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

		/* Week-of-year tests */
		"wiy(stodt('2022-01-01'))",
		"wiy1(stodt('2022-01-01'))",
		"wiyo(stodt('2022-01-01'), 1)",
		"stdwiy(stodt('2022-01-01'))",
		"wiy(stodt('2022-01-02'))",
		"wiy1(stodt('2022-01-02'))",
		"wiyo(stodt('2022-01-02'), 1)",
		"stdwiy(stodt('2022-01-02'))",
		"wiy(stodt('2022-01-03'))",
		"wiy1(stodt('2022-01-03'))",
		"wiyo(stodt('2022-01-03'), 1)",
		"stdwiy(stodt('2022-01-03'))",

		/* Modification tests */
		"dateof(stodt('1979-03-01'))",
		"dateof(stodt('1979-03-01 06:15:37'))",
		"dateof(stodt('06:15:37'))",
		"timeof(stodt('1979-03-01'))",
		"timeof(stodt('1979-03-01 06:15:37'))",
		"timeof(stodt('06:15:37'))",

		"chgdateof(stodt('1979-03-01'), stodt('2022-04-21'))",
		"chgdateof(stodt('1979-03-01 06:30:21'), stodt('2022-04-21'))",
		"chgdateof(stodt('06:30:21'), stodt('2022-04-21'))",
		"chgdateof(stodt('1979-03-01'), stodt('2022-04-21 14:37:12'))",
		"chgdateof(stodt('1979-03-01 06:30:21'), stodt('2022-04-21 14:37:12'))",
		"chgdateof(stodt('06:30:21'), stodt('2022-04-21 14:37:12'))",
		"chgdateof(stodt('1979-03-01'), stodt('14:37:12'))",
		"chgdateof(stodt('1979-03-01 06:30:21'), stodt('14:37:12'))",
		"chgdateof(stodt('06:30:21'), stodt('14:37:12'))",

		"chgtimeof(stodt('1979-03-01'), stodt('2022-04-21'))",
		"chgtimeof(stodt('1979-03-01 06:30:21'), stodt('2022-04-21'))",
		"chgtimeof(stodt('06:30:21'), stodt('2022-04-21'))",
		"chgtimeof(stodt('1979-03-01'), stodt('2022-04-21 14:37:12'))",
		"chgtimeof(stodt('1979-03-01 06:30:21'), stodt('2022-04-21 14:37:12'))",
		"chgtimeof(stodt('06:30:21'), stodt('2022-04-21 14:37:12'))",
		"chgtimeof(stodt('1979-03-01'), stodt('14:37:12'))",
		"chgtimeof(stodt('1979-03-01 06:30:21'), stodt('14:37:12'))",
		"chgtimeof(stodt('06:30:21'), stodt('14:37:12'))",

		"settimeinsecs(stodt('1979-03-01'), 3600)",
		"settimeinsecs(stodt('1979-03-01 06:30:21'), 3600)",
		"settimeinsecs(stodt('06:30:21'), 3600)",
		"settimeinsecs(nulldt(), 3600)",
		"settimeinsecs(stodt('1979-03-01'), nulln())",

		"gettimeinsecs(stodt('1979-03-01'))",
		"gettimeinsecs(stodt('1979-03-01 06:30:21'))",
		"gettimeinsecs(stodt('06:30:21'))",
		"gettimeinsecs(nulldt())",

		/* Invalid operand arithmetics */
		"stodt('1980-05-05') + stodt('1980-05-05')",
		"1 - stodt('1980-05-05 06:00')",
		"1 - stodt('06:00')",

		/* Invalid operand function calls */
		"wiy(stodt('2022-01-01'), 1)",
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

	for (l = 0; l < nlocs; l++) {
		o = ocrpt_init();
		ocrpt_set_locale(o, locales[l]);

		printf("\nPrint datetime date part in %s\n\n", locales[l]);

		for (i = 0; i < nstr1; i++) {
			char *err = NULL;

			printf("string: %s\n", str1[i]);
			e = ocrpt_expr_parse(o, str1[i], &err);
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
	}

	return 0;
}
