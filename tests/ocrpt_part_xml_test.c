/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_list *pl;
	int32_t i;

	if (!ocrpt_parse_xml(o, "ocrpt_part_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	for (pl = o->parts, i = 0; pl; pl = pl->next, i++) {
		char partname[16];
		ocrpt_part *p = (ocrpt_part *)pl->data;

		sprintf(partname, "part %d", i);
		print_part_reports(partname, p);
	}

	ocrpt_free(o);

	return 0;
}
