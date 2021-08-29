/*
 * OpenCReports test
 * Copyright (C) 2021-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"

#include "ocrpt_test_common.c"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_part *p1 UNUSED;
	ocrpt_part *p2 UNUSED;
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
	r1 = ocrpt_report_new();
	ocrpt_part_append_report(o, p1, r1);

	p2 = ocrpt_part_new(o);
	r2 = ocrpt_report_new();
	ocrpt_part_append_report(o, p2, r2);

	printf("Allocated two parts and one report for each part\n");

	print_part_reports("p1", p1);
	print_part_reports("p2", p2);

	ocrpt_free(o);

	printf("Freed report structure\n");

	o = ocrpt_init();

	p1 = ocrpt_part_new(o);

	p2 = ocrpt_part_new(o);
	r1 = ocrpt_report_new();
	ocrpt_part_append_report(o, p2, r1);

	r2 = ocrpt_report_new();
	ocrpt_part_append_report(o, p2, r2);

	printf("Allocated two parts and two reports for the second part\n");

	print_part_reports("p1", p1);
	print_part_reports("p2", p2);

	ocrpt_free(o);

	printf("Freed report structure\n");

	return 0;
}
