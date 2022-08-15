/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

/* NOT static */ const char *array[7][4] = {
	{ "id", "name", "age", "male" },
	{ "1", "Fred Flintstone", "31", "yes" },
	{ "2", "Barney Rubble", "29", "yes" },
	{ "3", "Bamm-Bamm Rubble", "2", "yes" },
	{ "4", "Wilma Flintstone", "28", "no" },
	{ "5", "Betty Rubble", "27", "no" },
	{ "6", "Pebbles Flintstone", "5e-1", "no" },
};

/* NOT static */ const enum ocrpt_result_type coltypes[4] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

struct rowdata {
	ocrpt_query *q;
	ocrpt_expr *e;
};

static int32_t row = 0;

static void test_newrow_cb(opencreport *o, ocrpt_report *r, void *ptr) {
	struct rowdata *rd = ptr;
	int32_t cols;
	ocrpt_query_result *qr = ocrpt_query_get_result(rd->q, &cols);
	ocrpt_result *rs;

	if (row)
		printf("\n");
	printf("Row #%d\n", row++);
	print_result_row("a", qr, cols);

	rs = ocrpt_expr_get_result(o, rd->e);
	ocrpt_expr_print(o, rd->e);
	ocrpt_result_print(rs);
}

static void test_break_trigger_cb(opencreport *o, ocrpt_report *r, ocrpt_break *br, void *dummy UNUSED) {
	printf("break '%s' triggered\n", ocrpt_break_get_name(br));
}

int main(void) {
	opencreport *o = ocrpt_init();
	struct rowdata rd;
	ocrpt_break *br;

	if (!ocrpt_parse_xml(o, "break_multi2_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	rd.q = ocrpt_query_get(o, "a");

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	ocrpt_report *r = get_first_report(o);

	rd.e = ocrpt_expr_parse(o, r, "v.age_avg", NULL);

	ocrpt_report_add_new_row_cb(o, r, test_newrow_cb, &rd);

	br = ocrpt_break_get(o, r, "male");
	ocrpt_break_add_trigger_cb(o, r, br, test_break_trigger_cb, NULL);

	br = ocrpt_break_get(o, r, "adult");
	ocrpt_break_add_trigger_cb(o, r, br, test_break_trigger_cb, NULL);

	ocrpt_execute(o);

	ocrpt_expr_free(o, r, rd.e);

	ocrpt_free(o);

	return 0;
}
