/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
#include "test_common.h"

/* NOT static */ const char *array[8][6] = {
	{ "id", "firstname", "surname", "age", "male", "indent" },
	{ "1", "Fred", "Flintstone", "31", "yes", "16" },
	{ "2", "Barney", "Rubble", "29", "yes", "16" },
	{ "3", "Bamm-Bamm", "Rubble", "2", "yes", "38" },
	{ "4", "The Great", "Gazoo", "1200", "yes", "16" },
	{ "5", "Wilma", "Flintstone", "28", "no", "27" },
	{ "6", "Betty", "Rubble", "27", "no", "27" },
	{ "7", "Pebbles", "Flintstone", "5e-1", "no", "38" },
};

/* NOT static */ const int32_t coltypes[6] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_matched_overlay_text_indent_test.xml")) {
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
