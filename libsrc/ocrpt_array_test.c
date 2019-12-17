/*
 * OpenCReports test
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

static const char *array[4][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

static const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

static const char *array2[3][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "2", "Betty Rubble", "beautiful", "27", "yes" },
	{ "1", "Barney Rubble", "small", "29", "yes" },
};

void print_result_row(const char *name, ocrpt_query_result *qr, int32_t cols) {
	int i;

	printf("Query: '%s':\n", name);
	for (i = 0; i < cols; i++) {
		printf("\tCol #%d: '%s': '%s'", i, qr[i].name, qr[i].result.isnull ? "NULL" : qr[i].result.string->str);
		if (!qr[i].result.isnull && qr[i].result.number_initialized)
			mpfr_printf(" (converted to number: %RF)", qr[i].result.number);
		printf("\n");
	}
}

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_query *q, *q2;
	ocrpt_query_result *qr, *qr2;
	ocrpt_expr *id, *name, *age, *adult, *match;
	char *err;
	int32_t cols, cols2, row, i;

	err = NULL;
	id = ocrpt_expr_parse(o, "id", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(o, id);

	err = NULL;
	name = ocrpt_expr_parse(o, "name", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(o, name);

	err = NULL;
	age = ocrpt_expr_parse(o, "age * 2", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(o, age);

	err = NULL;
	adult = ocrpt_expr_parse(o, "a.adult", &err);

	q = ocrpt_add_array_query_as(o, "array", "a", (const char **)array, 3, 5, coltypes);
	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns:\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, qr[i].name);

	ocrpt_expr_resolve(o, id);
	ocrpt_expr_resolve(o, name);
	ocrpt_expr_resolve(o, age);
	ocrpt_expr_resolve(o, adult);

	row = 0;
	ocrpt_navigate_start(o, q);

	while (ocrpt_navigate_next(o, q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(o, id);
		r = ocrpt_expr_eval(o, id);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, name);
		r = ocrpt_expr_eval(o, name);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, age);
		r = ocrpt_expr_eval(o, age);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, adult);
		r = ocrpt_expr_eval(o, adult);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);
		printf("Expression is %s previous row\n", ocrpt_expr_cmp_results(o, adult) ? "identical to" : "different from");

		printf("\n");
	}

	printf("--- TESTING FOLLOWER ---\n\n");

	q2 = ocrpt_add_array_query_as(o, "array", "b", (const char **)array2, 2, 5, coltypes);
	ocrpt_add_query_follower(o, q, q2);

	row = 0;
	ocrpt_navigate_start(o, q);

	while (ocrpt_navigate_next(o, q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols2);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);
		print_result_row("b", qr2, cols2);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(o, id);
		r = ocrpt_expr_eval(o, id);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, name);
		r = ocrpt_expr_eval(o, name);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, age);
		r = ocrpt_expr_eval(o, age);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("\n");
	}

	ocrpt_free_query(o, q2);

	printf("--- TESTING FOLLOWER N:1 ---\n\n");

	q2 = ocrpt_add_array_query_as(o, "array", "b", (const char **)array2, 2, 5, coltypes);
	qr2 = ocrpt_query_get_result(q2, &cols2);
	err = NULL;
	match = ocrpt_expr_parse(o, "a.id = b.id", &err);
	ocrpt_strfree(err);

	ocrpt_add_query_follower_n_to_1(o, q, q2, match);

	row = 0;
	ocrpt_navigate_start(o, q);

	while (ocrpt_navigate_next(o, q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols2);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);
		print_result_row("b", qr2, cols2);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(o, id);
		r = ocrpt_expr_eval(o, id);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, name);
		r = ocrpt_expr_eval(o, name);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, age);
		r = ocrpt_expr_eval(o, age);
		printf("Evaluated: ");
		ocrpt_expr_result_print(r);

		printf("\n");
	}

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q2);

	printf("--- END ---\n");

	ocrpt_free_expr(id);
	ocrpt_free_expr(name);
	ocrpt_free_expr(age);
	ocrpt_free_expr(adult);

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q);

	ocrpt_free(o);

	return 0;
}
