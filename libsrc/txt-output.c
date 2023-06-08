/*
 * OpenCReports TXT output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <math.h>

#include "ocrpt-private.h"
#include "exprutil.h"
#include "formatting.h"
#include "parts.h"
#include "txt-output.h"

static void ocrpt_txt_end_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l) {
	ocrpt_mem_string_append(o->output_buffer, "\n");
}

static void ocrpt_txt_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double page_width, double page_indent, double y) {
	txt_private_data *priv = o->output_private;
	int startpos = le->pline ? pango_layout_line_get_start_index(le->pline) : 0;
	int length = le->pline ? pango_layout_line_get_length(le->pline) : 0;
	int32_t l2, bl2;
	int32_t field_width;

	if (le->width) {
		field_width = (int32_t)ceil(le->width_computed / l->font_width);
		if (field_width <= 0)
			field_width++;

		ocrpt_utf8forward(le->result_str->str + startpos, le->memo ? field_width : length, &l2, length, &bl2);
	} else {
		ocrpt_utf8forward(le->result_str->str + startpos, length, &l2, length, &bl2);
		field_width = ((le->p_align == PANGO_ALIGN_LEFT || ocrpt_list_length(l->elements) > 1) ? l2 : (page_width / l->font_width));
	}

	//fprintf(stderr, "ocrpt_txt_draw_text: wc %.2lf fw %.2lf field  w %d text len %d (%d): '%*.*s'\n", le->width_computed, l->font_width, field_width, l2, bl2, bl2, bl2, le->result_str->str + startpos);

	if (field_width >= l2) {
		/* Designated field width is wider than the text, padding is needed */
		int32_t nspc = field_width - l2, i;
		int32_t nspc1 = nspc >> 1, nspc2 = nspc - nspc1;

		if ((nspc + 1) > priv->spc_padding->allocated_len)
			ocrpt_mem_string_resize(priv->spc_padding, nspc + 1);

		for (i = 0; i < nspc; i++)
			priv->spc_padding->str[i] = ' ';
		priv->spc_padding->str[nspc] = 0;
		priv->spc_padding->len = nspc;

		switch (le->p_align) {
		case PANGO_ALIGN_LEFT:
			ocrpt_mem_string_append_len(o->output_buffer, le->result_str->str + startpos, bl2);
			ocrpt_mem_string_append_len(o->output_buffer, priv->spc_padding->str, priv->spc_padding->len);
			break;
		case PANGO_ALIGN_RIGHT:
			ocrpt_mem_string_append_len(o->output_buffer, priv->spc_padding->str, priv->spc_padding->len);
			ocrpt_mem_string_append_len(o->output_buffer, le->result_str->str + startpos, bl2);
			break;
		case PANGO_ALIGN_CENTER:
			ocrpt_mem_string_append_len(o->output_buffer, priv->spc_padding->str, nspc1);
			ocrpt_mem_string_append_len(o->output_buffer, le->result_str->str + startpos, bl2);
			ocrpt_mem_string_append_len(o->output_buffer, priv->spc_padding->str, nspc2);
			break;
		}
	} else {
		/* Designated field width is narrower than the text, truncation is needed */
		int32_t trnc = l2 - field_width;
		int32_t trnc1 = trnc >> 1;
		int bl3;

		switch (le->p_align) {
		case PANGO_ALIGN_LEFT:
			ocrpt_utf8forward(le->result_str->str + startpos, field_width, &l2, length, &bl2);
			ocrpt_mem_string_append_len(o->output_buffer, le->result_str->str, bl2);
			break;
		case PANGO_ALIGN_RIGHT:
			ocrpt_utf8backward(le->result_str->str + startpos, field_width, &l2, length, &bl2);
			ocrpt_mem_string_append_len(o->output_buffer, le->result_str->str + startpos + bl2, le->result_str->len - startpos - bl2);
			break;
		case PANGO_ALIGN_CENTER:
			ocrpt_utf8forward(le->result_str->str + startpos, trnc1, &l2, length, &bl2);
			ocrpt_utf8forward(le->result_str->str + startpos + bl2, field_width, NULL, length - bl2, &bl3);
			ocrpt_mem_string_append_len(o->output_buffer, le->result_str->str + startpos + bl2, bl3);
			break;
		}
	}

	if (l->current_line + 1 < le->lines) {
		le->pline = pango_layout_get_line(le->layout, l->current_line + 1);
		pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
	} else
		le->pline = NULL;
}

