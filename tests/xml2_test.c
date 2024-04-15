/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

#define COLS 5
static const int32_t coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_xml(o, "xml");
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_expr *id, *name, *age, *adult;
	char *err;
	int32_t cols, row, i;

	err = NULL;
	id = ocrpt_expr_parse(o, "id", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(id);

	err = NULL;
	name = ocrpt_expr_parse(o, "name", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(name);

	err = NULL;
	age = ocrpt_expr_parse(o, "age * 2", &err);
	ocrpt_strfree(err);
	ocrpt_expr_print(age);

	err = NULL;
	adult = ocrpt_expr_parse(o, "a.adult", &err);

	q = ocrpt_query_add_file(ds, "a", "xmldata2.xml", coltypes, COLS);
	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns:\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr, i));

	ocrpt_expr_resolve(id);
	ocrpt_expr_resolve(name);
	ocrpt_expr_resolve(age);
	ocrpt_expr_resolve(adult);

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		ocrpt_result *r;

		qr = ocrpt_query_get_result(q, &cols);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		printf("Expression: ");
		ocrpt_expr_print(id);
		r = ocrpt_expr_eval(id);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(name);
		r = ocrpt_expr_eval(name);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(age);
		r = ocrpt_expr_eval(age);
		printf("Evaluated: ");
		ocrpt_result_print(r);

		printf("Expression: ");
		ocrpt_expr_print(adult);
		r = ocrpt_expr_eval(adult);
		printf("Evaluated: ");
		ocrpt_result_print(r);
		printf("Expression is %s previous row\n", ocrpt_expr_cmp_results(adult) ? "identical to" : "different from");

		printf("\n");
	}

	printf("--- END ---\n");

	ocrpt_expr_free(id);
	ocrpt_expr_free(name);
	ocrpt_expr_free(age);
	ocrpt_expr_free(adult);

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q);

	ocrpt_free(o);

	return 0;
}
