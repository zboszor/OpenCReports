/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
//#include "test_common.h"

int main(int argc, char **argv) {
	if (!ocrpt_pandas_initialize()) {
		fprintf(stderr, "Failed to register pandas datasource driver.\n");
		return 0;
	}

	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_xlsx_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		ocrpt_pandas_deinitialize();
		return 0;
	}

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	ocrpt_pandas_deinitialize();

	return 0;
}
