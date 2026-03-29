/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>

#define ROWS 120
#define COLS 6
const char *array[ROWS + 1][COLS] = {
	{ "id", "name", "property", "age", "male", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "2", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "3", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "4", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "5", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "6", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "7", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "8", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "9", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "10", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "11", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "12", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "13", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "14", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "15", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "16", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "17", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "18", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "19", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "20", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "21", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "22", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "23", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "24", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "25", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "26", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "27", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "28", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "29", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "30", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "31", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "32", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "33", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "34", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "35", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "36", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "37", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "38", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "39", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "40", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "41", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "42", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "43", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "44", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "45", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "46", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "47", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "48", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "49", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "50", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "51", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "52", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "53", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "54", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "55", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "56", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "57", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "58", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "59", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "60", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "1", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "2", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "3", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "4", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "5", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "6", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "7", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "8", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "9", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "10", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "11", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "12", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "13", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "14", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "15", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "16", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "17", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "18", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "19", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "20", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "21", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "22", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "23", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "24", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "25", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "26", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "27", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "28", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "29", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "30", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "31", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "32", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "33", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "34", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "35", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "36", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "37", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "38", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "39", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "40", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "41", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "42", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "43", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "44", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "45", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "46", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "47", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "48", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "49", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "50", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "51", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "52", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "53", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "54", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "55", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "56", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "57", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "58", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "59", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "60", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
};

const int32_t coltypes[6] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	ocrpt_datasource *ds = ocrpt_datasource_add(o, "myarray", "array", NULL);
	ocrpt_query *q = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);

	ocrpt_part *p = ocrpt_part_new(o);
	ocrpt_part_set_paper_type(p, "A4");
	ocrpt_part_set_font_size(p, "10.0");

	/* Construct page header */

	ocrpt_output *out = ocrpt_layout_part_page_header(p);

	ocrpt_hline *hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "1");
	ocrpt_hline_set_color(hl, "'red'");

	ocrpt_line *l = ocrpt_output_add_line(out);

	ocrpt_text *t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Page header");

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "1");
	ocrpt_hline_set_color(hl, "'red'");

	/* Construct page footer */

	out = ocrpt_layout_part_page_footer(p);

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "1");
	ocrpt_hline_set_color(hl, "'green'");

	l = ocrpt_output_add_line(out);

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Page footer");

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "1");
	ocrpt_hline_set_color(hl, "'green'");

	/* End of page header / footer */

	ocrpt_part_row *pr = ocrpt_part_new_row(p);

	ocrpt_part_column *pd = ocrpt_part_row_new_column(pr);
	ocrpt_part_column_set_detail_columns(pd, "2");
	ocrpt_part_column_set_column_padding(pd, "0.2");

	ocrpt_report *r = ocrpt_part_column_new_report(pd);
	ocrpt_report_set_main_query(r, q);
	ocrpt_report_set_font_size(r, "12.0");

	/* Construct field header */

	out = ocrpt_layout_report_field_header(r);

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "1");
	ocrpt_hline_set_color(hl, "'black'");

	l = ocrpt_output_add_line(out);
	ocrpt_line_set_bgcolor(l, "'0xe5e5e5'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "ID");
	ocrpt_text_set_width(t, "4");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Name");
	ocrpt_text_set_width(t, "20");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Property");
	ocrpt_text_set_width(t, "10");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Age");
	ocrpt_text_set_width(t, "6");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Adult");
	ocrpt_text_set_width(t, "5");

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "1");
	ocrpt_hline_set_color(hl, "'black'");

	/* Construct field details */

	out = ocrpt_layout_report_field_details(r);

	l = ocrpt_output_add_line(out);
	ocrpt_line_set_bgcolor(l, "iif(r.detailcnt%2,'0xe5e5e5','white')");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "id");
	ocrpt_text_set_width(t, "4");
	ocrpt_text_set_alignment(t, "'right'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "name");
	ocrpt_text_set_width(t, "20");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "property");
	ocrpt_text_set_width(t, "10");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "age");
	ocrpt_text_set_width(t, "6");
	ocrpt_text_set_alignment(t, "'right'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "adult ? 'yes' : 'no'");
	ocrpt_text_set_width(t, "5");
	ocrpt_text_set_alignment(t, "'center'");

	/* Construct field footer */

	out = ocrpt_layout_report_field_footer(r);

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "1");
	ocrpt_hline_set_color(hl, "'black'");

	l = ocrpt_output_add_line(out);

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "This is a field footer before the page ends.");

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "1");
	ocrpt_hline_set_color(hl, "'black'");

	/* End of field header / details / footer */

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
