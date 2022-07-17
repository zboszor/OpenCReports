/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_odbc(o, "odbc", "ocrpttest2", "ocrpt", NULL);
	ocrpt_datasource *ds2 = ocrpt_datasource_add_odbc(o, "odbc2", "ocrpttest2", "ocrpt", NULL);
	ocrpt_query *q = ocrpt_query_add_odbc(o, ds, "a", "SELECT * FROM flintstones");
	ocrpt_query *q2 = ocrpt_query_add_odbc(o, ds2, "b", "SELECT * FROM rubbles");
	int32_t cols, cols2, i, row;
	ocrpt_query_result *qr = ocrpt_query_get_result(q, &cols);
	ocrpt_query_result *qr2;
	ocrpt_expr *match;
	char *err;

	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns:\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, qr[i].name);

	qr2 = ocrpt_query_get_result(q2, &cols2);
	printf("Query columns:\n");
	for (i = 0; i < cols2; i++)
		printf("%d: '%s'\n", i, qr2[i].name);

	err = NULL;
	match = ocrpt_expr_parse(o, NULL, "a.id = b.id", &err);
	ocrpt_strfree(err);

	ocrpt_query_add_follower_n_to_1(o, q, q2, match);

	row = 0;
	ocrpt_query_navigate_start(o, q);

	while (ocrpt_query_navigate_next(o, q)) {
		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		print_result_row("b", qr2, cols2);

		printf("\n");
	}

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q);

	ocrpt_free(o);

	return 0;
}
