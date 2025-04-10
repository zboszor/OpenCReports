/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds;

	setenv("dsname", "pgsql", 1);
	setenv("dstype", "postgresql", 1);
	setenv("dbconnstr", "dbname=ocrpttest user=ocrpt", 1);
	setenv("qname", "pgquery", 1);
	setenv("query", "SELECT * FROM flintstones;", 1);

	if (!ocrpt_parse_xml(o, "pgsqlquery7.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ds = ocrpt_datasource_get(o, "pgsql");
	printf("Connecting to PostgreSQL database was %ssuccessful\n", (ds ? "" : "NOT "));

	ocrpt_free(o);

	return 0;
}
