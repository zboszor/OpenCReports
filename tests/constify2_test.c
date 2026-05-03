/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 3
#define COLS 4
static const char *constify_array[ROWS + 1][COLS] = {
	{ "font_name", "font_size", "memo", "expr" },
	{ "Arial", "10", "yes", "3 * id" },
	{ "Courier", "12", "no", "id * 3" },
	{ "Times New Roman", "11", "yes", "name" }
};

static const int32_t constify_coltypes[COLS] = {
	OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING
};

#define ROWS2 2
#define COLS2 2
static const char *array[ROWS2 + 1][COLS2] = {
	{ "id", "name" },
	{ "1", "George of the Jungle" },
	{ "2", "Alice in Wonderland" }
};

static const int32_t coltypes[COLS2] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING
};

static ocrpt_expr *e[ROWS][COLS];

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q;
	ocrpt_query *q_constify;
	ocrpt_query_result *qr;
	int32_t i, j, row, row_constify, cols, cols_constify;

	/* Latin1 -> UTF-8, a NOP for plain ASCII characters */
	ocrpt_datasource_set_encoding(ds, "ISO-8859-1");

	q = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS2, COLS2, coltypes, COLS2);
	qr = ocrpt_query_get_result(q, &cols);
	printf("Query columns for 'a':\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr, i));

	q_constify = ocrpt_query_add_data(ds, "constify", (const char **)constify_array, ROWS, COLS, constify_coltypes, COLS);
	qr = ocrpt_query_get_result(q_constify, &cols_constify);
	printf("Query columns for 'constify':\n");
	for (i = 0; i < cols_constify; i++)
		printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr, i));

	row_constify = 0;
	ocrpt_query_navigate_start(q_constify);

	while (ocrpt_query_navigate_next(q_constify)) {
		qr = ocrpt_query_get_result(q_constify, &cols_constify);

		printf("Row #%d\n", row_constify + 1);
		print_result_row("constify", qr, cols_constify);

		printf("\n");

		e[row_constify][0] = ocrpt_expr_parse(o, "eval(font_name)", NULL);
		e[row_constify][1] = ocrpt_expr_parse(o, "eval(font_size)", NULL);
		e[row_constify][2] = ocrpt_expr_parse(o, "eval(memo)", NULL);
		e[row_constify][3] = ocrpt_expr_parse(o, "eval(expr)", NULL);

		for (i = 0; i < cols_constify; i++) {
			ocrpt_expr_resolve_from_query(e[row_constify][i], q_constify);
			ocrpt_expr_constify(e[row_constify][i], q_constify);
			ocrpt_expr_optimize(e[row_constify][i]);
			printf("Expression: ");
			ocrpt_expr_print(e[row_constify][i]);
		}

		printf("\n");

		row_constify++;
	}

	row = 0;
	ocrpt_query_navigate_start(q);

	while (ocrpt_query_navigate_next(q)) {
		qr = ocrpt_query_get_result(q, &cols);

		if (row > 0)
			printf("\n");

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		for (i = 0; i < row_constify; i++) {
			if (i > 0)
				printf("\n");

			printf("Constified expressions from row %d of 'constify':\n", i + 1);
			for (j = 0; j < cols_constify; j++) {
				ocrpt_result *r;

				printf("Expression: ");
				ocrpt_expr_print(e[i][j]);
				r = ocrpt_expr_eval(e[i][j]);
				printf("Evaluated: ");
				ocrpt_result_print(r);
			}
		}
	}

	ocrpt_free(o);

	return 0;
}
