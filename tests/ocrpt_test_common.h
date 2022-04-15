/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _OCRPT_TEST_COMMON_H_
#define _OCRPT_TEST_COMMON_H_

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

static void print_result_row(const char *name, ocrpt_query_result *qr, int32_t cols) UNUSED;

static void print_result_row(const char *name, ocrpt_query_result *qr, int32_t cols) {
	int i;

	printf("Query: '%s':\n", name);
	for (i = 0; i < cols; i++) {
		printf("\tCol #%d: '%s': string value: %s", i, qr[i].name, (qr[i].result.isnull || !qr[i].result.string) ? "NULL" : qr[i].result.string->str);
		if (!qr[i].result.isnull && qr[i].result.number_initialized)
			mpfr_printf(" (converted to number: %.6RF)", qr[i].result.number);
		printf("\n");
	}
}

static void print_part_reports(char *name, ocrpt_part *p) UNUSED;
static void print_part_reports(char *name, ocrpt_part *p) {
	ocrpt_list *row, *col;
	int i, j;

	printf("part %s:\n", name);
	for (row = p->rows, i = 0; row; row = row->next, i++) {
		printf("row %d reports:", i);
		for (col = (ocrpt_list *)row->data, j = 0; col; col = col->next, j++) {
			ocrpt_report *r = (ocrpt_report *) col->data;
			if (r)
				printf(" %d", j);
		}
		printf("\n");
	}
}

#endif
