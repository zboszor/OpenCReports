/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 5
#define COLS 5

static const char *array[ROWS + 1][COLS] = {
	{ "first_name", "last_name", "color", "group", "breakfast" },
	{ "Bob", "Doan", "blue", "1", "Green Eggs And Spam I Am I Am" },
	{ "Eric", "Eburuschkin", "green", "1", "Green Eggs And Spam I Am I Am" },
	{ "Mike", "Roth", "yellow", "2", "Green Eggs And Spam I Am I Am" },
	{ "Bob", "Kratz", "pink", "2", "Green Eggs And Spam I Am I Am" },
	{ "Steve", "Tilden", "purple", "2", "Green Eggs And Spam I Am I Am" }
};

#define ROWS1 2
#define COLS1 1
static const char *initials[ROWS1 + 1][COLS1] = {
	{ "initials" },
	{ "WRD" },
	{ "ERB" }
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q, *q2;
	ocrpt_query_result *qr, *qr2;
	ocrpt_expr *match;
	char *err;
	int32_t cols, cols2, row, i;

	q = ocrpt_query_add_array(ds, "data", (const char **)array, ROWS, COLS, NULL, 0);
	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns:\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr, i));

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		qr = ocrpt_query_get_result(q, &cols);

		printf("Row #%d\n", row++);
		print_result_row("data", qr, cols);

		printf("\n");
	}

	printf("--- TESTING FOLLOWER ---\n\n");

	q2 = ocrpt_query_add_array(ds, "more_data", (const char **)initials, ROWS1, COLS1, NULL, 0);
	ocrpt_query_add_follower(q, q2);

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols2);

		printf("Row #%d\n", row++);
		print_result_row("data", qr, cols);
		print_result_row("more_data", qr2, cols2);

		printf("\n");
	}

	ocrpt_query_free(q2);

	printf("--- TESTING FOLLOWER N:1 ---\n\n");

	q2 = ocrpt_query_add_array(ds, "more_data", (const char **)initials, ROWS1, COLS1, NULL, 0);
	qr2 = ocrpt_query_get_result(q2, &cols2);
	err = NULL;
	match = ocrpt_expr_parse(o, "rownum('data') = rownum('more_data')", &err);
	ocrpt_strfree(err);

	ocrpt_query_add_follower_n_to_1(q, q2, match);

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols2);

		printf("Row #%d\n", row++);
		print_result_row("data", qr, cols);
		print_result_row("more_data", qr2, cols2);

		printf("\n");
	}

	printf("--- END ---\n");

	ocrpt_free(o);

	return 0;
}
