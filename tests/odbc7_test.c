/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "dbname", .param_value = "ocrpttest2" },
		{ .param_name = "user", .param_value = "ocrpt" },
		{ NULL }
	};
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "odbc", "odbc", conn_params);
	ocrpt_datasource *ds2 = ocrpt_datasource_add(o, "odbc2", "odbc", conn_params);
	ocrpt_query *q = ocrpt_query_add_sql(ds, "a", "SELECT * FROM flintstones");
	ocrpt_query *q2 = ocrpt_query_add_sql(ds2, "b", "SELECT * FROM rubbles");
	int32_t cols, cols2, i, row;
	ocrpt_query_result *qr UNUSED = ocrpt_query_get_result(q, &cols);
	ocrpt_query_result *qr2;
	ocrpt_expr *match;
	char *err;

	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns:\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr, i));

	qr2 = ocrpt_query_get_result(q2, &cols2);
	printf("Query columns:\n");
	for (i = 0; i < cols2; i++)
		printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr2, i));

	err = NULL;
	match = ocrpt_expr_parse(o, "a.id = b.id", &err);
	ocrpt_strfree(err);

	ocrpt_query_add_follower_n_to_1(q, q2, match);

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
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
