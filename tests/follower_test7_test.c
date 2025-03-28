/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 5
#define COLS 5

const char *array[ROWS + 1][COLS] = {
	{ "first_name", "last_name", "color", "group", "breakfast" },
	{ "Bob", "Doan", "blue", "1", "Green Eggs And Spam I Am I Am" },
	{ "Eric", "Eburuschkin", "green", "1", "Green Eggs And Spam I Am I Am" },
	{ "Mike", "Roth", "yellow", "2", "Green Eggs And Spam I Am I Am" },
	{ "Bob", "Kratz", "pink", "2", "Green Eggs And Spam I Am I Am" },
	{ "Steve", "Tilden", "purple", "2", "Green Eggs And Spam I Am I Am" }
};

#define ROWS1 5
#define COLS1 1
const char *initials[ROWS1 + 1][COLS1] = {
	{ "initials" },
	{ "WRD" },
	{ "ERB" },
	{ "MR" },
	{ "RK" },
	{ "ST" }
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q = ocrpt_query_add_data(ds, "data", (const char **)array, ROWS, COLS, NULL, 0);
	ocrpt_query *q2 = ocrpt_query_add_data(ds, "more_data", (const char **)initials, ROWS1, COLS1, NULL, 0);
	ocrpt_expr *match;
	uint32_t row;

	match = ocrpt_expr_parse(o, "rownum('data') = rownum('more_data')", NULL);
	ocrpt_query_add_follower_n_to_1(q, q2, match);

	if (!ocrpt_parse_xml(o, "follower.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	ocrpt_report *r = get_first_report(o);

	/* There is only one break in the report, extract it */
	ocrpt_list *brl = NULL;
	ocrpt_break *br = ocrpt_break_get_next(r, &brl);

	row = 0;
	ocrpt_query_navigate_start(q);
	ocrpt_report_resolve_breaks(r);
	ocrpt_report_resolve_variables(r);

	while (ocrpt_query_navigate_next(q)) {
		int32_t cols, cols2;

		ocrpt_query_result *qr = ocrpt_query_get_result(q, &cols);
		ocrpt_query_result *qr2 = ocrpt_query_get_result(q2, &cols2);

		if (ocrpt_break_check_fields(br)) {
			long rownum;
			printf("Break triggers\n");

			rownum = ocrpt_report_get_query_rownum(r);
			if (rownum > 1)
				ocrpt_break_reset_vars(br);
		}

		ocrpt_report_evaluate_variables(r);

		printf("Row #%d\n", row++);
		print_result_row("data", qr, cols);
		print_result_row("more_data", qr2, cols2);

		printf("\n");
	}

	ocrpt_free(o);

	return 0;
}
