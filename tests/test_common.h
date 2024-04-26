/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
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
		const char *name = ocrpt_query_result_column_name(qr, i);
		ocrpt_result *r = ocrpt_query_result_column_result(qr, i);
		ocrpt_string *s = ocrpt_result_get_string(r);
		bool isnull = ocrpt_result_isnull(r);
		bool isnumber = ocrpt_result_isnumber(r);

		printf("\tCol #%d: '%s': string value: %s", i, name, (isnull || !s) ? "NULL" : s->str);
		if (!isnull && isnumber) {
			mpfr_ptr number = ocrpt_result_get_number(r);
			mpfr_printf(" (converted to number: %.6RF)", number);
		}
		printf("\n");
	}
}

static void print_part_reports(char *name, ocrpt_part *p) UNUSED;
static void print_part_reports(char *name, ocrpt_part *p) {
	ocrpt_list *prl = NULL;
	ocrpt_part_row *pr;
	int i, j;

	printf("part %s:\n", name);
	for (pr = ocrpt_part_row_get_next(p, &prl), i = 0; pr; pr = ocrpt_part_row_get_next(p, &prl), i++) {
		ocrpt_list *pdl = NULL;

		printf("row %d reports:", i);
		j = 0;
		for (ocrpt_part_column *pd = ocrpt_part_column_get_next(pr, &pdl); pd; pd = ocrpt_part_column_get_next(pr, &pdl)) {
			ocrpt_list *rl = NULL;

			for (ocrpt_report *r = ocrpt_report_get_next(pd, &rl); r; r = ocrpt_report_get_next(pd, &rl), j++)
				printf(" %d", j);
		}
		printf("\n");
	}
}

static ocrpt_report *get_first_report(opencreport *o) UNUSED;
static ocrpt_report *get_first_report(opencreport *o) {
	ocrpt_list *l;

	l = NULL;
	ocrpt_part *p = ocrpt_part_get_next(o, &l);

	l = NULL;
	ocrpt_part_row *pr = ocrpt_part_row_get_next(p, &l);

	l = NULL;
	ocrpt_part_column *pd = ocrpt_part_column_get_next(pr, &l);

	l = NULL;
	ocrpt_report *r = ocrpt_report_get_next(pd, &l);

	return r;
}

#endif
