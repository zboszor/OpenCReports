/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

#define COLS 5
const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

OCRPT_STATIC_FUNCTION(my_rownum) {
	if (ocrpt_expr_get_num_operands(e) != 0) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	ocrpt_expr_set_long(e, 100L);
}

struct rowdata {
	ocrpt_expr *id;
	ocrpt_expr *lineno;
	ocrpt_expr *rownum;
};

static void test_newrow_cb(opencreport *o, ocrpt_report *r, void *ptr) {
	struct rowdata *rd = ptr;
	ocrpt_result *rs;

	printf("id:\n");
	rs = ocrpt_expr_get_result(rd->id);
	ocrpt_result_print(rs);

	printf("lineno:\n");
	rs = ocrpt_expr_get_result(rd->lineno);
	ocrpt_result_print(rs);

	printf("rownum():\n");
	rs = ocrpt_expr_get_result(rd->rownum);
	ocrpt_result_print(rs);

	printf("\n");
}

int main(int argc, char **argv) {
	struct rowdata rd;
	opencreport *o = ocrpt_init();

	ocrpt_function_add(o, "rownum", my_rownum, NULL, 0, false, false, false, false);

	if (!ocrpt_parse_xml(o, "csvquery.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_query *q = ocrpt_query_get(o, "a");
	ocrpt_report *r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));
	ocrpt_report_set_main_query(r, q);

	if (!ocrpt_report_add_new_row_cb(r, test_newrow_cb, &rd)) {
		fprintf(stderr, "Failed to add new row callback.\n");
		ocrpt_free(o);
		return  1;
	}

	rd.id = ocrpt_report_expr_parse(r, "id", NULL);
	rd.lineno = ocrpt_report_expr_parse(r, "r.lineno", NULL);
	rd.rownum = ocrpt_report_expr_parse(r, "rownum()", NULL);

	ocrpt_expr_resolve(rd.id);
	ocrpt_expr_resolve(rd.lineno);
	ocrpt_expr_resolve(rd.rownum);

	ocrpt_execute(o);

	ocrpt_free(o);

	return 0;
}
