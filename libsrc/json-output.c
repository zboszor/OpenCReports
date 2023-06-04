/*
 * OpenCReports JSON output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>

#include "ocrpt-private.h"
#include "parts.h"
#include "json-output.h"

static void ocrpt_json_start_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, double page_indent, double y) {
}

static void ocrpt_json_end_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l) {
}

static void ocrpt_json_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double page_width, double page_indent, double y) {
	if (le->value || le->result_str->len)
		fprintf(stderr, "ocrpt_json_draw_text: %s\n", le->result_str->str);
}

static void ocrpt_json_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, double page_width, double page_indent, double x, double y, double w, double h) {
	fprintf(stderr, "ocrpt_json_draw_image: -\n");
}

static void ocrpt_json_finalize(opencreport *o) {
	ocrpt_common_finalize(o);

	ocrpt_string **content_type = ocrpt_mem_malloc(4 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: text/json; charset=utf-8\n");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = ocrpt_mem_string_new_printf("Content-Disposition: attachment; filename=%s", o->csv_filename ? o->csv_filename : "report.json");
	content_type[3] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_json_init(opencreport *o) {
	ocrpt_common_init(o, sizeof(json_private_data), 4096, 65536);
	o->output_functions.start_data_row = ocrpt_json_start_data_row;
	o->output_functions.end_data_row = ocrpt_json_end_data_row;
	o->output_functions.draw_image = ocrpt_json_draw_image;
	o->output_functions.draw_text = ocrpt_json_draw_text;
	o->output_functions.finalize = ocrpt_json_finalize;
}
