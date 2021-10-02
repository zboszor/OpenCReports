/*
 * OpenCReports test
 * Copyright (C) 2021-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.c"

/* NOT static */ const char *array[4][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

/* NOT static */ const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

struct test_expr_types {
	const char *type_name;
	ocrpt_var_type type;
};

static char *test_vars[] = {
	"v.var_id_count",
	"v.var_id_countall",
	"v.var_id_sum",
	"v.var_id_highest",
	"v.var_id_lowest",
	"v.var_age_avg"
};

#define N_TEST_VARS (sizeof(test_vars) / sizeof(char *))

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_report *r;
	ocrpt_expr *e[N_TEST_VARS];
	int32_t cols, row, i;

	if (!ocrpt_parse_xml(o, "ocrpt_variable_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	q = ocrpt_query_find(o, "a");
	qr = ocrpt_query_get_result(q, &cols);

	for (i = 0; i < N_TEST_VARS; i++)
		e[i] = ocrpt_expr_parse(o, test_vars[i], NULL);

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	r = (ocrpt_report *)((ocrpt_list *)(((ocrpt_part *)(o->parts->data))->rows->data))->data;

	row = 0;
	ocrpt_query_navigate_start(o, q);
	ocrpt_report_resolve_variables(o, r);

	for (i = 0; i < N_TEST_VARS; i++) {
		ocrpt_expr_resolve(o, r, e[i]);
		ocrpt_expr_optimize(o, r, e[i]);
	}

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_result *rs;

		qr = ocrpt_query_get_result(q, &cols);

		ocrpt_report_evaluate_variables(o, r);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		ocrpt_report_evaluate_variables(o, r);

		for (i = 0; i < N_TEST_VARS; i++) {
			printf("Expression: ");
			ocrpt_expr_print(o, e[i]);
			rs = ocrpt_expr_eval(o, r, e[i]);
			printf("Evaluated: ");
			ocrpt_result_print(rs);
		}

		printf("\n");
	}

	for (i = 0; i < N_TEST_VARS; i++)
		ocrpt_expr_free(e[i]);

	ocrpt_free(o);

	return 0;
}
