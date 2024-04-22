/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 8
#define COLS 1
static const char *array[ROWS + 1][COLS] = {
	{ "text" },
	{ "bad-tempered" },
	{ "old" },
	{ "ladies" },
	{ "love" },
	{ "our" },
	{ "chic" },
	{ "kitchen" },
	{ "sink" },
};

static const int32_t coltypes[COLS] = { OCRPT_RESULT_STRING };

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_report *r;
	ocrpt_expr *e;
	ocrpt_var *v;
	char *err;
	int32_t cols, row;

	r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));

	q = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);
	qr = ocrpt_query_get_result(q, &cols);

	v = ocrpt_variable_new_full(r, OCRPT_RESULT_STRING, "var1", "upper(left(a.text, 1))", NULL, NULL, "r.self + r.baseexpr", NULL);

	printf("Base expression for 'var1' reprinted: ");
	ocrpt_expr_print(ocrpt_variable_baseexpr(v));
	printf("\n");

	printf("Result expression for 'var1' reprinted: ");
	ocrpt_expr_print(ocrpt_variable_resultexpr(v));
	printf("\n");

	row = 0;
	ocrpt_query_navigate_start(q);
	printf("ocrpt_query_navigate_start done\n");
	ocrpt_report_resolve_variables(r);
	printf("ocrpt_report_resolve_variables done\n");

	err = NULL;
	e = ocrpt_report_expr_parse(r, "v.var1", &err);
	ocrpt_strfree(err);
	printf("Variable expression reprinted: ");
	ocrpt_expr_print(e);
	printf("\n");

	ocrpt_expr_resolve(e);

	while (ocrpt_query_navigate_next(q)) {
		ocrpt_result *rs;

		qr = ocrpt_query_get_result(q, &cols);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		ocrpt_report_evaluate_variables(r);

		printf("Expression: ");
		ocrpt_expr_print(e);
		rs = ocrpt_expr_eval(e);
		printf("Evaluated: ");
		ocrpt_result_print(rs);

		printf("\n");
	}

	ocrpt_expr_free(e);

	ocrpt_free(o);

	return 0;

}
