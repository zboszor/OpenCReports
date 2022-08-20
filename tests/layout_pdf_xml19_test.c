/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
//#include "test_common.h"

const char *array[6][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" },
	{ "4", "Betty Rubble", "beautiful", "27", "yes" },
	{ "5", "Barney Rubble", "small", "29", "yes" }
};

const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

const char *array2[5][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Mr. George Slate", "grumpy", "53", "yes" },
	{ "2", "Joe Rockhead", "friendly", "33", "yes" },
	{ "3", "Sam Slagheap", "leader", "37", "yes" },
	{ "4", "The Great Gazoo", "hostile alien", "1200", "yes" },
};

int main(void) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_pdf_xml19_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_set_output_format(o, OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
