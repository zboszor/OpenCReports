/*
 * OpenCReports output driver common code
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <unistd.h>
#include <utf8proc.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "exprutil.h"
#include "layout.h"
#include "parts.h"
#include "common-output.h"

static void ocrpt_common_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	common_private_data *priv = o->output_private;

	if (o->precalculate) {
		if (!priv->current_page) {
			if (!priv->pages) {
				void *page = (o->output_functions.get_new_page ? o->output_functions.get_new_page(o, p) : NULL);
				priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, page);
			}
			priv->current_page = priv->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			void *page = (o->output_functions.get_new_page ? o->output_functions.get_new_page(o, p) : NULL);
			priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, page);
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
			if (o->output_functions.add_new_page_epilogue)
				o->output_functions.add_new_page_epilogue(o);
		}
	}
}

static void *ocrpt_common_get_current_page(opencreport *o) {
	common_private_data *priv = o->output_private;

	return priv->current_page;
}

static void ocrpt_common_set_current_page(opencreport *o, void *page) {
	common_private_data *priv = o->output_private;

	priv->current_page = page;
}

static bool ocrpt_common_is_current_page_first(opencreport *o) {
	common_private_data *priv = o->output_private;

	return priv->current_page == priv->pages;
}

void ocrpt_common_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width) {
	common_private_data *priv = o->output_private;

	if (o->output_functions.prepare_set_font_sizes)
		o->output_functions.prepare_set_font_sizes(o);

	PangoLayout *layout;
	PangoFontDescription *font_description;

	font_description = pango_font_description_new();

	pango_font_description_set_family(font_description, font);
	pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_font_description_set_style(font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_absolute_size(font_description, wanted_font_size * PANGO_SCALE);

	layout = pango_cairo_create_layout(priv->cr);
	pango_layout_set_font_description(layout, font_description);

	PangoContext *context = pango_layout_get_context(layout);
	PangoLanguage *language = pango_context_get_language(context);
	PangoFontMetrics *metrics = pango_context_get_metrics(context, font_description, language);

	const char *family = pango_font_description_get_family(font_description);
	PangoFontFamily **families;
	int n_families, i;
	bool monospace = true;

	pango_context_list_families(context, &families, &n_families);

	for (i = 0; i < n_families; i++) {
		if (strcmp(family, pango_font_family_get_name(families[i])) == 0) {
			monospace = pango_font_family_is_monospace(families[i]);
			break;
		}
	}

	/* This pointer MUST be freed with g_free() */
	g_free(families);

	int char_width = pango_font_metrics_get_approximate_char_width(metrics);

	if (result_font_size)
		*result_font_size = wanted_font_size;
	if (result_font_width)
		*result_font_width = (monospace ? ((double)char_width / PANGO_SCALE) : wanted_font_size);

	pango_font_metrics_unref(metrics);
	g_object_unref(layout);
	pango_font_description_free(font_description);
}

