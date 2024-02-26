/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>

#define ROWS 20
#define COLS 3

const char *array[ROWS + 1][COLS] = {
	{ "id", "bc", "type" },
	{ "1",  "123456789010", "ean-13" },
	{ "2",  "123456789011", "ean-13" },
	{ "3",  "123456789012", "ean-13" },
	{ "4",  "123456789013", "ean-13" },
	{ "5",  "123456789010A", "code39" },
	{ "6",  "123456789011B", "code39" },
	{ "7",  "123456789012C", "code39" },
	{ "8",  "123456789013D", "code39" },
	{ "9",  "123456789010Aa", "code128b" },
	{ "10", "123456789011Ba", "code128b" },
	{ "11", "123456789012Ca", "code128b" },
	{ "12", "123456789013Da", "code128b" },
	{ "13", "123456789010", "code128c" },
	{ "14", "123456789011", "code128c" },
	{ "15", "123456789012", "code128c" },
	{ "16", "123456789013", "code128c" },
	{ "17", "123456789010Aa", "code128" },
	{ "18", "123456789011Ba", "code128" },
	{ "19", "123456789012Ca", "code128" },
	{ "20", "123456789013Da", "code128" },
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");

	ocrpt_query_add_array(ds, "data", (const char **)array, ROWS, COLS, NULL, 0);

	if (!ocrpt_parse_xml(o, "layout_barcode_test.xml")) {
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
