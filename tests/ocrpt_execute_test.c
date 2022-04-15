/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

static void test_report_start_cb(opencreport *o, ocrpt_report *r, void *dummy UNUSED) {
	fprintf(stderr, "report %p is executing\n", r);
}

static void test_report_added_cb(opencreport *o, ocrpt_report *r, void *dummy UNUSED) {
	fprintf(stderr, "appended ocrpt_report %p\n", r);

	ocrpt_report_add_start_cb(o, r, test_report_start_cb, NULL);
}

int main(void) {
	opencreport *o = ocrpt_init();

	ocrpt_add_report_added_cb(o, test_report_added_cb, NULL);

	if (!ocrpt_parse_xml(o, "ocrpt_part_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_execute(o);

	ocrpt_free(o);

	return 0;
}
