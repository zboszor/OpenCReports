/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

const char *array2[3][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "2", "Betty Rubble", "beautiful", "27", "yes" },
	{ "1", "Barney Rubble", "small", "29", "yes" },
};

static ocrpt_expr *id, *name, *age, *adult;

void create_exprs(opencreport *o) {
	char *err;

	err = NULL;
	id = ocrpt_expr_parse(o, NULL, "id", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(o, id);

	err = NULL;
	name = ocrpt_expr_parse(o, NULL, "name", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(o, name);

	err = NULL;
	age = ocrpt_expr_parse(o, NULL, "age * 2", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(o, age);

	err = NULL;
	adult = ocrpt_expr_parse(o, NULL, "a.adult", &err);

	ocrpt_expr_resolve(o, NULL, id);
	ocrpt_expr_resolve(o, NULL, name);
	ocrpt_expr_resolve(o, NULL, age);
	ocrpt_expr_resolve(o, NULL, adult);
}

void free_exprs(opencreport *o, ocrpt_report *r) {
	ocrpt_expr_free(o, r, id);
	ocrpt_expr_free(o, r, name);
	ocrpt_expr_free(o, r, age);
	ocrpt_expr_free(o, r, adult);
}

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds, *ds2;
	ocrpt_query *q, *q2;
	ocrpt_query_result *qr, *qr2;
	int32_t cols, cols2, row, i;

	if (!ocrpt_parse_xml(o, "csvquery.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ds = ocrpt_datasource_get(o, "mycsv");
	q = ocrpt_query_get(o, "a");
	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns:\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, qr[i].name);

	create_exprs(o);

	row = 0;
	ocrpt_query_navigate_start(o, q);

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(o, id);
		r = ocrpt_expr_eval(o, NULL, id);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, name);
		r = ocrpt_expr_eval(o, NULL, name);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, age);
		r = ocrpt_expr_eval(o, NULL, age);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, adult);
		r = ocrpt_expr_eval(o, NULL, adult);
		printf("Evaluated: ");
		ocrpt_result_print(r);
		printf("Expression is %s previous row\n", ocrpt_expr_cmp_results(o, adult) ? "identical to" : "different from");

		printf("\n");
	}

	free_exprs(o, NULL);

	ocrpt_query_free(o, q);
	ocrpt_datasource_free(o, ds);

	printf("--- TESTING FOLLOWER ---\n\n");

	if (!ocrpt_parse_xml(o, "csvquery2.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ds = ocrpt_datasource_get(o, "mycsv");
	if (!ds) {
		printf("datasource 'mycsv' not found\n");
		ocrpt_free(o);
		return 1;
	}
	q = ocrpt_query_get(o, "a");
	if (!q) {
		printf("query 'a' not found\n");
		ocrpt_free(o);
		return 1;
	}
	qr = ocrpt_query_get_result(q, &cols);
	printf("q cols %d\n", cols);

	ds2 = ocrpt_datasource_get(o, "myarray");
	if (!ds2) {
		printf("datasource 'myarray' not found\n");
		ocrpt_free(o);
		return 1;
	}
	q2 = ocrpt_query_get(o, "b");
	if (!q2) {
		printf("query 'b' not found\n");
		ocrpt_free(o);
		return 1;
	}
	qr2 = ocrpt_query_get_result(q2, &cols2);
	printf("q2 cols %d\n", cols2);

	create_exprs(o);

	row = 0;
	ocrpt_query_navigate_start(o, q);

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols2);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);
		print_result_row("b", qr2, cols2);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(o, id);
		r = ocrpt_expr_eval(o, NULL, id);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, name);
		r = ocrpt_expr_eval(o, NULL, name);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, age);
		r = ocrpt_expr_eval(o, NULL, age);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("\n");
	}

	free_exprs(o, NULL);

	ocrpt_query_free(o, q);
	ocrpt_query_free(o, q2);

	printf("--- TESTING FOLLOWER N:1 ---\n\n");

	if (!ocrpt_parse_xml(o, "csvquery3.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	q = ocrpt_query_get(o, "a");
	q2 = ocrpt_query_get(o, "b");

	create_exprs(o);

	row = 0;
	ocrpt_query_navigate_start(o, q);

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);
		qr2 = ocrpt_query_get_result(q2, &cols2);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);
		print_result_row("b", qr2, cols2);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(o, id);
		r = ocrpt_expr_eval(o, NULL, id);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, name);
		r = ocrpt_expr_eval(o, NULL, name);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(o, age);
		r = ocrpt_expr_eval(o, NULL, age);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("\n");
	}

	free_exprs(o, NULL);

	/* ocrpt_free() will free it */
	//ocrpt_query_free(o, q);
	//ocrpt_query_free(o, q2);

	printf("--- END ---\n");


	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q);

	ocrpt_free(o);

	return 0;
}
