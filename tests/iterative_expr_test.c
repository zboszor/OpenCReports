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
/* NOT static */ const char *array[ROWS + 1][COLS] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" },
	{ "4", "Betty Rubble", "beautiful", "27", "yes" },
	{ "5", "Barney Rubble", "small", "29", "yes" }
};

/* NOT static */ const int32_t coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);
	ocrpt_expr *e1, *e2;
	int i;

	e1 = ocrpt_expr_parse(o, "r.self + 2", NULL);
	/* Initialize the value of "e" to 0 */
	ocrpt_expr_init_results(e1, OCRPT_RESULT_NUMBER);
	for (i = 0; i < OCRPT_EXPR_RESULTS; i++)
		ocrpt_expr_set_nth_result_long(e1, i, 0);
	ocrpt_expr_set_iterative_start_value(e1, true);
	ocrpt_expr_resolve(e1);
	ocrpt_expr_optimize(e1);
	printf("e1 expr (starts with initial value): ");
	ocrpt_expr_print(e1);

	e2 = ocrpt_expr_parse(o, "r.self + 2", NULL);
	/* Initialize the value of "e" to 0 */
	ocrpt_expr_init_results(e2, OCRPT_RESULT_NUMBER);
	for (i = 0; i < OCRPT_EXPR_RESULTS; i++)
		ocrpt_expr_set_nth_result_long(e2, i, 0);
	/* This iterative expression starts with evaluated value */
	//ocrpt_expr_set_iterative_start_value(e2, true);
	ocrpt_expr_resolve(e2);
	ocrpt_expr_optimize(e2);
	printf("e2 expr (starts with evaluated value): ");
	ocrpt_expr_print(e2);

	printf("Both e1 and e2 were initialized to 0\n");
	printf("\n");

	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		ocrpt_result *r;

		r = ocrpt_expr_eval(e1);
		printf("e1 value: ");
		ocrpt_result_print(r);

		r = ocrpt_expr_eval(e2);
		printf("e2 value: ");
		ocrpt_result_print(r);

		printf("\n");
	}

	ocrpt_expr_free(e1);
	ocrpt_expr_free(e2);

	ocrpt_free(o);

	return 0;
}
