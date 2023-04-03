/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"

#include "test_common.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_part *p1;
	ocrpt_part *p2;
	ocrpt_part_row *pr1;
	ocrpt_part_row *pr2;
	ocrpt_part_column *pd1;
	ocrpt_part_column *pd2;

	p1 = ocrpt_part_new(o);
	p2 = ocrpt_part_new(o);

	printf("Allocated two parts for report structure\n");

	/* Both parts will be freed implicitly */
	ocrpt_free(o);

	printf("Freed report structure\n");

	o = ocrpt_init();

	p1 = ocrpt_part_new(o);
	pr1 = ocrpt_part_new_row(p1);
	pd1 = ocrpt_part_row_new_column(pr1);
	ocrpt_part_column_new_report(pd1);

	p2 = ocrpt_part_new(o);
	pr2 = ocrpt_part_new_row(p2);
	pd2 = ocrpt_part_row_new_column(pr2);
	ocrpt_part_column_new_report(pd2);

	printf("Allocated two parts and one report for each part\n");

	print_part_reports("p1", p1);
	print_part_reports("p2", p2);

	ocrpt_free(o);

	printf("Freed report structure\n");

	o = ocrpt_init();

	p1 = ocrpt_part_new(o);

	p2 = ocrpt_part_new(o);
	pr2 = ocrpt_part_new_row(p2);
	pd2 = ocrpt_part_row_new_column(pr2);
	ocrpt_part_column_new_report(pd2);
	ocrpt_part_column_new_report(pd2);

	printf("Allocated two parts and two reports for the second part\n");

	print_part_reports("p1", p1);
	print_part_reports("p2", p2);

	ocrpt_free(o);

	printf("Freed report structure\n");

	return 0;
}
