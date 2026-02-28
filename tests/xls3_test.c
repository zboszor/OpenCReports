/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

int main(int argc, char **argv) {
	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "filename", .param_value = "notexist.xls" },
		{ NULL }
	};
	opencreport *o;
	ocrpt_datasource *ds;

	if (!ocrpt_pandas_initialize()) {
		fprintf(stderr, "Failed to register pandas datasource driver.\n");
		return 0;
	}

	o = ocrpt_init();
	ds = ocrpt_datasource_add(o, "pandas", "pandas", conn_params);
	if (!ds) {
		fprintf(stderr, "Failed to add a pandas datasource.\n");
		ocrpt_free(o);
		ocrpt_pandas_deinitialize();
		return 1;
	}

	fprintf(stderr, "Adding a non-existing pandas datasource succeeded (ERROR).\n");

	ocrpt_free(o);

	ocrpt_pandas_deinitialize();

	return 0;
}
