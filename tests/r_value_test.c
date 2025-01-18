/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <opencreport.h>
#include "test_common.h"

#define COLS 5
const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

struct rowdata {
	ocrpt_expr *expr;
	ocrpt_expr *rval1;
};

static void test_newrow_cb(opencreport *o, ocrpt_report *r, void *ptr) {
	struct rowdata *rd = ptr;
	ocrpt_result *rs;

	printf("expr:\n");
	rs = ocrpt_expr_get_result(rd->expr);
	ocrpt_result_print(rs);

	printf("r.value + 1:\n");
	rs = ocrpt_expr_get_result(rd->rval1);
	ocrpt_result_print(rs);

	printf("\n");
}

static ocrpt_report *setup_report(opencreport *o, void *ptr) {
	if (!ocrpt_parse_xml(o, "csvquery.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		exit(0);
	}

	ocrpt_query *q = ocrpt_query_get(o, "a");
	ocrpt_report *r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));
	ocrpt_report_set_main_query(r, q);

	if (!ocrpt_report_add_new_row_cb(r, test_newrow_cb, ptr)) {
		fprintf(stderr, "Failed to add new row callback.\n");
		ocrpt_free(o);
		exit(1);
	}

	return r;
}

int main(int argc, char **argv) {
	struct rowdata rd;
	opencreport *o;
	ocrpt_report *r;

	printf("Normal expression creation and evaluation order:\n");

	o = ocrpt_init();
	r = setup_report(o, &rd);

	rd.expr = ocrpt_report_expr_parse(r, "id", NULL);
	rd.rval1 = ocrpt_report_expr_parse(r, "r.value + 1", NULL);

	ocrpt_expr_set_field_expr(rd.rval1, rd.expr);

	ocrpt_execute(o);

	ocrpt_free(o);

	printf("Bad expression creation and evaluation order:\n");

	o = ocrpt_init();
	r = setup_report(o, &rd);

	/*
	 * "rval1" is created earlier than "expr".
	 * Since the order of expression evaluation is the same
	 * as the order of expression creation, this is bad.
	 *
	 * For simple expressions using plain data set columns,
	 * it's not a problem in practice.
	 */
	rd.rval1 = ocrpt_report_expr_parse(r, "r.value + 1", NULL);
	rd.expr = ocrpt_report_expr_parse(r, "id", NULL);

	ocrpt_expr_set_field_expr(rd.rval1, rd.expr);

	ocrpt_execute(o);

	ocrpt_free(o);

	printf("Good expression creation and evaluation order:\n");

	o = ocrpt_init();
	r = setup_report(o, &rd);

	rd.expr = ocrpt_report_expr_parse(r, "isnull(r.self) ? 1 : r.self + 1", NULL);
	rd.rval1 = ocrpt_report_expr_parse(r, "r.value + 1", NULL);

	ocrpt_expr_set_field_expr(rd.rval1, rd.expr);

	ocrpt_execute(o);

	ocrpt_free(o);

	printf("Bad expression creation and evaluation order:\n");

	o = ocrpt_init();
	r = setup_report(o, &rd);

	/*
	 * "rval1" is created earlier than "expr".
	 * Since the order of expression evaluation is the same
	 * as the order of expression creation, this is bad.
	 *
	 * For complex (self-referencing) expressions, it can be a problem.
	 */
	rd.rval1 = ocrpt_report_expr_parse(r, "r.value + 1", NULL);
	rd.expr = ocrpt_report_expr_parse(r, "isnull(r.self) ? 1: r.self + 1", NULL);

	ocrpt_expr_set_field_expr(rd.rval1, rd.expr);

	ocrpt_execute(o);

	ocrpt_free(o);

	return 0;
}
