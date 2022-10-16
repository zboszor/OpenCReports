/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
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
		ocrpt_result *rs = ocrpt_expr_get_result(o, rd->e);
		ocrpt_result_print(rs);
	}
}

int main(void) {
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
	q = ocrpt_query_add_postgresql(o, ds, "a", "select '0.0000001'::numeric as num from generate_series(1," ROWS_STR ");");
	if (!q) {
		fprintf(stderr, "ocrpt_query_add_postgresql failed\n");
		return 1;
	}

	r = ocrpt_report_new(o);
	ocrpt_part_append_report(o, NULL, NULL, NULL, r);

	ocrpt_report_set_main_query(o, r, q);

	if (!ocrpt_report_add_new_row_cb(o, r, test_newrow_cb, &rd)) {
		fprintf(stderr, "Failed to add new row callback.\n");
		ocrpt_free(o);
		return  1;
	}

	ocrpt_variable_new(o, r, OCRPT_VARIABLE_SUM, "var1", "a.num", NULL);

	rd.e = ocrpt_expr_parse(o, r, "v.var1", NULL);
	ocrpt_expr_resolve(o, r, rd.e);
	ocrpt_expr_optimize(o, r, rd.e);

	ocrpt_execute(o);

	ocrpt_expr_free(o, r, rd.e);

	ocrpt_free(o);

	return 0;
}
