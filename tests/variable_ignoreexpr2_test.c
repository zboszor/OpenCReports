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

struct rowdata {
	ocrpt_query *q;
	ocrpt_expr *e;
	ocrpt_expr *e2;
	int32_t row;
};

static void test_newrow_cb(opencreport *o, ocrpt_report *r, void *ptr) {
	struct rowdata *rd = ptr;
	ocrpt_query_result *qr;
	ocrpt_result *rs;
	int32_t cols;

	qr = ocrpt_query_get_result(rd->q, &cols);

	printf("Row #%d\n", rd->row++);
	print_result_row("a", qr, cols);

	printf("\n");

	ocrpt_report_evaluate_variables(r);

	printf("Expression: ");
	ocrpt_expr_print(rd->e);
	rs = ocrpt_expr_eval(rd->e);
	printf("Evaluated: ");
	ocrpt_result_print(rs);

	printf("Expression: ");
	ocrpt_expr_print(rd->e2);
	rs = ocrpt_expr_eval(rd->e2);
	printf("Evaluated: ");
	ocrpt_result_print(rs);

	printf("\n");
}

int main(int argc, char **argv) {
	opencreport *o;
	ocrpt_datasource *ds;
	ocrpt_report *r;
	ocrpt_var *v, *v2;
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

	v = ocrpt_variable_new(r, OCRPT_VARIABLE_AVERAGE, "var1", "age", NULL, NULL, false);
	printf("Result expression for 'var1' reprinted: ");
	ocrpt_expr_print(ocrpt_variable_resultexpr(v));
	printf("\n");

	v2 = ocrpt_variable_new(r, OCRPT_VARIABLE_COUNT, "var2", "1", "abs(v.var1 - age) > 10", NULL, false);
	printf("Result expression for 'var2' reprinted: ");
	ocrpt_expr_print(ocrpt_variable_resultexpr(v2));
	printf("\n");

	rd.e = ocrpt_report_expr_parse(r, "v.var1", NULL);
	printf("Variable expression reprinted: ");
	ocrpt_expr_print(rd.e);
	printf("\n");

	rd.e2 = ocrpt_report_expr_parse(r, "v.var2", NULL);
	printf("Variable expression reprinted: ");
	ocrpt_expr_print(rd.e2);
	printf("\n");

	ocrpt_expr_resolve(rd.e);
	ocrpt_expr_optimize(rd.e);

	ocrpt_expr_resolve(rd.e2);
	ocrpt_expr_optimize(rd.e2);

	ocrpt_execute(o);

	ocrpt_expr_free(rd.e);
	ocrpt_expr_free(rd.e2);

	ocrpt_free(o);

	printf("============================================\n\n");

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

	v = ocrpt_variable_new(r, OCRPT_VARIABLE_AVERAGE, "var1", "age", NULL, NULL, true);

	printf("Result expression for 'var1' reprinted: ");
	ocrpt_expr_print(ocrpt_variable_resultexpr(v));
	printf("\n");

	v2 = ocrpt_variable_new(r, OCRPT_VARIABLE_COUNT, "var2", "1", "abs(v.var1 - age) > 10", NULL, false);
	printf("Result expression for 'var2' reprinted: ");
	ocrpt_expr_print(ocrpt_variable_resultexpr(v2));
	printf("\n");

	rd.e = ocrpt_report_expr_parse(r, "v.var1", NULL);
	printf("Variable expression reprinted: ");
	ocrpt_expr_print(rd.e);
	printf("\n");

	rd.e2 = ocrpt_report_expr_parse(r, "v.var2", NULL);
	printf("Variable expression reprinted: ");
	ocrpt_expr_print(rd.e2);
	printf("\n");

	ocrpt_expr_resolve(rd.e);
	ocrpt_expr_optimize(rd.e);

	ocrpt_expr_resolve(rd.e2);
	ocrpt_expr_optimize(rd.e2);

	ocrpt_execute(o);

	ocrpt_expr_free(rd.e);
	ocrpt_expr_free(rd.e2);

	ocrpt_free(o);

	return 0;
}
