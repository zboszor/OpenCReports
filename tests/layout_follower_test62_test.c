/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 5
#define COLS 6

const char *array[ROWS + 1][COLS] = {
	{ "id", "first_name", "last_name", "color", "group", "breakfast" },
	{ "1", "Bob", "Doan", "blue", "1", "Green Eggs And Spam I Am I Am" },
	{ "2", "Eric", "Eburuschkin", "green", "1", "Green Eggs And Spam I Am I Am" },
	{ "3", "Mike", "Roth", "yellow", "2", "Green Eggs And Spam I Am I Am" },
	{ "4", "Bob", "Kratz", "pink", "2", "Green Eggs And Spam I Am I Am" },
	{ "5", "Steve", "Tilden", "purple", "2", "Green Eggs And Spam I Am I Am" }
};

#define ROWS1 3
#define COLS1 2
const char *initials[ROWS1 + 1][COLS1] = {
	{ "id", "initials" },
	{ "1", "WRD" },
	{ "2", "ERB" },
	{ "4", "RK" }
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *q = ocrpt_query_add_array(ds, "data", (const char **)array, ROWS, COLS, NULL, 0);
	ocrpt_query *q2 = ocrpt_query_add_array(ds, "more_data", (const char **)initials, ROWS1, COLS1, NULL, 0);
	ocrpt_expr *match;

	match = ocrpt_expr_parse(o, "data.id = more_data.id", NULL);
	ocrpt_query_add_follower_n_to_1(q, q2, match);

	if (!ocrpt_parse_xml(o, "follower2.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
