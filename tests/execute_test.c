/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

ocrpt_part *parts[16];
int nparts = 0;
ocrpt_report *reports[16];
int nreports = 0;

static void test_part_iteration_cb(opencreport *o, ocrpt_part *p, void *dummy UNUSED) {
	for (int i = 0; i < nparts; i++) {
		if (parts[i] == p) {
			printf("part %d iteration done\n", i);
			return;
		}
	}
	printf("unknown part???");
}

static void test_part_added_cb(opencreport *o, ocrpt_part *p, void *dummy UNUSED) {
	parts[nparts++] = p;
	printf("appended part %d\n", nparts - 1);
}

static void test_report_start_cb(opencreport *o, ocrpt_report *r, void *dummy UNUSED) {
	for (int i = 0; i < nreports; i++) {
		if (reports[i] == r) {
			printf("report %d started\n", i);
			return;
		}
	}
	printf("unknown report???");
}

static void test_report_done_cb(opencreport *o, ocrpt_report *r, void *dummy UNUSED) {
	for (int i = 0; i < nreports; i++) {
		if (reports[i] == r) {
			printf("report %d done\n", i);
			return;
		}
	}
	printf("unknown report???");
}

static void test_report_iteration_cb(opencreport *o, ocrpt_report *r, void *dummy UNUSED) {
	for (int i = 0; i < nreports; i++) {
		if (reports[i] == r) {
			printf("report %d iteration done\n", i);
			return;
		}
	}
	printf("unknown report???");
}

static void test_report_precalc_done_cb(opencreport *o, ocrpt_report *r, void *dummy UNUSED) {
	for (int i = 0; i < nreports; i++) {
		if (reports[i] == r) {
			printf("report %d precalculation done\n", i);
			return;
		}
	}
	printf("unknown report???");
}

static void test_precalc_done_cb(opencreport *o, void *dummy UNUSED) {
	printf("all reports' precalculation done\n");
}

static void test_report_added_cb(opencreport *o, ocrpt_report *r, void *dummy UNUSED) {
	reports[nreports++] = r;

	printf("appended report %d\n", nreports - 1);

	ocrpt_report_add_start_cb(o, r, test_report_start_cb, NULL);
	ocrpt_report_add_done_cb(o, r, test_report_done_cb, NULL);
	ocrpt_report_add_iteration_cb(o, r, test_report_iteration_cb, NULL);
	ocrpt_report_add_precalculation_done_cb(o, r, test_report_precalc_done_cb, NULL);
}

int main(void) {
	opencreport *o = ocrpt_init();

	ocrpt_add_precalculation_done_cb(o, test_precalc_done_cb, NULL);
	ocrpt_add_part_added_cb(o, test_part_added_cb, NULL);
	ocrpt_add_part_iteration_cb(o, test_part_iteration_cb, NULL);
	ocrpt_add_report_added_cb(o, test_report_added_cb, NULL);

	if (!ocrpt_parse_xml(o, "part_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_execute(o);

	ocrpt_free(o);

	return 0;
}
