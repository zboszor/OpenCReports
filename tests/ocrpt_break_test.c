/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

static const char *array[4][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

static const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *q UNUSED;
	ocrpt_report *r;
	ocrpt_break *br;
	ocrpt_expr *e;
	char *err;

	q = ocrpt_query_add_array(o, ds, "a", (const char **)array, 3, 5, coltypes);

	r = ocrpt_report_new(o);
	ocrpt_part_append_report(o, NULL, NULL, NULL, r);

	br = ocrpt_break_new(o, r, "age");
	if (!br) {
		fprintf(stderr, "adding break failed\n");
		ocrpt_free(o);
		return 0;
	}

	err = NULL;
	e = ocrpt_expr_parse(o, r, "age > 18", &err);
	ocrpt_strfree(err);

	if (!ocrpt_break_add_breakfield(o, r, br, e)) {
		fprintf(stderr, "adding breakfield failed\n");
		ocrpt_free(o);
		return 0;
	}

	printf("adding a break and a breakfield to it succeeded\n");

	ocrpt_free(o);

	return 0;
}
