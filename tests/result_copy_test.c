/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <string.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 3
#define COLS 5
static const char *array[ROWS + 1][COLS] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ NULL, "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

static const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_expr *id, *name, *err;
	ocrpt_result *rs = ocrpt_result_new(o);
	int32_t cols, row;

	id = ocrpt_expr_parse(o, "id", NULL);
	ocrpt_expr_print(id);

	name = ocrpt_expr_parse(o, "name", NULL);
	ocrpt_expr_print(name);

	err = ocrpt_expr_parse(o, "error('Error')", NULL);
	ocrpt_expr_print(err);

	q = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);

	ocrpt_expr_resolve(id);
	ocrpt_expr_resolve(name);
	ocrpt_expr_resolve(err);

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
		printf("Copied: ");
		ocrpt_result_copy(rs, ocrpt_expr_get_result(id));
		ocrpt_result_print(rs);

		printf("Expression: ");
		ocrpt_expr_print(name);
		r = ocrpt_expr_eval(name);
		printf("Evaluated: ");
		ocrpt_result_print(r);
		printf("Copied: ");
		ocrpt_result_copy(rs, ocrpt_expr_get_result(name));
		ocrpt_result_print(rs);

		printf("Expression: ");
		ocrpt_expr_print(err);
		r = ocrpt_expr_eval(err);
		printf("Evaluated: ");
		ocrpt_result_print(r);
		printf("Copied: ");
		ocrpt_result_copy(rs, ocrpt_expr_get_result(err));
		ocrpt_result_print(rs);

		printf("\n");
	}

	ocrpt_expr_free(id);
	ocrpt_expr_free(name);
	ocrpt_expr_free(err);
	ocrpt_result_free(rs);

	/* ocrpt_free() will free it */
	//ocrpt_free_query(o, q);

	ocrpt_free(o);

	return 0;
}
