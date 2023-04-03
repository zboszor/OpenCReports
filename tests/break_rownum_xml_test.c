/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

/* NOT static */ const char *array[6][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" },
	{ "4", "Betty Rubble", "beautiful", "27", "yes" },
	{ "5", "Barney Rubble", "small", "29", "yes" }
};

/* NOT static */ const int32_t coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_expr *e;
	int32_t row, cols;

	if (!ocrpt_parse_xml(o, "break_xml3_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	q = ocrpt_query_get(o, "a");
	qr = ocrpt_query_get_result(q, &cols);

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	ocrpt_report *r = get_first_report(o);

	/* There is only one break in the report, extract it */
	ocrpt_list *brl = NULL;
	ocrpt_break *br = ocrpt_break_get_next(r, &brl);

	e = ocrpt_report_expr_parse(r, "brrownum('adult')", NULL);

	row = 0;
	ocrpt_query_navigate_start(q);
	ocrpt_report_resolve_breaks(r);

	while (ocrpt_query_navigate_next(q)) {
		ocrpt_result *rs;

		qr = ocrpt_query_get_result(q, &cols);

		if (ocrpt_break_check_fields(br))
			printf("Break triggers\n");

		printf("\n");

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

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
