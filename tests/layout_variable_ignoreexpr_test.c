/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 9
#define COLS 4
const char *array[ROWS + 1][COLS] = {
	{ "id", "firstname", "lastname", "age" },
	{ "1", "Fred", "Flintstone", "31" },
	{ "2", "Wilma", "Flintstone", "28" },
	{ "3", "Pebbles", "Flintstone", "5e-1" },
	{ "4", "Betty", "Rubble", "27" },
	{ "5", "Barney", "Rubble", "29" },
	{ "6", "George", "Slate", "53" },
	{ "7", "Joe", "Rockhead", "33" },
	{ "8", "Sam", "Slagheap", "37" },
	{ "9", "The Great", "Gazoo", "1200" },
};

const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_part *p = ocrpt_part_new(o);
	ocrpt_part_row *pr = ocrpt_part_new_row(p);
	ocrpt_part_column *pd = ocrpt_part_row_new_column(pr);
	ocrpt_report *r = ocrpt_part_column_new_report(pd);
	ocrpt_break *b;

	ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);

	ocrpt_variable_new(r, OCRPT_VARIABLE_HIGHEST, "var1", "age", NULL, NULL, true);
	ocrpt_variable_new(r, OCRPT_VARIABLE_AVERAGE, "var2", "age", NULL, NULL, true);
	/*
	 * var3 is intentionally non-precalculated so
	 * an indirect reference to a precalculated variable
	 * is exercised.
	 */
	ocrpt_variable_new(r, OCRPT_VARIABLE_EXPRESSION, "var3", "0.9 * v.var1", NULL, NULL, false);
	ocrpt_variable_new(r, OCRPT_VARIABLE_AVERAGE, "var4", "age", "age > v.var3", "br1", true);
	ocrpt_variable_new(r, OCRPT_VARIABLE_AVERAGE, "var5", "age", "age > v.var3", NULL, true);

	b = ocrpt_break_new(r, "br1");
	ocrpt_expr *e = ocrpt_report_expr_parse(r, "lastname", NULL);
	ocrpt_break_add_breakfield(b, e);

	/* Construct a minimal layout */
	ocrpt_part_set_orientation(p, "landscape");
	ocrpt_part_set_font_size(p, "10");

	ocrpt_output *out = ocrpt_layout_report_field_details(r);
	ocrpt_line *l = ocrpt_output_add_line(out);

	ocrpt_text *t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "id");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, " ");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "firstname");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, " ");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "lastname");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, " ");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "age");
	ocrpt_text_set_format(t, "'%.3d'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, " ");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "v.var1");
	ocrpt_text_set_format(t, "'%.6d'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, " ");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "v.var2");
	ocrpt_text_set_format(t, "'%.6d'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, " ");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "v.var3");
	ocrpt_text_set_format(t, "'%.6d'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, " ");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "v.var4");
	ocrpt_text_set_format(t, "'%.6d'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, " ");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "v.var5");
	ocrpt_text_set_format(t, "'%.6d'");

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
