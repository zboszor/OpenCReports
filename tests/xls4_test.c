/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

struct rowdata {
	ocrpt_query *q;
	int32_t row;
};

static void test_newrow_cb(opencreport *o, ocrpt_report *r, void *ptr) {
	struct rowdata *rd = ptr;
	ocrpt_query_result *qr;
	int32_t cols;

	qr = ocrpt_query_get_result(rd->q, &cols);

	printf("Row #%d\n", rd->row++);
	print_result_row("a", qr, cols);

	printf("\n");
}

int main(int argc, char **argv) {
	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "filename", .param_value = "test.xls" },
		{ NULL }
	};
	opencreport *o;
	ocrpt_datasource *ds;
	ocrpt_report *r;
	struct rowdata rd = { .row = 0 };

	o = ocrpt_init();
	ds = ocrpt_datasource_add(o, "pandas", "pandas", conn_params);
	if (!ds) {
		fprintf(stderr, "Failed to add a pandas datasource.\n");
		ocrpt_free(o);
		return 1;
	}

	r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));

	rd.q = ocrpt_query_add_file(ds, "a", "Sheet2", NULL, 0);
	if (!rd.q) {
		fprintf(stderr, "Failed to add a spreadsheet query.\n");
		ocrpt_free(o);
		return 1;
	}

	ocrpt_report_set_main_query(r, rd.q);

	if (!ocrpt_report_add_new_row_cb(r, test_newrow_cb, &rd)) {
		fprintf(stderr, "Failed to add new row callback.\n");
		ocrpt_free(o);
		return  1;
	}

	ocrpt_execute(o);

	ocrpt_free(o);

	return 0;
}
