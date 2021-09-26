/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.c"

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
	ocrpt_report *r;

	if (!ocrpt_parse_xml(o, "ocrpt_break_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	/* There is only one ocrpt_report pointer in o->parts, extract it. */
	r = (ocrpt_report *)((ocrpt_list *)(((ocrpt_part *)(o->parts->data))->rows->data))->data;

	if (r->breaks)
		printf("adding a break and a breakfield to it succeeded\n");

	ocrpt_free(o);

	return 0;
}
