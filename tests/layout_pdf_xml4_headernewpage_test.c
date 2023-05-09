/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
//#include "test_common.h"

const char *array[7][6] = {
	{ "id", "name", "property", "age", "male", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "2", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "3", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "4", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "5", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "6", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
};

const int32_t coltypes[6] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_pdf_xml4_headernewpage_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