static void ocrpt_txt_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, double page_width, double page_indent, double x, double y, double w, double h) {
	txt_private_data *priv = o->output_private;
	double size;
	uint32_t nspc, i;

	if (line) {
		if (line && line->font_size && line->font_size->result[o->residx] && line->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && line->font_size->result[o->residx]->number_initialized)
			size = mpfr_get_d(line->font_size->result[o->residx]->number, o->rndmode);
		else if (r && r->font_size_expr)
			size = r->font_size;
		else if (p && p->font_size_expr)
			size = p->font_size;
		else
			size = OCRPT_DEFAULT_FONT_SIZE;

		nspc = round(w / size);

		if ((nspc + 1) > priv->base.data->allocated_len)
			ocrpt_mem_string_resize(priv->base.data, nspc + 1);

		for (i = 0; i < nspc; i++)
			priv->base.data->str[i] = ' ';
		priv->base.data->str[nspc] = 0;
		priv->base.data->len = nspc;

		ocrpt_mem_string_append_len(o->output_buffer, priv->base.data->str, priv->base.data->len);
	} else {
		if (r && r->font_size_expr)
			size = r->font_size;
		else if (p && p->font_size_expr)
			size = p->font_size;
		else
			size = OCRPT_DEFAULT_FONT_SIZE;

		nspc = round(w / size);

		if ((nspc + 1) > priv->bgimagepfx->allocated_len)
			ocrpt_mem_string_resize(priv->bgimagepfx, nspc + 1);

		for (i = 0; i < nspc; i++)
			priv->bgimagepfx->str[i] = ' ';
		priv->bgimagepfx->str[nspc] = 0;
		priv->bgimagepfx->len = nspc;
	}
}

static void ocrpt_txt_draw_imageend(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output) {
	txt_private_data *priv = o->output_private;

	priv->bgimagepfx->len = 0;
}

static void ocrpt_txt_end_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output) {
	txt_private_data *priv = o->output_private;

	priv->bgimagepfx->len = 0;
}

static void ocrpt_txt_finalize(opencreport *o) {
	txt_private_data *priv = o->output_private;

	ocrpt_mem_string_free(priv->bgimagepfx, true);
	ocrpt_mem_string_free(priv->spc_padding, true);

	ocrpt_common_finalize(o);

	ocrpt_string **content_type = ocrpt_mem_malloc(4 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: text/plain; charset=utf-8\n");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = ocrpt_mem_string_new_printf("Content-Disposition: attachment; filename=%s", o->csv_filename ? o->csv_filename : "report.txt");
	content_type[3] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_txt_init(opencreport *o) {
	ocrpt_common_init(o, sizeof(txt_private_data), 4096, 65536);
	o->output_functions.set_font_sizes = ocrpt_common_set_font_sizes;
	o->output_functions.get_text_sizes = ocrpt_common_get_text_sizes;
	o->output_functions.end_output = ocrpt_txt_end_output;
	o->output_functions.end_data_row = ocrpt_txt_end_data_row;
	o->output_functions.draw_image = ocrpt_txt_draw_image;
	o->output_functions.draw_imageend = ocrpt_txt_draw_imageend;
	o->output_functions.draw_text = ocrpt_txt_draw_text;
	o->output_functions.finalize = ocrpt_txt_finalize;
	o->output_functions.support_fontdesc = true;

	txt_private_data *priv = o->output_private;
	priv->bgimagepfx = ocrpt_mem_string_new_with_len(NULL, 64);
	priv->spc_padding = ocrpt_mem_string_new_with_len(NULL, 64);
}
