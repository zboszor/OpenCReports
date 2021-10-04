/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

/* NOT static */ const char *array[4][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

/* NOT static */ const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_report *r;
	ocrpt_break *br;
	int32_t row, cols;

	if (!ocrpt_parse_xml(o, "ocrpt_break_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	q = ocrpt_query_find(o, "a");
	qr = ocrpt_query_get_result(q, &cols);

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	r = (ocrpt_report *)((ocrpt_list *)(((ocrpt_part *)(o->parts->data))->rows->data))->data;

	/* There is only one break in the report, extract it */
	br = (ocrpt_break *)r->breaks->data;

	row = 0;
	ocrpt_query_navigate_start(o, q);
	ocrpt_report_resolve_breaks(o, r);

	while (ocrpt_query_navigate_next(o, q)) {
		qr = ocrpt_query_get_result(q, &cols);

		if (ocrpt_break_check_fields(o, r, br))
			printf("Break triggers\n");

		printf("\n");

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");
	}

	ocrpt_free(o);

	return 0;
}
