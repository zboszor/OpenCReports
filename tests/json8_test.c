/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

#define JSONFILE "jsonquery8.json"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_json(o, "json");
	ocrpt_query *q;

	q = ocrpt_query_add_json(ds, "a", JSONFILE, NULL, 0);
	if (!q) {
		printf(JSONFILE " parsing failed (good, it's intentional)\n");
		ocrpt_free(o);
		return 0;
	}

	printf(JSONFILE " parsing succeded (bad, there's errors in it)\n");
	ocrpt_free(o);

	return 0;
}
