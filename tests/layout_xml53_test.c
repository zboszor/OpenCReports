/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <opencreport.h>

/* Minimal single-row main query to drive one FieldDetails iteration */
const char *main_array[2][1] = {
	{ "dummy" },
	{ "1" },
};

const int32_t main_coltypes[1] = {
	OCRPT_RESULT_NUMBER
};

/*
 * GeneratedLine query with 12 elements per output row.
 * Elements 3 and 9 are barcodes (EAN-13, "123456789012").
 *   - Element 3: red background, black foreground.
 *   - Element 9: unset background, black foreground.
 * Elements 6 and 12 are SVG images (matplotlib.svg).
 *   - Element 6: green background.
 *   - Element 12: unset background.
 * The remaining 8 are text elements whose displayed value equals
 * their 1-based index in the list.
 *
 * Line-level (genline_*) attributes set in the XML:
 *   font_name = "Times New Roman", font_size = 14, bold = yes, italic = yes
 *
 * Per-element attributes for text elements (ignoring barcodes/images):
 *   font_name  alternates: "Courier", NULL, "Courier", NULL, ...
 *   bold       alternates: NULL, 1, 0, NULL, 1, 0, NULL, 1
 *   italic     alternates: 0, 1, NULL, 0, 1, NULL, 0, 1
 *
 * Alignment alternates left/right/center across all 12 elements.
 * Every fourth element (4, 8, 12) is suppressed.
 *
 * Columns: element_type, value, bgcolor, color,
 *          font_name, bold, italic, bctype, imgtype, width, height,
 *          align, suppress
 */
const char *gen_array[13][13] = {
	{ "element_type", "value",          "bgcolor", "color", "font_name", "bold", "italic", "bctype",  "imgtype", "width", "height", "align",    "suppress" },
	{ "text",         "1",              NULL,      NULL,    "Courier",   NULL,   "0",      NULL,      NULL,      "6",     "2",      "left",     NULL       },
	{ "text",         "2",              NULL,      NULL,    NULL,        "1",    "1",      NULL,      NULL,      "6",     "2",      "right",    NULL       },
	{ "barcode",      "'123456789012'", "red",     "black", NULL,        NULL,   NULL,     "ean-13",  NULL,      NULL,    "50",     "center",   NULL       },
	{ "text",         "4",              NULL,      NULL,    "Courier",   "0",    NULL,     NULL,      NULL,      "6",     "2",      "left",     "1"        },
	{ "text",         "5",              NULL,      NULL,    NULL,        NULL,   "0",      NULL,      NULL,      "6",     "2",      "right",    NULL       },
	{ "image",        "matplotlib.svg", "green",   NULL,    NULL,        NULL,   NULL,     NULL,      "svg",     "20",    "50",     "center",   NULL       },
	{ "text",         "7",              NULL,      NULL,    "Courier",   "1",    "1",      NULL,      NULL,      "6",     "2",      "left",     NULL       },
	{ "text",         "8",              NULL,      NULL,    NULL,        "0",    NULL,     NULL,      NULL,      "6",     "2",      "right",    "1"        },
	{ "barcode",      "'123456789012'", NULL,      "black", NULL,        NULL,   NULL,     "ean-13",  NULL,      NULL,    "50",     "center",   NULL       },
	{ "text",         "10",             NULL,      NULL,    "Courier",   NULL,   "0",      NULL,      NULL,      "6",     "2",      "left",     NULL       },
	{ "text",         "11",             NULL,      NULL,    NULL,        "1",    "1",      NULL,      NULL,      "6",     "2",      "right",    NULL       },
	{ "image",        "matplotlib.svg", NULL,      NULL,    NULL,        NULL,   NULL,     NULL,      "svg",     "20",    "50",     "center",   "1"        },
};

const int32_t gen_coltypes[13] = {
	OCRPT_RESULT_STRING, /* element_type */
	OCRPT_RESULT_STRING, /* value        */
	OCRPT_RESULT_STRING, /* bgcolor      */
	OCRPT_RESULT_STRING, /* color        */
	OCRPT_RESULT_STRING, /* font_name    */
	OCRPT_RESULT_NUMBER, /* bold         */
	OCRPT_RESULT_NUMBER, /* italic       */
	OCRPT_RESULT_STRING, /* bctype       */
	OCRPT_RESULT_STRING, /* imgtype      */
	OCRPT_RESULT_NUMBER, /* width        */
	OCRPT_RESULT_NUMBER, /* height       */
	OCRPT_RESULT_STRING, /* align        */
	OCRPT_RESULT_NUMBER, /* suppress     */
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_xml53_test.xml")) {
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
