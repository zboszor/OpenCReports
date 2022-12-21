/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <opencreport.h>

#define LOREMIPSUM "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

const char *array[10][5] = {
	{ "id", "name", "image", "age", "adult" },
	{ "1", "Fred Flintstone", "images/images/matplotlib.svg", "31", "yes" },
	{ "2", "Wilma Flintstone " LOREMIPSUM, "images/images/matplotlib.svg", "28", "yes" },
	{ "3", "Pebbles Flintstone", NULL, "5e-1", "no" },
	{ "4", "Betty Rubble " LOREMIPSUM, "images/images/matplotlib.svg", "27", "yes" },
	{ "5", "Barney Rubble", "images/images/matplotlib.svg", "29", "yes" },
	{ "6", "Mr. George Slate", "images/images/matplotlib.svg", "53", "yes" },
	{ "7", "Joe Rockhead", "images/images/matplotlib.svg", "33", "yes" },
	{ "8", "Sam Slagheap", "images/images/matplotlib.svg", "37", "yes" },
	{ "9", "The Great Gazoo " LOREMIPSUM, "images/images/matplotlib.svg", "1200", "yes" },
};

const int32_t coltypes[6] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	char srcdir0[PATH_MAX];
	char *abs_srcdir = getenv("abs_srcdir");
	if (!abs_srcdir || !*abs_srcdir)
		abs_srcdir = getcwd(srcdir0, sizeof(srcdir0));
	char *csrcdir = ocrpt_canonicalize_path(abs_srcdir);
	opencreport *o = ocrpt_init();

	ocrpt_add_search_path(o, csrcdir);

	if (!ocrpt_parse_xml(o, "layout_pdf_xml41_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_set_output_format(o, OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_mem_free(csrcdir);

	ocrpt_free(o);

	return 0;
}
