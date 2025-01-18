/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

const int32_t coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_list *pl = NULL;
	ocrpt_part *p;
	int32_t i;

	if (!ocrpt_parse_xml(o, "part_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	for (p = ocrpt_part_get_next(o, &pl), i = 0; p; p = ocrpt_part_get_next(o, &pl), i++) {
		char partname[16];

		sprintf(partname, "part %d", i);
		print_part_reports(partname, p);
	}

	ocrpt_free(o);

	return 0;
}
