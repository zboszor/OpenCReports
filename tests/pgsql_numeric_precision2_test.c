/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

#define ROWS 10000000
#define ROWS_STR "10000000"

struct rowdata {
	int rownum;
	ocrpt_expr *e;
};

static void test_newrow_cb(opencreport *o, ocrpt_report *r, void *ptr) {
	struct rowdata *rd = ptr;

	rd->rownum++;
	if (rd->rownum % 100000 == 0)
		printf("at row %d\n", rd->rownum);
	if (rd->rownum == ROWS) {
		printf("\nShowing off numbers not possible with RLIB...\n");
		ocrpt_result *rs = ocrpt_expr_get_result(rd->e);
		ocrpt_result_print(rs);
	}
}

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds;
	ocrpt_query *q;
	ocrpt_report *r;
	struct rowdata rd = { .rownum = 0 };

	ds = ocrpt_datasource_add_postgresql(o, "pgsql", NULL, NULL, "ocrpttest", "ocrpt", NULL);
	if (!ds) {
		fprintf(stderr, "ocrpt_datasource_add_postgresql failed\n");
		return 1;
	}
	/* 10^14 + 10^(-7) added together 10^7 times */
	q = ocrpt_query_add_postgresql(ds, "a", "select '100000000000000.0000001'::numeric as num from generate_series(1," ROWS_STR ");");
	if (!q) {
		fprintf(stderr, "ocrpt_query_add_postgresql failed\n");
		return 1;
	}

	r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));

	ocrpt_report_set_main_query(r, q);

	if (!ocrpt_report_add_new_row_cb(r, test_newrow_cb, &rd)) {
		fprintf(stderr, "Failed to add new row callback.\n");
		ocrpt_free(o);
		return 1;
	}

	ocrpt_variable_new(r, OCRPT_VARIABLE_SUM, "var1", "a.num", NULL);

	rd.e = ocrpt_report_expr_parse(r, "v.var1", NULL);
	ocrpt_expr_resolve(rd.e);
	ocrpt_expr_optimize(rd.e);

	ocrpt_execute(o);

	ocrpt_expr_free(rd.e);

	ocrpt_free(o);

	return 0;
}
