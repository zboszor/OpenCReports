/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <string.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

static const char *array[4][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ NULL, "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

static const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_expr *id, *name, *err;
	ocrpt_result rs;
	int32_t cols, row;

	memset(&rs, 0, sizeof(rs));

	id = ocrpt_expr_parse(o, NULL, "id", NULL);
	ocrpt_expr_print(o, id);

	name = ocrpt_expr_parse(o, NULL, "name", NULL);
	ocrpt_expr_print(o, name);

	err = ocrpt_expr_parse(o, NULL, "error('Error')", NULL);
	ocrpt_expr_print(o, err);

	q = ocrpt_query_add_array(o, ds, "a", (const char **)array, 3, 5, coltypes);

	ocrpt_expr_resolve(o, NULL, id);
	ocrpt_expr_resolve(o, NULL, name);
	ocrpt_expr_resolve(o, NULL, err);

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
		printf("Copied: ");
		ocrpt_result_copy(o, &rs, id->result[o->residx]);
		ocrpt_result_print(&rs);

		printf("Expression: ");
		ocrpt_expr_print(o, name);
		r = ocrpt_expr_eval(o, NULL, name);
		printf("Evaluated: ");
		ocrpt_result_print(r);
		printf("Copied: ");
		ocrpt_result_copy(o, &rs, name->result[o->residx]);
		ocrpt_result_print(&rs);

		printf("Expression: ");
		ocrpt_expr_print(o, err);
		r = ocrpt_expr_eval(o, NULL, err);
		printf("Evaluated: ");
		ocrpt_result_print(r);
		printf("Copied: ");
		ocrpt_result_copy(o, &rs, err->result[o->residx]);
		ocrpt_result_print(&rs);

		printf("\n");
	}

	ocrpt_expr_free(o, NULL, id);
	ocrpt_expr_free(o, NULL, name);
	ocrpt_expr_free(o, NULL, err);
	ocrpt_result_free_data(&rs);

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q);

	ocrpt_free(o);

	return 0;
}
