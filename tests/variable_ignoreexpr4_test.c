/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 9
#define COLS 4
const char *array[ROWS + 1][COLS] = {
	{ "id", "firstname", "lastname", "age" },
	{ "1", "Fred", "Flintstone", "31" },
	{ "2", "Wilma", "Flintstone", "28" },
	{ "3", "Pebbles", "Flintstone", "5e-1" },
	{ "4", "Betty", "Rubble", "27" },
	{ "5", "Barney", "Rubble", "29" },
	{ "6", "George", "Slate", "53" },
	{ "7", "Joe", "Rockhead", "33" },
	{ "8", "Sam", "Slagheap", "37" },
	{ "9", "The Great", "Gazoo", "1200" },
};

const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER
};

#define EXPRS 6

struct rowdata {
	ocrpt_query *q;
	ocrpt_expr *e[EXPRS];
	int32_t row;
};

static void test_newrow_cb(opencreport *o, ocrpt_report *r, void *ptr) {
	struct rowdata *rd = ptr;
	ocrpt_query_result *qr;
	int32_t cols, i;

	qr = ocrpt_query_get_result(rd->q, &cols);

	printf("Row #%d\n", rd->row++);
	print_result_row("a", qr, cols);

	printf("\n");

	for (i = 0; i < EXPRS; i++) {
		printf("Expression: ");
		ocrpt_expr_print(rd->e[i]);
		ocrpt_result *rs = ocrpt_expr_eval(rd->e[i]);
		printf("Evaluated: ");
		ocrpt_result_print(rs);
	}

	printf("\n");
}

int main(int argc, char **argv) {
	opencreport *o;
	ocrpt_datasource *ds;
	ocrpt_report *r;
	ocrpt_break *b;
	struct rowdata rd = { .row = 0 };

	o = ocrpt_init();
	ds = ocrpt_datasource_add(o, "array", "array", NULL);
	r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));

	rd.q = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);

	ocrpt_report_set_main_query(r, rd.q);

	if (!ocrpt_report_add_new_row_cb(r, test_newrow_cb, &rd)) {
		fprintf(stderr, "Failed to add new row callback.\n");
		ocrpt_free(o);
		return  1;
	}

	ocrpt_variable_new(r, OCRPT_VARIABLE_HIGHEST, "var1", "age", NULL, NULL, true);
	ocrpt_variable_new(r, OCRPT_VARIABLE_AVERAGE, "var2", "age", NULL, NULL, true);
	/*
	 * var3 is intentionally non-precalculated so
	 * an indirect reference to a precalculated variable
	 * is exercised.
	 */
	ocrpt_variable_new(r, OCRPT_VARIABLE_EXPRESSION, "var3", "0.9 * v.var1", NULL, NULL, false);
	ocrpt_variable_new(r, OCRPT_VARIABLE_AVERAGE, "var4", "age", "age > v.var3", "br1", true);
	ocrpt_variable_new(r, OCRPT_VARIABLE_AVERAGE, "var5", "age", "age > v.var3", NULL, true);

	rd.e[0] = ocrpt_report_expr_parse(r, "lastname", NULL);
	rd.e[1] = ocrpt_report_expr_parse(r, "v.var1", NULL);
	rd.e[2] = ocrpt_report_expr_parse(r, "v.var2", NULL);
	rd.e[3] = ocrpt_report_expr_parse(r, "v.var3", NULL);
	rd.e[4] = ocrpt_report_expr_parse(r, "v.var4", NULL);
	rd.e[5] = ocrpt_report_expr_parse(r, "v.var5", NULL);

	b = ocrpt_break_new(r, "br1");
	ocrpt_break_add_breakfield(b, rd.e[0]);

	ocrpt_execute(o);

	ocrpt_free(o);

	return 0;
}
