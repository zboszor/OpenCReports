/*
 * OpenCReports test
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds;

	setenv("dsname", "pgsql", 1);
	setenv("dstype", "postgresql", 1);
	setenv("dbname", "ocrpttest", 1);
	setenv("dbuser", "ocrpt", 1);

	if (!ocrpt_parse_xml(o, "pgsqlquery5.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ds = ocrpt_datasource_find(o, "pgsql");
	printf("Connecting to PostgreSQL database was %ssuccessful\n", (ds ? "" : "NOT "));

	ocrpt_free(o);

	return 0;
}
