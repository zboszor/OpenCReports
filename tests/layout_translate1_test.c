/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <opencreport.h>

#define ROWS 9
#define COLS 4
const char *array[ROWS + 1][COLS] = {
	{ "id", "name", "age", "adult" },
	{ "1", "Fred Flintstone", "31", "yes" },
	{ "2", "Wilma Flintstone", "28", "yes" },
	{ "3", "Pebbles Flintstone", "5e-1", "no" },
	{ "4", "Betty Rubble", "27", "yes" },
	{ "5", "Barney Rubble", "29", "yes" },
	{ "6", "Mr. George Slate", "53", "yes" },
	{ "7", "Joe Rockhead", "33", "yes" },
	{ "8", "Sam Slagheap", "37", "yes" },
	{ "9", "The Great Gazoo", "1200", "yes" },
};

const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	char blddir0[PATH_MAX];
	char *abs_builddir = getenv("abs_builddir");
	if (!abs_builddir || !*abs_builddir)
		abs_builddir = getcwd(blddir0, sizeof(blddir0));
	char *blddir = ocrpt_canonicalize_path(abs_builddir);
	ocrpt_string *localedir = ocrpt_mem_string_new(blddir, true);
	ocrpt_mem_string_append(localedir, "/locale");

	ocrpt_mem_free(blddir);

	opencreport *o = ocrpt_init();

	ocrpt_bindtextdomain(o, "translate_test", localedir->str);

	ocrpt_set_locale(o, "hu_HU");

	ocrpt_mem_string_free(localedir, true);

	ocrpt_datasource *ds = ocrpt_datasource_add(o, "myarray", "array", NULL);
	ocrpt_query *q = ocrpt_query_add_data(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);

	ocrpt_part *p = ocrpt_part_new(o);
	ocrpt_part_set_paper_by_name(p, "A4");
	ocrpt_part_set_orientation(p, "'landscape'");
	ocrpt_part_set_font_size(p, "10.0");

	/* Construct page header */
	ocrpt_output *out = ocrpt_layout_part_page_header(p);

	ocrpt_hline *hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "2");
	ocrpt_hline_set_color(hl, "'black'");

	ocrpt_line *l = ocrpt_output_add_line(out);
	ocrpt_line_set_font_size(l, "14");

	ocrpt_text *t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "printf(translate('Page header: %d / %d'), r.pageno, r.totpages)");
	ocrpt_text_set_alignment(t, "'right'");

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "2");
	ocrpt_hline_set_color(hl, "'black'");

	/* Construct page footer */
	out = ocrpt_layout_part_page_footer(p);

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "2");
	ocrpt_hline_set_color(hl, "'black'");

	l = ocrpt_output_add_line(out);
	ocrpt_line_set_font_size(l, "14");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "printf(translate('Page footer: %d / %d'), r.pageno, r.totpages)");
	ocrpt_text_set_alignment(t, "'right'");

	hl = ocrpt_output_add_hline(out);
	ocrpt_hline_set_size(hl, "2");
	ocrpt_hline_set_color(hl, "'black'");

	/* End of page header / footer */

	ocrpt_part_row *pr = ocrpt_part_new_row(p);

	ocrpt_part_column *pd = ocrpt_part_row_new_column(pr);
	ocrpt_part_column_set_border_width(pd, "2");
	ocrpt_part_column_set_border_color(pd, "bobkratz");

	ocrpt_report *r = ocrpt_part_column_new_report(pd);
	ocrpt_report_set_main_query(r, q);
	ocrpt_report_set_font_size(r, "12.0");

	/* Construct field headers */

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
	ocrpt_text_set_value_string(t, "Greetings");
	ocrpt_text_set_translate(t, "yes");
	ocrpt_text_set_width(t, "50");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Age");
	ocrpt_text_set_translate(t, "yes");
	ocrpt_text_set_width(t, "7");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Adult");
	ocrpt_text_set_translate(t, "yes");
	ocrpt_text_set_width(t, "7");
	ocrpt_text_set_alignment(t, "'center'");

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

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "name");
	ocrpt_text_set_format(t, "'Happy birthday, %s!'");
	ocrpt_text_set_translate(t, "yes");
	ocrpt_text_set_width(t, "50");
	ocrpt_text_set_alignment(t, "'justified'");
	ocrpt_text_set_memo(t, "yes");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "age");
	ocrpt_text_set_width(t, "7");
	ocrpt_text_set_format(t, "'%.2d'");
	ocrpt_text_set_alignment(t, "'right'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "adult ? 'yes' : 'no'");
	ocrpt_text_set_translate(t, "yes");
	ocrpt_text_set_width(t, "7");
	ocrpt_text_set_alignment(t, "'center'");

	/* End of field headers / details */

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
