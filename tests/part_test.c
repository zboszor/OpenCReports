/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"

#include "test_common.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_part *p1 UNUSED;
	ocrpt_part *p2 UNUSED;
	ocrpt_part_row *pr1 UNUSED;
	ocrpt_part_row *pr2 UNUSED;
	ocrpt_part_row_data *pd1 UNUSED;
	ocrpt_part_row_data *pd2 UNUSED;
	ocrpt_report *r1 UNUSED;
	ocrpt_report *r2 UNUSED;

	p1 = ocrpt_part_new(o);
	p2 = ocrpt_part_new(o);

	printf("Allocated two parts for report structure\n");

	/* Both parts will be freed implicitly */
	ocrpt_free(o);

	printf("Freed report structure\n");

	o = ocrpt_init();

	p1 = ocrpt_part_new(o);
	r1 = ocrpt_report_new(o);
	ocrpt_part_append_report(o, p1, NULL, NULL, r1);

	p2 = ocrpt_part_new(o);
	r2 = ocrpt_report_new(o);
	ocrpt_part_append_report(o, p2, NULL, NULL, r2);

	printf("Allocated two parts and one report for each part\n");

	print_part_reports("p1", p1);
	print_part_reports("p2", p2);

	ocrpt_free(o);

	printf("Freed report structure\n");

	o = ocrpt_init();

	p1 = ocrpt_part_new(o);

	p2 = ocrpt_part_new(o);
	pr2 = ocrpt_part_new_row(o, p2);
	pd2 = ocrpt_part_row_new_data(o, p2, pr2);
	r1 = ocrpt_report_new(o);
	ocrpt_part_append_report(o, p2, pr2, pd2, r1);

	r2 = ocrpt_report_new(o);
	ocrpt_part_append_report(o, p2, pr2, pd2, r2);

	printf("Allocated two parts and two reports for the second part\n");

	print_part_reports("p1", p1);
	print_part_reports("p2", p2);

	ocrpt_free(o);

	printf("Freed report structure\n");

	return 0;
}