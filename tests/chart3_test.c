/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"
#include "test_common.h"

#define ROWS 6
#define COLS 5
static const char *array[ROWS + 1][COLS] = {
	{ "id", "family", "name", "age_group", "sex" },
	{ "1", "Flintstone", "Fred", "adult", "man" },
	{ "1", "Flintstone", "Wilma", "adult", "woman" },
	{ "1", "Flintstone", "Pebbles", "infant", "girl" },
	{ "2", "Rubble", "Barney", "adult", "man" },
	{ "2", "Rubble", "Betty", "adult", "woman" },
	{ "2", "Rubble", "Bamm-Bamm", "infant", "boy" }
};

/*
 * This accidentally caught a problem with missing elements initialized to 0.
 * Fixed in commit "Reorder enum ocrpt_result_type and enum ocrpt_expr_type"
 */
static const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING //, OCRPT_RESULT_STRING
};

#define ROWS1 6
#define COLS1 1
static const char *agegroup_sex[ROWS1 + 1][COLS1] = {
	{ "age_group_sex" },
	{ "infant girl" },
	{ "infant boy" },
	{ "teenage girl" },
	{ "teenage boy" },
	{ "adult woman" },
	{ "adult man" }
};

static const int32_t coltypes2[COLS1] = { OCRPT_RESULT_STRING };

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_part *p = ocrpt_part_new(o);
	ocrpt_part_row *pr = ocrpt_part_new_row(p);
	ocrpt_part_column *pd = ocrpt_part_row_new_column(pr);
	ocrpt_report *r = ocrpt_part_column_new_report(pd);
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q1 = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);
	ocrpt_query *q2 = ocrpt_query_add_data(ds, "b", (const char **)agegroup_sex, ROWS1, COLS1, coltypes2, COLS1);
	ocrpt_query_result *qr;
	int32_t cols, i;

	//ocrpt_report_set_main_query(r, q1);

	ocrpt_chart *c = ocrpt_layout_report_chart(r);
	ocrpt_chart_set_name(c, "chart1");

	/* Expression validity test */
	ocrpt_chart_set_title(c, "'My test chart'");
	ocrpt_chart_set_cell_width(c, "200");
	ocrpt_chart_set_cell_height(c, "20");
	ocrpt_chart_set_cell_width_padding(c, "2");
	ocrpt_chart_set_cell_height_padding(c, "2");
	ocrpt_chart_set_label_width(c, "200");
	ocrpt_chart_set_header_row_enabled(c, "true");

	ocrpt_chart_set_header_row_query(c, q2);
	ocrpt_expr *age_sex_from_header = ocrpt_chart_set_header_row_field(c, "age_group_sex");
	ocrpt_expr_result_deep_print(age_sex_from_header);
	ocrpt_chart_set_header_row_colspan(c, "1");

	ocrpt_chart_set_row(c, "id");
	ocrpt_expr *age_sex1_from_rows = ocrpt_chart_set_bar_start(c, "age_group + ' ' + sex");
	ocrpt_expr_result_deep_print(age_sex1_from_rows);
	ocrpt_expr *age_sex2_from_rows = ocrpt_chart_set_bar_end(c, "age_group + ' ' + sex");
	ocrpt_expr_result_deep_print(age_sex2_from_rows);
	ocrpt_chart_set_label(c, "family");
	ocrpt_chart_set_bar_label(c, "name");
	ocrpt_chart_set_label_color(c, "'black'");
	ocrpt_chart_set_bar_color(c, "'blue'");
	ocrpt_chart_set_bar_label_color(c, "'white'");

	ocrpt_report_resolve_chart(c);
	ocrpt_report_resolve_expressions(r);

	printf("After ocrpt_report_resolve_chart() chart is %s\n", ocrpt_chart_is_valid(c) ? "valid" : "invalid");

	qr = ocrpt_query_get_result(q2, &cols);
	printf("Query columns of 'b':\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr, i));
	printf("End of Query columns of 'b'\n");

	int32_t row = 0;
	ocrpt_query_navigate_start(q2);

	while (ocrpt_query_navigate_next(q2)) {
		qr = ocrpt_query_get_result(q2, &cols);
		printf("Row #%d\n", row++);
		print_result_row("b", qr, cols);

		printf("\n");

		ocrpt_expr_eval(age_sex_from_header);
		ocrpt_expr_result_deep_print(age_sex_from_header);

		printf("\n");
	}

	printf("\n");

	qr = ocrpt_query_get_result(q1, &cols);
	printf("Query columns of 'a':\n");
	for (i = 0; i < cols; i++)
		printf("%d: '%s'\n", i, ocrpt_query_result_column_name(qr, i));
	printf("End of Query columns of 'a'\n");

	row = 0;
	ocrpt_query_navigate_start(q1);

	while (ocrpt_query_navigate_next(q1)) {
		qr = ocrpt_query_get_result(q1, &cols);
		printf("Row #%d\n", row++);
		print_result_row("a", qr, cols);

		printf("\n");

		ocrpt_expr_eval(age_sex1_from_rows);
		ocrpt_expr_result_deep_print(age_sex1_from_rows);

		ocrpt_expr_eval(age_sex2_from_rows);
		ocrpt_expr_result_deep_print(age_sex2_from_rows);
	}

	ocrpt_free(o);

	printf("Freed report structure\n");

	return 0;
}
