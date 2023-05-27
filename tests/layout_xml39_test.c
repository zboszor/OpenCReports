/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <opencreport.h>
#include "test_common.h"

const char *array[1][5] = {
	{ "id", "name", "property", "age", "adult" },
};

const int32_t coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_xml39_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	char srcdir0[PATH_MAX];
	char *srcdir = getenv("abs_srcdir");
	if (!srcdir || !*srcdir)
		srcdir = getcwd(srcdir0, sizeof(srcdir0));
	char *csrcdir = ocrpt_canonicalize_path(srcdir);
	ocrpt_string *imgdir = ocrpt_mem_string_new("", true);
	ocrpt_mem_string_append_printf(imgdir, "%s/images/images", csrcdir);
	ocrpt_add_search_path(o, imgdir->str);
	ocrpt_mem_free(csrcdir);
	ocrpt_mem_string_free(imgdir, true);

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
