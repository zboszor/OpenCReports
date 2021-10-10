/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

/* NOT static */ const char *array[6][6] = {
	{ "id", "id2", "name", "property", "age", "adult" },
	{ "1", "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", NULL, "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "3", "Pebbles Flintstone", "young", "5e-1", "no" },
	{ "4", NULL, "Betty Rubble", "beautiful", "27", "yes" },
	{ "5", "5", "Barney Rubble", "small", "29", "yes" }
};

/* NOT static */ const enum ocrpt_result_type coltypes[6] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

struct test_expr_types {
	const char *type_name;
	ocrpt_var_type type;
};

static char *test_vars[] = {
	"id2",
	"v.var_id_count",
	"v.var_id_countall",
	"v.var_id_sum",
	"v.var_id_highest",
	"v.var_id_lowest",
	"v.var_id_avg",
	"v.var_id_avgall",
	"v.var_id2_count",
	"v.var_id2_countall",
	"v.var_id2_sum",
	"v.var_id2_highest",
	"v.var_id2_lowest",
	"v.var_id2_avg",
	"v.var_id2_avgall"
};

#define N_TEST_VARS (sizeof(test_vars) / sizeof(char *))

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_report *r;
	ocrpt_break *br;
	ocrpt_expr *e[N_TEST_VARS];
	int32_t cols, row, i;

	if (!ocrpt_parse_xml(o, "ocrpt_variable_resetonbreak_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	q = ocrpt_query_get(o, "a");
	qr = ocrpt_query_get_result(q, &cols);

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	r = (ocrpt_report *)((ocrpt_list *)(((ocrpt_part *)(o->parts->data))->rows->data))->data;

	/* There is only one break in the report, extract it */
	br = (ocrpt_break *)r->breaks->data;

	for (i = 0; i < N_TEST_VARS; i++)
		e[i] = ocrpt_expr_parse(o, r, test_vars[i], NULL);

	printf("First run of the query\n\n");

	row = 0;
	ocrpt_query_navigate_start(o, q);
	ocrpt_report_resolve_breaks(o, r);
	ocrpt_report_resolve_variables(o, r);

	for (i = 0; i < N_TEST_VARS; i++) {
		ocrpt_expr_resolve(o, r, e[i]);
		ocrpt_expr_optimize(o, r, e[i]);
	}

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_result *rs;

		qr = ocrpt_query_get_result(q, &cols);

		if (ocrpt_break_check_fields(o, r, br)) {
			int32_t rownum;
			printf("Break triggers\n");

			ocrpt_expr_get_value(o, r->query_rownum, NULL, &rownum);
			if (rownum > 1)
				ocrpt_break_reset_vars(o, r, br);
		}

		ocrpt_report_evaluate_variables(o, r);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		for (i = 0; i < N_TEST_VARS; i++) {
			printf("Expression: ");
			ocrpt_expr_print(o, e[i]);
			rs = ocrpt_expr_eval(o, r, e[i]);
#if 0
			printf("Deep print: ");
			ocrpt_expr_result_deep_print(o, e[i]);
#endif
			printf("Evaluated: ");
			ocrpt_result_print(rs);
		}

		printf("\n");
	}

#if 0
	printf("Second run of the query\n\n");

	row = 0;
	ocrpt_query_navigate_start(o, q);
	ocrpt_report_resolve_breaks(o, r);
	ocrpt_report_resolve_variables(o, r);

	for (i = 0; i < N_TEST_VARS; i++) {
		ocrpt_expr_resolve(o, r, e[i]);
		ocrpt_expr_optimize(o, r, e[i]);
	}

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_result *rs;

		qr = ocrpt_query_get_result(q, &cols);

		if (ocrpt_break_check_fields(o, r, br))
			printf("Break triggers\n");

		ocrpt_report_evaluate_variables(o, r);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		for (i = 0; i < N_TEST_VARS; i++) {
			printf("Expression: ");
			ocrpt_expr_print(o, e[i]);
			rs = ocrpt_expr_eval(o, r, e[i]);
			printf("Evaluated: ");
			ocrpt_result_print(rs);
		}

		printf("\n");
	}
#endif

	for (i = 0; i < N_TEST_VARS; i++)
		ocrpt_expr_free(o, r, e[i]);

	ocrpt_free(o);

	return 0;
}
