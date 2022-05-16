/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

/* NOT static */ const char *array[6][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" },
	{ "4", "Betty Rubble", "beautiful", "27", "yes" },
	{ "5", "Barney Rubble", "small", "29", "yes" }
};

/* NOT static */ const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *q = ocrpt_query_add_array(o, ds, "a", (const char **)array, 5, 5, coltypes);
	ocrpt_expr *e1, *e2;
	int i;

	e1 = ocrpt_expr_parse(o, NULL, "r.self + 2", NULL);
	/* Initialize the value of "e" to 0 */
	ocrpt_expr_init_results(o, e1, OCRPT_RESULT_NUMBER);
	for (i = 0; i < OCRPT_EXPR_RESULTS; i++)
		mpfr_set_ui(e1->result[i]->number, 0, o->rndmode);
	ocrpt_expr_set_iterative_start_value(e1, true);
	ocrpt_expr_resolve(o, NULL, e1);
	ocrpt_expr_optimize(o, NULL, e1);
	printf("e1 expr (starts with initial value): ");
	ocrpt_expr_print(o, e1);

	e2 = ocrpt_expr_parse(o, NULL, "r.self + 2", NULL);
	/* Initialize the value of "e" to 0 */
	ocrpt_expr_init_results(o, e2, OCRPT_RESULT_NUMBER);
	for (i = 0; i < OCRPT_EXPR_RESULTS; i++)
		mpfr_set_ui(e2->result[i]->number, 0, o->rndmode);
	/* This iterative expression starts with evaluated value */
	//ocrpt_expr_set_iterative_start_value(e2, true);
	ocrpt_expr_resolve(o, NULL, e2);
	ocrpt_expr_optimize(o, NULL, e2);
	printf("e2 expr (starts with evaluated value): ");
	ocrpt_expr_print(o, e2);

	printf("Both e1 and e2 were initialized to 0\n");
	printf("\n");

	ocrpt_query_navigate_start(o, q);

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_result *r;

		r = ocrpt_expr_eval(o, NULL, e1);
		printf("e1 value: ");
		ocrpt_result_print(r);

		r = ocrpt_expr_eval(o, NULL, e2);
		printf("e2 value: ");
		ocrpt_result_print(r);

		printf("\n");
	}

	ocrpt_expr_free(o, NULL, e1);
	ocrpt_expr_free(o, NULL, e2);

	ocrpt_free(o);

	return 0;
}
