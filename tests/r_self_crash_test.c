/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <opencreport.h>
#include "test_common.h"

#define COLS 5
const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};


static char *myexprs[] = {
	"|r.self|",
	"-r.self",

	"r.self + r.self",
	"r.self + 1",
	"1 + r.self ",
	"r.self + 'a'",
	"'a' + r.self",
	"r.self + interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) + r.self",

	"r.self - r.self",
	"r.self - 1",
	"1 - r.self",
	"r.self - 'a'",
	"'a' - r.self",
	"r.self - interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) - r.self",

	"r.self * r.self",
	"r.self * 1",
	"1 * r.self",
	"r.self * 'a'",
	"'a' * r.self",
	"r.self * interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) * r.self",

	"r.self / r.self",
	"r.self / 1",
	"1 / r.self",
	"r.self / 'a'",
	"'a' / r.self",
	"r.self / interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) / r.self",

	"r.self % r.self",
	"r.self % 1",
	"1 % r.self",
	"r.self % 'a'",
	"'a' % r.self",
	"r.self % interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) % r.self",

	"r.self ^ r.self",
	"r.self ^ 1",
	"1 ^ r.self",
	"r.self ^ 'a'",
	"'a' ^ r.self",
	"r.self ^ interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) ^ r.self",

	"r.self = r.self",
	"r.self = 1",
	"1 = r.self",
	"r.self = 'a'",
	"'a' = r.self",
	"r.self = interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) = r.self",

	"r.self != r.self",
	"r.self != 1",
	"1 != r.self",
	"r.self != 'a'",
	"'a' != r.self",
	"r.self != interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) != r.self",

	"r.self < r.self",
	"r.self < 1",
	"1 < r.self",
	"r.self < 'a'",
	"'a' < r.self",
	"r.self < interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) < r.self",

	"r.self <= r.self",
	"r.self <= 1",
	"1 <= r.self",
	"r.self <= 'a'",
	"'a' <= r.self",
	"r.self <= interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) <= r.self",

	"r.self > r.self",
	"r.self > 1",
	"1 > r.self",
	"r.self > 'a'",
	"'a' > r.self",
	"r.self > interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) > r.self",

	"r.self >= r.self",
	"r.self >= 1",
	"1 >= r.self",
	"r.self >= 'a'",
	"'a' >= r.self",
	"r.self >= interval(0,0,0,0,0,1)",
	"interval(0,0,0,0,0,1) >= r.self",

	"val(r.self)",
	"isnull(r.self)",
	"null(r.self)",
	"r.self ? r.self : r.self",

	"r.self++",
	"++r.self",
	"r.self--",
	"--r.self",

	"error(r.self)",

	"concat(r.self, r.self, r.self)",

	"left(r.self, r.self)",
	"right(r.self, r.self)",
	"mid(r.self, r.self, r.self)",

	"r.self!",

	"r.self && r.self",
	"r.self || r.self",
	"!r.self",

	"r.self & r.self",
	"r.self | r.self",
	"~r.self",
	"xor(r.self, r.self)",
	"r.self << r.self",
	"r.self >> r.self",

	"fmod(r.self, r.self)",
	"remainder(r.self, r.self)",
	"rint(r.self)",
	"ceil(r.self)",
	"floor(r.self)",
	"round(r.self)",
	"trunc(r.self)",
	"pow(r.self, r.self)",

	"lower(r.self)",
	"upper(r.self)",
	"proper(r.self)",

	"rownum()",
	"rownum(r.self)",
	"brrownum(r.self)",

	"stodt(r.self)",
	"dtos(r.self)",
	"year(r.self)",
	"month(r.self)",
	"day(r.self)",
	"dim(r.self)",
	"wiy(r.self)",
	"wiy1(r.self)",
	"wiyo(r.self, r.self)",
	"stdwiy(r.self)",
	"dateof(r.self)",
	"timeof(r.self)",
	"chgdateof(r.self, r.self)",
	"chgtimeof(r.self, r.self)",
	"gettimeinsecs(r.self)",
	"settimeinsecs(r.self, r.self)",
	"interval(r.self)",
	"interval(r.self, r.self, r.self, r.self, r.self, r.self)",

	"cos(r.self)",
	"sin(r.self)",
	"tan(r.self)",
	"acos(r.self)",
	"asin(r.self)",
	"atan(r.self)",
	"sec(r.self)",
	"csc(r.self)",
	"cot(r.self)",

	"log(r.self)",
	"log2(r.self)",
	"log10(r.self)",
	"exp(r.self)",
	"exp2(r.self)",
	"exp10(r.self)",
	"sqr(r.self)",
	"sqrt(r.self)",

	"fxpval(r.self, r.self)",

	"str(r.self, r.self, r.self)",
	"strlen(r.self)",
	"format(r.self, r.self)",

	"dtosf(r.self, r.self)",

	"printf(r.self, r.self, r.self, r.self, r.self)",

	"translate(r.self)",
	"translate2(r.self, r.self, r.self)",

	"prevval(r.self)",

	NULL
};

#define MYEXPRS (sizeof(myexprs) / sizeof(char *))

struct rowdata {
	ocrpt_expr *expr[MYEXPRS];
};

static void test_newrow_cb(opencreport *o, ocrpt_report *r, void *ptr) {
	struct rowdata *rd = ptr;
	ocrpt_result *rs;

	for (int32_t i = 0; i < MYEXPRS && myexprs[i]; i++) {
		printf("expr%d:\n", i);
		rs = ocrpt_expr_get_result(rd->expr[i]);
		ocrpt_result_print(rs);
	}

	printf("\n");
}

static ocrpt_report *setup_report(opencreport *o, void *ptr) {
	if (!ocrpt_parse_xml(o, "csvquery.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		exit(0);
	}

	ocrpt_query *q = ocrpt_query_get(o, "a");
	ocrpt_report *r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));
	ocrpt_report_set_main_query(r, q);

	if (!ocrpt_report_add_new_row_cb(r, test_newrow_cb, ptr)) {
		fprintf(stderr, "Failed to add new row callback.\n");
		ocrpt_free(o);
		exit(1);
	}

	return r;
}

int main(int argc, char **argv) {
	struct rowdata rd;
	opencreport *o;
	ocrpt_report *r;

	o = ocrpt_init();
	r = setup_report(o, &rd);

	for (int32_t i = 0; i < MYEXPRS && myexprs[i]; i++) {
		char *err = NULL;
		rd.expr[i] = ocrpt_report_expr_parse(r, myexprs[i], &err);

		if (!rd.expr[i])
			fprintf(stderr, "expr: %s error: %s\n", myexprs[i], err);
		ocrpt_strfree(err);
	}

	ocrpt_execute(o);

	ocrpt_free(o);

	return 0;
}
