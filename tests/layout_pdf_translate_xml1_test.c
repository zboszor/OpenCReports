/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <opencreport.h>

const char *array[10][4] = {
	{ "id", "name", "age", "adult" },
	{ "1", "Fred Flintstone", "31", "yes" },
	{ "2", "Wilma Flintstone ", "28", "yes" },
	{ "3", "Pebbles Flintstone", "5e-1", "no" },
	{ "4", "Betty Rubble ", "27", "yes" },
	{ "5", "Barney Rubble", "29", "yes" },
	{ "6", "Mr. George Slate", "53", "yes" },
	{ "7", "Joe Rockhead", "33", "yes" },
	{ "8", "Sam Slagheap", "37", "yes" },
	{ "9", "The Great Gazoo ", "1200", "yes" },
};

const int32_t coltypes[4] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_pdf_translate_xml1_test.xml")) {
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
