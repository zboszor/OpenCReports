/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

/* NOT static */ const char *array[4][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

/* NOT static */ const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "break_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	ocrpt_part *p = (ocrpt_part *)o->parts->data;
	ocrpt_part_row *pr = (ocrpt_part_row *)p->rows->data;
	ocrpt_part_row_data *pd = (ocrpt_part_row_data *)pr->pd_list->data;
	ocrpt_report *r = (ocrpt_report *)pd->reports->data;

	if (r->breaks)
		printf("adding a break and a breakfield to it succeeded\n");

	ocrpt_free(o);

	return 0;
}
