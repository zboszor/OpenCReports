/*
 * OpenCReports CSV output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <csv.h>

#include "ocrpt-private.h"
#include "exprutil.h"
#include "parts.h"
#include "csv-output.h"

static void ocrpt_csv_start_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, double page_indent, double y) {
	csv_private_data *priv = o->output_private;

	priv->column_index = 0;
}

static void ocrpt_csv_end_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l) {
	csv_private_data *priv = o->output_private;

	if (priv->column_index)
		ocrpt_mem_string_append(o->output_buffer, "\n");
}

static void ocrpt_csv_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double page_width, double page_indent, double y) {
	csv_private_data *priv = o->output_private;

	if (le->value || le->result_str->len) {
		if (priv->column_index)
			ocrpt_mem_string_append(o->output_buffer, o->csv_delimiter ? o->csv_delimiter : ",");
		if (priv->base.data->allocated_len < le->result_str->len * 3)
			ocrpt_mem_string_resize(priv->base.data, le->result_str->len * 3 + 16);

		if (o->no_quotes)
			ocrpt_mem_string_append_len(o->output_buffer, le->result_str->str, le->result_str->len);
		else if (o->only_quote_strings) {
			if (le->value) {
				/*
				 * The field has a value set.
				 * Quote it if it's a string or string-like value.
				 */
				if (le->value->result[o->residx] && (le->value->result[o->residx]->type == OCRPT_RESULT_STRING || le->value->result[o->residx]->type == OCRPT_RESULT_ERROR)) {
					size_t len = csv_write(priv->base.data->str, priv->base.data->allocated_len, le->result_str->str, le->result_str->len);
					ocrpt_mem_string_append_len(o->output_buffer, priv->base.data->str, len);
				} else
					ocrpt_mem_string_append_len(o->output_buffer, le->result_str->str, le->result_str->len);
			} else {
				/*
				 * The field has no value set, but there's
				 * a non-empty result string(!) - quote it.
				 */
				size_t len = csv_write(priv->base.data->str, priv->base.data->allocated_len, le->result_str->str, le->result_str->len);
				ocrpt_mem_string_append_len(o->output_buffer, priv->base.data->str, len);
			}
		} else {
			size_t len = csv_write(priv->base.data->str, priv->base.data->allocated_len, le->result_str->str, le->result_str->len);
			ocrpt_mem_string_append_len(o->output_buffer, priv->base.data->str, len);
		}

		priv->column_index++;
	}
}

static void ocrpt_csv_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, double page_width, double page_indent, double x, double y, double w, double h) {
	csv_private_data *priv = o->output_private;

	if (!line)
		return;

	if (priv->column_index)
		ocrpt_mem_string_append(o->output_buffer, o->csv_delimiter ? o->csv_delimiter : ",");

	/* No data here */
	priv->column_index++;
}

static void ocrpt_csv_finalize(opencreport *o) {
	ocrpt_common_finalize(o);

	ocrpt_string **content_type = ocrpt_mem_malloc(4 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: %s; charset=utf-8\n", o->csv_as_text ? "text/plain" : "text/csv");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = ocrpt_mem_string_new_printf("Content-Disposition: attachment; filename=%s", o->csv_filename ? o->csv_filename : "report.csv");
	content_type[3] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_csv_init(opencreport *o) {
	ocrpt_common_init(o, sizeof(csv_private_data), 4096, 65536);
	o->output_functions.start_data_row = ocrpt_csv_start_data_row;
	o->output_functions.end_data_row = ocrpt_csv_end_data_row;
	o->output_functions.draw_image = ocrpt_csv_draw_image;
	o->output_functions.draw_text = ocrpt_csv_draw_text;
	o->output_functions.finalize = ocrpt_csv_finalize;
}
