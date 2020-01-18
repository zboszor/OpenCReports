/*
 * OpenCReports test
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds;

	if (!ocrpt_parse_xml(o, "odbcquery2.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ds = ocrpt_datasource_find(o, "odbc");
	printf("Connecting to ODBC database was %ssuccessful\n", (ds ? "" : "NOT "));

	ocrpt_free(o);

	return 0;
}
