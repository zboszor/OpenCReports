/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

static const char *array[9][1] = {
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

static const enum ocrpt_result_type coltypes[1] = { OCRPT_RESULT_STRING };

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_report *r;
	ocrpt_expr *e;
	ocrpt_var *v;
	char *err;
	int32_t cols, row;

	r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));

	q = ocrpt_query_add_array(ds, "a", (const char **)array, 8, 1, coltypes);
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
