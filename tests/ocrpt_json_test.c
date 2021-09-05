/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.c"

static const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

static ocrpt_expr *id, *name, *age, *adult;

void create_exprs(opencreport *o) {
	char *err;

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

	ocrpt_expr_resolve(o, NULL, id);
	ocrpt_expr_resolve(o, NULL, name);
	ocrpt_expr_resolve(o, NULL, age);
	ocrpt_expr_resolve(o, NULL, adult);
}

void free_exprs(void) {
	ocrpt_expr_free(id);
	ocrpt_expr_free(name);
	ocrpt_expr_free(age);
	ocrpt_expr_free(adult);
}

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_json(o, "json");
	ocrpt_query *q;
	ocrpt_query_result *qr;
	int32_t cols, row, i;

	q = ocrpt_query_add_json(o, ds, "a", "jsonquery.json", coltypes);
	if (!q) {
		ocrpt_free(o);
		return 0;
	}

	create_exprs(o);

	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns:\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, qr[i].name);

	ocrpt_expr_resolve(o, NULL, id);
	ocrpt_expr_resolve(o, NULL, name);
	ocrpt_expr_resolve(o, NULL, age);
	ocrpt_expr_resolve(o, NULL, adult);

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

	printf("--- END ---\n");

	free_exprs();

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q);

	ocrpt_free(o);

	return 0;
}
