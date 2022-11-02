/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

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

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *q, *q2;
	ocrpt_query_result *qr, *qr2;
	ocrpt_expr *id, *rownum1, *rownum2, *rownum3, *match;
	char *err;
	int32_t cols, cols2, row;

	err = NULL;
	id = ocrpt_expr_parse(o, "id", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(id);

	err = NULL;
	rownum1 = ocrpt_expr_parse(o, "rownum()", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(rownum1);

	err = NULL;
	rownum2 = ocrpt_expr_parse(o, "rownum('a')", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(rownum2);

	err = NULL;
	rownum3 = ocrpt_expr_parse(o, "rownum('b')", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(rownum3);

	q = ocrpt_query_add_array(ds, "a", (const char **)array, 3, 5, coltypes);
	q2 = ocrpt_query_add_array(ds, "b", (const char **)array2, 2, 5, coltypes);
	ocrpt_query_add_follower(q, q2);

	ocrpt_expr_resolve(id);
	ocrpt_expr_resolve(rownum1);
	ocrpt_expr_resolve(rownum2);
	ocrpt_expr_resolve(rownum3);

	printf("--- TESTING FOLLOWER ---\n\n");

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols2);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);
		print_result_row("b", qr2, cols2);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(id);
		r = ocrpt_expr_eval(id);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(rownum1);
		r = ocrpt_expr_eval(rownum1);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(rownum2);
		r = ocrpt_expr_eval(rownum2);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(rownum3);
		r = ocrpt_expr_eval(rownum3);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("\n");
	}

	/*
	 * rownum3 referenced query 'b'
	 * free this expression to avoid use-after-free
	 * after freeing and re-creating q2 a.k.a. query 'b'
	 */
	ocrpt_expr_free(rownum3);

	ocrpt_query_free(q2);

	printf("--- TESTING FOLLOWER N:1 ---\n\n");

	q2 = ocrpt_query_add_array(ds, "b", (const char **)array2, 2, 5, coltypes);
	qr2 = ocrpt_query_get_result(q2, &cols2);
	err = NULL;
	match = ocrpt_expr_parse(o, "a.id = b.id", &err);
	ocrpt_strfree(err);

	err = NULL;
	rownum3 = ocrpt_expr_parse(o, "rownum('b')", &err);
	ocrpt_strfree(err);

	ocrpt_query_add_follower_n_to_1(q, q2, match);

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols2);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);
		print_result_row("b", qr2, cols2);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(id);
		r = ocrpt_expr_eval(id);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(rownum1);
		r = ocrpt_expr_eval(rownum1);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(rownum2);
		r = ocrpt_expr_eval(rownum2);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(rownum3);
		r = ocrpt_expr_eval(rownum3);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("\n");
	}

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q2);

	printf("--- END ---\n");

	ocrpt_expr_free(id);
	ocrpt_expr_free(rownum1);
	ocrpt_expr_free(rownum2);
	ocrpt_expr_free(rownum3);

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q);

	ocrpt_free(o);

	return 0;
}
