/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
#include "test_common.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds;
	ocrpt_query *q, *q2;
	ocrpt_query_result *qr, *qr2;
	int32_t cols, cols2, i, row;

	if (!ocrpt_parse_xml(o, "mariadbquery3.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ds = ocrpt_datasource_get(o, "mariadb");
	printf("Connecting to MariaDB database was %ssuccessful\n", (ds ? "" : "NOT "));

	q = ocrpt_query_get(o, "a");
	printf("Adding query 'a' was %ssuccessful\n", (q ? "" : "NOT "));
	q2 = ocrpt_query_get(o, "b");
	printf("Adding query 'b' was %ssuccessful\n", (q2 ? "" : "NOT "));

	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns (a):\n");
		for (i = 0; i < cols; i++)
			printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr, i));
	qr2 = ocrpt_query_get_result(q2, &cols2);
	printf("Query columns (b):\n");
		for (i = 0; i < cols2; i++)
			printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr2, i));

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);
		print_result_row("b", qr2, cols2);

		printf("\n");
	}

	ocrpt_free(o);

	return 0;
}
