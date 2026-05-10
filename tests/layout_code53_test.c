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

	ocrpt_datasource *ds = ocrpt_datasource_add(o, "myarray", "array", NULL);
	ocrpt_query *qa = ocrpt_query_add_data(ds, "a", (const char **)main_array, 1, 1, main_coltypes, 1);
	ocrpt_query *h = ocrpt_query_add_data(ds, "h", (const char **)gen_array, 12, 13, gen_coltypes, 13);

	ocrpt_part *p = ocrpt_part_new(o);
	ocrpt_part_set_paper_type(p, "'A4'");
	ocrpt_part_set_orientation(p, "'landscape'");

	ocrpt_part_row *pr = ocrpt_part_new_row(p);

	ocrpt_part_column *pd = ocrpt_part_row_new_column(pr);

	ocrpt_report *r = ocrpt_part_column_new_report(pd);
	ocrpt_report_set_main_query(r, qa);

	/* Construct field details */

	ocrpt_output *out = ocrpt_layout_report_field_details(r);

	ocrpt_genline *gl = ocrpt_output_add_genline(out);
	ocrpt_genline_set_query(gl, h);
	ocrpt_genline_set_element_type(gl, "h.element_type");
	ocrpt_genline_set_line_font_name(gl, "'Times New Roman'");
	ocrpt_genline_set_line_font_size(gl, "14");
	ocrpt_genline_set_line_bold(gl, "yes");
	ocrpt_genline_set_line_italic(gl, "yes");
	ocrpt_genline_set_value(gl, "h.value");
	ocrpt_genline_set_bgcolor(gl, "h.bgcolor");
	ocrpt_genline_set_color(gl, "h.color");
	ocrpt_genline_set_font_name(gl, "h.font_name");
	ocrpt_genline_set_bold(gl, "h.bold");
	ocrpt_genline_set_italic(gl, "h.italic");
	ocrpt_genline_set_barcode_type(gl, "h.bctype");
	ocrpt_genline_set_image_type(gl, "h.imgtype");
	ocrpt_genline_set_width(gl, "h.width");
	ocrpt_genline_set_height(gl, "h.height");
	ocrpt_genline_set_alignment(gl, "h.align");
	ocrpt_genline_set_suppress(gl, "h.suppress");

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
