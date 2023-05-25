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

static void ocrpt_json_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	json_private_data *priv = o->output_private;

	if (o->precalculate) {
		if (!priv->current_page) {
			if (!priv->pages)
				priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, NULL);
			priv->current_page = priv->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, NULL);
			priv->current_page = priv->last_page;
		}

		if (mpfr_cmp(o->totpages->number, o->pageno->number) < 0)
			mpfr_set(o->totpages->number, o->pageno->number, o->rndmode);
	} else {
		if (!priv->current_page) {
			priv->current_page = priv->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			priv->current_page = priv->current_page->next;
		}
	}
}

static void *ocrpt_json_get_current_page(opencreport *o) {
	json_private_data *priv = o->output_private;

	return priv->current_page;
}

static void ocrpt_json_set_current_page(opencreport *o, void *page) {
	json_private_data *priv = o->output_private;

	priv->current_page = page;
}

static bool ocrpt_json_is_current_page_first(opencreport *o) {
	json_private_data *priv = o->output_private;

	return priv->current_page == priv->pages;
}

static void ocrpt_json_start_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, double page_indent, double y) {
}

static void ocrpt_json_end_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l) {
}

static void ocrpt_json_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double page_width, double page_indent, double y) {
	if (le->value || le->result_str->len)
		fprintf(stderr, "ocrpt_json_draw_text: %s\n", le->result_str->str);
}

static void ocrpt_json_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, double page_width, double page_indent, double x, double y, double w, double h) {
	fprintf(stderr, "ocrpt_json_draw_image: -\n");
}

static void ocrpt_json_finalize(opencreport *o) {
	json_private_data *priv = o->output_private;

	ocrpt_list_free(priv->pages);
	ocrpt_mem_string_free(priv->data, true);
	ocrpt_mem_free(priv);
	o->output_private = NULL;

	ocrpt_string **content_type = ocrpt_mem_malloc(4 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: text/json; charset=utf-8\n");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = ocrpt_mem_string_new_printf("Content-Disposition: attachment; filename=%s", o->csv_filename ? o->csv_filename : "report.json");
	content_type[3] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_json_init(opencreport *o) {
	memset(&o->output_functions, 0, sizeof(ocrpt_output_functions));
	o->output_functions.start_data_row = ocrpt_json_start_data_row;
	o->output_functions.end_data_row = ocrpt_json_end_data_row;
	o->output_functions.draw_image = ocrpt_json_draw_image;
	o->output_functions.draw_text = ocrpt_json_draw_text;
	o->output_functions.add_new_page = ocrpt_json_add_new_page;
	o->output_functions.get_current_page = ocrpt_json_get_current_page;
	o->output_functions.set_current_page = ocrpt_json_set_current_page;
	o->output_functions.is_current_page_first = ocrpt_json_is_current_page_first;
	o->output_functions.finalize = ocrpt_json_finalize;

	o->output_private = ocrpt_mem_malloc(sizeof(json_private_data));
	memset(o->output_private, 0, sizeof(json_private_data));

	json_private_data *priv = o->output_private;
	priv->data = ocrpt_mem_string_new_with_len(NULL, 4096);

	o->output_buffer = ocrpt_mem_string_new_with_len(NULL, 65536);
}
