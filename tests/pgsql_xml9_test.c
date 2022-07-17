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
	ocrpt_query *q;
	ocrpt_query_result *qr;
	int32_t cols, i, row;

	setenv("dsname", "pgsql", 1);
	setenv("dstype", "postgresql", 1);
	setenv("dbconnstr", "dbname=ocrpttest user=ocrpt", 1);
	setenv("qname", "pgquery", 1);

	if (!ocrpt_parse_xml(o, "pgsqlquery9.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ds = ocrpt_datasource_get(o, "pgsql");
	printf("Connecting to PostgreSQL database was %ssuccessful\n", (ds ? "" : "NOT "));

	q = ocrpt_query_get(o, "pgquery");
	printf("Adding query was %ssuccessful\n", (q ? "" : "NOT "));

	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns:\n");
		for (i = 0; i < cols; i++)
			printf("%d: '%s'\n", i, qr[i].name);

	row = 0;
	ocrpt_query_navigate_start(o, q);

	while (ocrpt_query_navigate_next(o, q)) {
		qr = ocrpt_query_get_result(q, &cols);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");
	}

	ocrpt_free(o);

	return 0;
}
