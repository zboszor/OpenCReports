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

	printf("Chart pointer is %s\n", c ? "valid" : "invalid");

	ocrpt_free(o);

	printf("Freed report structure\n");

	return 0;
}
