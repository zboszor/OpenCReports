/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
//#include "test_common.h"

const char *array[9][3] = {
	{ "country", "city", "population" },
	{ "Germany", "Hamburg", "1841000" },
	{ "Germany", "Berlin", "3645000" },
	{ "Germany", "Dresden", "554649" },
	{ "San Marino", NULL, "33745" },
	{ "Monaco", NULL, "36686" },
	{ "Hungary", "Budapest", "1756000" },
	{ "Hungary", "Szolnok", "71285" },
	{ "Vatican", NULL, "825" },
};

const int32_t coltypes[3] = {
	OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "break_suppressblank_test.xml")) {
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
