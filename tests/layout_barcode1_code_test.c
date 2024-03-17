/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>

#define ROWS 20
#define COLS 3

const char *array[ROWS + 1][COLS] = {
	{ "id", "bc", "type" },
	{ "1",  "123456789010", "ean-13" },
	{ "2",  "123456789011", "ean-13" },
	{ "3",  "123456789012", "ean-13" },
	{ "4",  "123456789013", "ean-13" },
	{ "5",  "123456789010A", "code39" },
	{ "6",  "123456789011B", "code39" },
	{ "7",  "123456789012C", "code39" },
	{ "8",  "123456789013D", "code39" },
	{ "9",  "123456789010Aa", "code128b" },
	{ "10", "123456789011Ba", "code128b" },
	{ "11", "123456789012Ca", "code128b" },
	{ "12", "123456789013Da", "code128b" },
	{ "13", "123456789010", "code128c" },
	{ "14", "123456789011", "code128c" },
	{ "15", "123456789012", "code128c" },
	{ "16", "123456789013", "code128c" },
	{ "17", "123456789010Aa", "code128" },
	{ "18", "123456789011Ba", "code128" },
	{ "19", "123456789012Ca", "code128" },
	{ "20", "123456789013Da", "code128" },
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");

	ocrpt_query_add_array(ds, "data", (const char **)array, ROWS, COLS, NULL, 0);

	ocrpt_part *p = ocrpt_part_new(o);
	ocrpt_part_row *pr = ocrpt_part_new_row(p);
	ocrpt_part_column *pd = ocrpt_part_row_new_column(pr);
	ocrpt_report *r = ocrpt_part_column_new_report(pd);

	ocrpt_output *fh = ocrpt_layout_report_field_header(r);

	ocrpt_line *l = ocrpt_output_add_line(fh);

	ocrpt_text *t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "ID");
	ocrpt_text_set_width(t, "4");
	ocrpt_text_set_alignment(t, "'right'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");
	ocrpt_text_set_bgcolor(t, "'red'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "String");
	ocrpt_text_set_width(t, "20");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");
	ocrpt_text_set_bgcolor(t, "'red'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_string(t, "Barcode");
	ocrpt_text_set_width(t, "20");
	ocrpt_text_set_alignment(t, "'center'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");
	ocrpt_text_set_bgcolor(t, "'red'");

	ocrpt_hline *hl = ocrpt_output_add_hline(fh);
	ocrpt_hline_set_color(hl, "'red'");

	ocrpt_output *fd = ocrpt_layout_report_field_details(r);

	l = ocrpt_output_add_line(fd);

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "id");
	ocrpt_text_set_width(t, "4");
	ocrpt_text_set_alignment(t, "'right'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");
	ocrpt_text_set_bgcolor(t, "'red'");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_value_expr(t, "bc");
	ocrpt_text_set_width(t, "20");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");
	ocrpt_text_set_bgcolor(t, "'red'");

	ocrpt_barcode *bc = ocrpt_line_add_barcode(l);
	ocrpt_barcode_set_value(bc, "bc");
	ocrpt_barcode_set_type(bc, "type");
	ocrpt_barcode_set_width(bc, "20");

	t = ocrpt_line_add_text(l);
	ocrpt_text_set_width(t, "1");
	ocrpt_text_set_bgcolor(t, "'red'");

	hl = ocrpt_output_add_hline(fd);
	ocrpt_hline_set_color(hl, "'red'");

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
