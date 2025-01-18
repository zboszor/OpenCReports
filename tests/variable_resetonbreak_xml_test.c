/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

/* NOT static */ const char *array[6][6] = {
	{ "id", "id2", "name", "property", "age", "adult" },
	{ "1", "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", NULL, "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "3", "Pebbles Flintstone", "young", "5e-1", "no" },
	{ "4", NULL, "Betty Rubble", "beautiful", "27", "yes" },
	{ "5", "5", "Barney Rubble", "small", "29", "yes" }
};

/* NOT static */ const int32_t coltypes[6] = {
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

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_query *q;
	ocrpt_query_result *qr;
	ocrpt_expr *e[N_TEST_VARS];
	int32_t cols, row, i;

	if (!ocrpt_parse_xml(o, "variable_resetonbreak_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	q = ocrpt_query_get(o, "a");
	qr = ocrpt_query_get_result(q, &cols);

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	ocrpt_report *r = get_first_report(o);
	ocrpt_report_set_main_query(r, q);

	/* There is only one break in the report, extract it */
	ocrpt_list *brl = NULL;
	ocrpt_break *br = ocrpt_break_get_next(r, &brl);

	for (i = 0; i < N_TEST_VARS; i++)
		e[i] = ocrpt_report_expr_parse(r, test_vars[i], NULL);

	printf("First run of the query\n\n");

	row = 0;
	ocrpt_query_navigate_start(q);
	ocrpt_report_resolve_breaks(r);
	ocrpt_report_resolve_variables(r);

	for (i = 0; i < N_TEST_VARS; i++) {
		ocrpt_expr_resolve(e[i]);
		ocrpt_expr_optimize(e[i]);
	}

	while (ocrpt_query_navigate_next(q)) {
		ocrpt_result *rs;

		qr = ocrpt_query_get_result(q, &cols);

		if (ocrpt_break_check_fields(br)) {
			long rownum;
			printf("Break triggers\n");

			rownum = ocrpt_report_get_query_rownum(r);
			if (rownum > 1)
				ocrpt_break_reset_vars(br);
		}

		ocrpt_report_evaluate_variables(r);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		for (i = 0; i < N_TEST_VARS; i++) {
			printf("Expression: ");
			ocrpt_expr_print(e[i]);
			rs = ocrpt_expr_eval(e[i]);
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
	ocrpt_query_navigate_start(q);
	ocrpt_report_resolve_breaks(r);
	ocrpt_report_resolve_variables(r);

	for (i = 0; i < N_TEST_VARS; i++) {
		ocrpt_expr_resolve(e[i]);
		ocrpt_expr_optimize(e[i]);
	}

	while (ocrpt_query_navigate_next(q)) {
		ocrpt_result *rs;

		qr = ocrpt_query_get_result(q, &cols);

		if (ocrpt_break_check_fields(br))
			printf("Break triggers\n");

		ocrpt_report_evaluate_variables(r);

		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		for (i = 0; i < N_TEST_VARS; i++) {
			printf("Expression: ");
			ocrpt_expr_print(e[i]);
			rs = ocrpt_expr_eval(e[i]);
			printf("Evaluated: ");
			ocrpt_result_print(rs);
		}

		printf("\n");
	}
#endif

	for (i = 0; i < N_TEST_VARS; i++)
		ocrpt_expr_free(e[i]);

	ocrpt_free(o);

	return 0;
}
