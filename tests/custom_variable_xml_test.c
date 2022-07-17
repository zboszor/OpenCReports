/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

/* NOT static */ const char *array[9][1] = {
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

/* NOT static */ const enum ocrpt_result_type coltypes[1] = { OCRPT_RESULT_STRING };

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_expr *e;
	char *err;
	int32_t cols, row;

	if (!ocrpt_parse_xml(o, "custom_variable_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	q = ocrpt_query_get(o, "a");
	qr = ocrpt_query_get_result(q, &cols);

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	ocrpt_part *p = (ocrpt_part *)o->parts->data;
	ocrpt_part_row *pr = (ocrpt_part_row *)p->rows->data;
	ocrpt_part_row_data *pd = (ocrpt_part_row_data *)pr->pd_list->data;
	ocrpt_report *r = (ocrpt_report *)pd->reports->data;

	err = NULL;
	e = ocrpt_expr_parse(o, r, "v.var1", &err);
	ocrpt_strfree(err);
	printf("Variable expression reprinted: ");
	ocrpt_expr_print(o, e);
	printf("\n");

	ocrpt_expr_resolve(o, r, e);

	row = 0;
	ocrpt_query_navigate_start(o, q);
	ocrpt_report_resolve_variables(o, r);

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_result *rs;

		qr = ocrpt_query_get_result(q, &cols);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		ocrpt_report_evaluate_variables(o, r);

		printf("Expression: ");
		ocrpt_expr_print(o, e);
		rs = ocrpt_expr_eval(o, r, e);
		printf("Evaluated: ");
		ocrpt_result_print(rs);

		printf("\n");
	}

	ocrpt_expr_free(o, r, e);

	ocrpt_free(o);

	return 0;

}
