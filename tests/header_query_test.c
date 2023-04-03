/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

/* NOT static */ const char *array[7][4] = {
	{ "id", "name", "age", "male" },
	{ "1", "Fred Flintstone", "31", "yes" },
	{ "2", "Barney Rubble", "29", "yes" },
	{ "3", "Bamm-Bamm Rubble", "2", "yes" },
	{ "4", "Wilma Flintstone", "28", "no" },
	{ "5", "Betty Rubble", "27", "no" },
	{ "6", "Pebbles Flintstone", "5e-1", "no" },
};

/* NOT static */ const int32_t coltypes[4] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

/* NOT static */ const char *header[2][8] = {
	{ "id", "paper_size", "font_name", "font_size", "top_margin", "bottom_margin", "left_margin", "right_margin" },
	{ "1", "B5", "Times New Roman", "9", "1", "2", "1", "2" },
};

int main(void) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "header_query_test.xml")) {
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
