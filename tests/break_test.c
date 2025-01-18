/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 3
#define COLS 5
static const char *array[ROWS + 1][COLS] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

static const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q UNUSED;
	ocrpt_report *r;
	ocrpt_break *br;
	ocrpt_expr *e;
	char *err;

	q = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);

	r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));

	br = ocrpt_break_new(r, "age");
	if (!br) {
		fprintf(stderr, "adding break failed\n");
		ocrpt_free(o);
		return 0;
	}

	err = NULL;
	e = ocrpt_report_expr_parse(r, "age > 18", &err);
	ocrpt_strfree(err);

	if (!ocrpt_break_add_breakfield(br, e)) {
		fprintf(stderr, "adding breakfield failed\n");
		ocrpt_free(o);
		return 0;
	}

	printf("adding a break and a breakfield to it succeeded\n");

	ocrpt_free(o);

	return 0;
}
