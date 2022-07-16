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
	ocrpt_list *prl, *pdl, *rl;
	int i, j;

	printf("part %s:\n", name);
	for (prl = p->rows, i = 0; prl; prl = prl->next, i++) {
		ocrpt_part_row *pr = (ocrpt_part_row *)prl->data;

		printf("row %d reports:", i);
		j = 0;
		for (pdl = pr->pd_list; pdl; pdl = pdl->next) {
			ocrpt_part_row_data *pd = (ocrpt_part_row_data *)pdl->data;

			for (rl = pd->reports; rl; rl = rl->next, j++)
				printf(" %d", j);
		}
		printf("\n");
	}
}

#endif