void ocrpt_common_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double total_width) {
	common_private_data *priv = o->output_private;
	const char *font;
	double size, w;
	bool bold = false, italic = false, newfont = false, justified = false;
	PangoAlignment align;

	if (o->output_functions.prepare_get_text_sizes)
		o->output_functions.prepare_get_text_sizes(o);

	if (l->current_line == 0 || (l->current_line > 0 && le->lines > 0 && l->current_line < le->lines)) {
		if (o->output_functions.line_element_font && le->font_name && le->font_name->result[o->residx] && le->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && le->font_name->result[o->residx]->string)
			font = le->font_name->result[o->residx]->string->str;
		else if (l->font_name && l->font_name->result[o->residx] && l->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && l->font_name->result[o->residx]->string)
			font = l->font_name->result[o->residx]->string->str;
		else if (r && r->font_name)
			font = r->font_name;
		else if (p->font_name)
			font = p->font_name;
		else
			font = "Courier";

		if (o->output_functions.line_element_font && le && le->font_size && le->font_size->result[o->residx] && le->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->font_size->result[o->residx]->number_initialized)
			size = mpfr_get_d(le->font_size->result[o->residx]->number, o->rndmode);
		else if (l && l->font_size && l->font_size->result[o->residx] && l->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->font_size->result[o->residx]->number_initialized)
			size = mpfr_get_d(l->font_size->result[o->residx]->number, o->rndmode);
		else if (r && r->font_size_expr)
			size = r->font_size;
		else if (p && p->font_size_expr)
			size = p->font_size;
		else
			size = OCRPT_DEFAULT_FONT_SIZE;

		if (le->bold && le->bold->result[o->residx] && le->bold->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->bold->result[o->residx]->number_initialized)
			bold = !!mpfr_get_ui(le->bold->result[o->residx]->number, o->rndmode);
		else if (l->bold && l->bold->result[o->residx] && l->bold->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->bold->result[o->residx]->number_initialized)
			bold = !!mpfr_get_ui(l->bold->result[o->residx]->number, o->rndmode);

		if (le->italic && le->italic->result[o->residx] && le->italic->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->italic->result[o->residx]->number_initialized)
			italic = !!mpfr_get_ui(le->italic->result[o->residx]->number, o->rndmode);
		else if (l->italic && l->italic->result[o->residx] && l->italic->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->italic->result[o->residx]->number_initialized)
			italic = !!mpfr_get_ui(l->italic->result[o->residx]->number, o->rndmode);

		if (le->align && le->align->result[o->residx] && le->align->result[o->residx]->type == OCRPT_RESULT_STRING && le->align->result[o->residx]->string) {
			const char *alignment = le->align->result[o->residx]->string->str;
			if (strcasecmp(alignment, "right") == 0)
				align = PANGO_ALIGN_RIGHT;
			else if (strcasecmp(alignment, "center") == 0)
				align = PANGO_ALIGN_CENTER;
			else if (strcasecmp(alignment, "justified") == 0) {
				align = PANGO_ALIGN_LEFT;
				justified = true;
			} else
				align = PANGO_ALIGN_LEFT;
		} else
			align = PANGO_ALIGN_LEFT;

		if (!le->font)
			newfont = true;
		else if (le->font && strcmp(le->font, font))
			newfont = true;
		else if (le->fontsz != size)
			newfont = true;
		else if (le->bold_val != bold)
			newfont = true;
		else if (le->italic_val != italic)
			newfont = true;
		else if (le->p_align != align || le->justified != justified)
			newfont = true;

		if (newfont) {
			if (o->output_functions.support_fontdesc) {
				if (le->font_description)
					pango_font_description_free(le->font_description);
				le->font_description = pango_font_description_new();

				pango_font_description_set_family(le->font_description, font);
				pango_font_description_set_absolute_size(le->font_description, size * PANGO_SCALE);
				pango_font_description_set_weight(le->font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
				pango_font_description_set_style(le->font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
			}

			le->font = font;
			le->fontsz = size;
			le->bold_val = bold;
			le->italic_val = italic;
			le->p_align = align;
			le->justified = justified;
		}

		if (!le->layout) {
			le->layout = pango_cairo_create_layout(priv->cr);
			newfont = true;
		} else if (priv->drawing_page != le->drawing_page) {
			if (le->layout)
				g_object_unref(le->layout);
			le->layout = pango_cairo_create_layout(priv->cr);
			newfont = true;
		}

		if (newfont) {
			pango_layout_set_font_description(le->layout, le->font_description);

			PangoContext *context = pango_layout_get_context(le->layout);
			PangoLanguage *language = pango_context_get_language(context);
			PangoFontMetrics *metrics = pango_context_get_metrics(context, le->font_description, language);

			PangoAttrList *alist = pango_layout_get_attributes(le->layout);
			bool free_alist = false;
			if (!alist) {
				alist = pango_attr_list_new();
				free_alist = true;
			}

			PangoAttribute *nohyph = pango_attr_insert_hyphens_new(TRUE);
			pango_attr_list_insert(alist, nohyph);
			pango_layout_set_attributes(le->layout, alist);

			if (free_alist)
				pango_attr_list_unref(alist);

			pango_layout_set_alignment(le->layout, le->p_align);
			if (le->justified) {
				pango_layout_set_justify(le->layout, true);
				pango_layout_set_justify_last_line(le->layout, false);
			}

			int char_width = pango_font_metrics_get_approximate_char_width(metrics);
			le->font_width = (double)char_width / PANGO_SCALE;

			le->ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
			le->descent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;

			pango_font_metrics_unref(metrics);
		}

		pango_layout_set_text(le->layout, le->result_str->str, le->result_str->len);

		if (le->width && le->width->result[o->residx] && le->width->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->width->result[o->residx]->number_initialized) {
			w = mpfr_get_d(le->width->result[o->residx]->number, o->rndmode);
			if (!o->size_in_points)
				w *= l->font_width;
		} else if (ocrpt_list_length(l->elements) == 1) {
			w = total_width;
		} else {
			PangoRectangle logical_rect;

			/* This assumes pango_layout_set_text() was called previously */
			pango_layout_get_extents(le->layout, NULL, &logical_rect);
			w = (double)logical_rect.width / PANGO_SCALE;
		}

		le->width_computed = w;

		if (le->memo) {
			pango_layout_set_width(le->layout, w * PANGO_SCALE);
			pango_layout_set_wrap(le->layout, le->memo_wrap_chars ? PANGO_WRAP_CHAR : PANGO_WRAP_WORD);
		} else {
			pango_layout_set_wrap(le->layout, PANGO_WRAP_CHAR);
		}

		le->use_bb = false;
		if (le->memo) {
			le->lines = pango_layout_get_line_count(le->layout);
			if (le->memo_max_lines && le->memo_max_lines < le->lines)
				le->lines = le->memo_max_lines;
		} else {
			le->lines = 1;

			if (o->output_functions.support_bbox) {
				double render_width = (double)le->p_rect.width / PANGO_SCALE;

				if (pd && (pd->column_width < le->start + le->width_computed)) {
					le->width_computed = pd->column_width - le->start;
					le->use_bb = true;
				} else
					le->use_bb = (render_width > le->width_computed);
			}
		}

		if (l->current_line < le->lines) {
			le->pline = pango_layout_get_line(le->layout, l->current_line);
			pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
		} else
			le->pline = NULL;
	}
}

void ocrpt_common_init(opencreport *o, size_t privsz, size_t datasz, size_t outbufsz) {
	memset(&o->output_functions, 0, sizeof(ocrpt_output_functions));
	o->output_functions.add_new_page = ocrpt_common_add_new_page;
	o->output_functions.get_current_page = ocrpt_common_get_current_page;
	o->output_functions.set_current_page = ocrpt_common_set_current_page;
	o->output_functions.is_current_page_first = ocrpt_common_is_current_page_first;

	o->output_private = ocrpt_mem_malloc(privsz);
	memset(o->output_private, 0, privsz);

	common_private_data *priv = o->output_private;
	priv->data = ocrpt_mem_string_new_with_len(NULL, datasz);

	o->output_buffer = ocrpt_mem_string_new_with_len(NULL, outbufsz);

	cairo_rectangle_t page = { .x = o->paper->width, .y = o->paper->height };
	priv->nullpage_cs = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &page);
	priv->cr = cairo_create(priv->nullpage_cs);
}

void ocrpt_common_finalize(opencreport *o) {
	common_private_data *priv = o->output_private;

	ocrpt_list_free(priv->pages);
	ocrpt_mem_string_free(priv->data, true);

	if (priv->cr)
		cairo_destroy(priv->cr);
	if (priv->nullpage_cs)
		cairo_surface_destroy(priv->nullpage_cs);

	ocrpt_mem_free(priv);

	o->output_private = NULL;
}
