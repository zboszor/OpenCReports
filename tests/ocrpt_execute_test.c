/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

int main(void) {
	opencreport *o = ocrpt_init();

	o->debug_report_ptr = true;

	if (!ocrpt_parse_xml(o, "ocrpt_part_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_execute(o);

	ocrpt_free(o);

	return 0;
}
