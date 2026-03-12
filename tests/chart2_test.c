/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"
#include "test_common.h"

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_part *p = ocrpt_part_new(o);
	ocrpt_part_row *pr = ocrpt_part_new_row(p);
	ocrpt_part_column *pd = ocrpt_part_row_new_column(pr);
	ocrpt_report *r = ocrpt_part_column_new_report(pd);

	printf("Allocated a report part with one row, one column and one report\n");

	print_part_reports("p", p);

	ocrpt_chart *c = ocrpt_layout_report_chart(r);

	ocrpt_chart_set_name(c, "chart1");

	ocrpt_chart *c2 = ocrpt_chart_get_by_name(o, "chart1");

	printf("Chart pointer is %s\n", (c == c2) ? "valid" : "invalid");

	/* Expression leak test */
	ocrpt_chart_set_title(c, "1");
	ocrpt_chart_set_cell_width(c, "1");
	ocrpt_chart_set_cell_height(c, "1");
	ocrpt_chart_set_cell_width_padding(c, "1");
	ocrpt_chart_set_cell_height_padding(c, "1");
	ocrpt_chart_set_label_width(c, "1");
	ocrpt_chart_set_header_row_enabled(c, "1");

	ocrpt_chart_set_header_row_field(c, "1");
	ocrpt_chart_set_header_row_colspan(c, "1");

	ocrpt_chart_set_row(c, "1");
	ocrpt_chart_set_bar_start(c, "1");
	ocrpt_chart_set_bar_end(c, "1");
	ocrpt_chart_set_label(c, "1");
	ocrpt_chart_set_bar_label(c, "1");
	ocrpt_chart_set_label_color(c, "1");
	ocrpt_chart_set_bar_color(c, "1");
	ocrpt_chart_set_bar_label_color(c, "1");

	ocrpt_free(o);

	printf("Freed report structure\n");

	return 0;
}
