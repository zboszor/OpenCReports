/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "optionfile", .param_value = "./mariadb/ocrpt.cnf" },
		{ .param_name = "group", .param_value = "ocrpt" },
		{ NULL }
	};
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "mariadb", "mariadb", conn_params);

	printf("Connecting to MariaDB database was %ssuccessful\n", (ds ? "" : "NOT "));

	ocrpt_free(o);

	return 0;
}
