/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>

const char *array[6][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" },
	{ "4", "Betty Rubble", "beautiful", "27", "yes" },
	{ "5", "Barney Rubble", "small", "29", "yes" }
};

const int32_t coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

const char *array2[5][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Mr. George Slate", "grumpy", "53", "yes" },
	{ "2", "Joe Rockhead", "friendly", "33", "yes" },
	{ "3", "Sam Slagheap", "leader", "37", "yes" },
	{ "4", "The Great Gazoo", "hostile alien", "1200", "yes" },
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_xml17_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);
	ocrpt_set_output_parameter(o, "only_quote_strings", "yes");

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
