/*
 * OpenCReports main module
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "exprutil.h"
#include "layout.h"
#include "parts.h"
#include "pdf-output.h"

static cairo_surface_t *ocrpt_pdf_new_page(opencreport *o, const ocrpt_paper *paper, bool landscape) {
	cairo_rectangle_t page = { .x = 0.0, .y = 0.0 };

	if (landscape) {
		page.width = paper->height;
		page.height = paper->width;
	} else {
		page.width = paper->width;
		page.height = paper->height;
	}

	return cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &page);
}

static inline void ocrpt_cairo_create(opencreport *o) {
	pdf_private_data *priv = o->output_private;

	if (priv->current_page) {
		if (priv->cr) {
			if (priv->drawing_page != priv->current_page) {
				cairo_destroy(priv->cr);
				priv->cr = cairo_create((cairo_surface_t *)priv->current_page->data);
				priv->drawing_page = priv->current_page;
			}
		} else
			priv->cr = cairo_create((cairo_surface_t *)priv->current_page->data);
	} else {
		if (!priv->nullpage_cs)
			priv->nullpage_cs = ocrpt_pdf_new_page(o, o->paper, false);
		if (priv->cr) {
			if (priv->drawing_page != priv->current_page) {
				cairo_destroy(priv->cr);
				priv->cr = cairo_create(priv->nullpage_cs);
				priv->drawing_page = priv->current_page;
			}
		} else
			priv->cr = cairo_create(priv->nullpage_cs);
	}
}

static void ocrpt_pdf_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	pdf_private_data *priv = o->output_private;

	if (o->precalculate) {
		if (!priv->current_page) {
			if (!priv->pages) {
				cairo_surface_t *page = ocrpt_pdf_new_page(o, p->paper, p->landscape);
				priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, page);
			}
			priv->current_page = priv->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			cairo_surface_t *page = ocrpt_pdf_new_page(o, p->paper, p->landscape);
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
		}
	}
}

static void *ocrpt_pdf_get_current_page(opencreport *o) {
	pdf_private_data *priv = o->output_private;

	return priv->current_page;
}

static void ocrpt_pdf_set_current_page(opencreport *o, void *page) {
	pdf_private_data *priv = o->output_private;

	priv->current_page = page;
}

static bool ocrpt_pdf_is_current_page_first(opencreport *o) {
	pdf_private_data *priv = o->output_private;

	return priv->current_page == priv->pages;
}

static void ocrpt_pdf_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, double page_indent, double x, double y, double w, double h) {
	pdf_private_data *priv = o->output_private;
	ocrpt_image_file *img_file = img->img_file;

	if (!img_file)
		return;

	ocrpt_cairo_create(o);

	cairo_save(priv->cr);

	if (pd && (page_indent + pd->column_width < x + w)) {
		cairo_rectangle(priv->cr, x, y, x + w - page_indent - pd->column_width, h);
		cairo_clip(priv->cr);
	}

	cairo_save(priv->cr);

	/*
	 * The background filler is 0.1 points wider.
	 * This way, there's no lines between the line elements.
	 */
	cairo_set_source_rgb(priv->cr, img->bg.r, img->bg.g, img->bg.b);
	cairo_set_line_width(priv->cr, 0.0);
	cairo_rectangle(priv->cr, x, y, w + 0.1, h);
	cairo_fill(priv->cr);

	cairo_restore(priv->cr);

	if (!line || !line->current_line) {
		cairo_save(priv->cr);

		if (!line) {
			cairo_translate(priv->cr, x, y);
			cairo_scale(priv->cr, w / img->img_file->width, h / img->img_file->height);
		} else {
			double w1 = h * img->img_file->width / img->img_file->height;

			if (img->align && img->align->result[o->residx] && img->align->result[o->residx]->type == OCRPT_RESULT_STRING && img->align->result[o->residx]->string) {
				const char *alignment = img->align->result[o->residx]->string->str;
				if (strcasecmp(alignment, "right") == 0)
					cairo_translate(priv->cr, x + w - w1, y);
				else if (strcasecmp(alignment, "center") == 0)
					cairo_translate(priv->cr, x + (w - w1) / 2.0, y);
				else
					cairo_translate(priv->cr, x, y);
			} else {
				cairo_translate(priv->cr, x, y);
			}

			cairo_scale(priv->cr, w1 / img->img_file->width, h / img->img_file->height);
		}

		cairo_set_source_surface(priv->cr, img_file->surface, 0.0, 0.0);

		if (img->img_file->rsvg) {
			RsvgRectangle rect = { .x = 0.0, .y = 0.0, .width = img_file->width, .height = img_file->height };
			rsvg_handle_render_document(img_file->rsvg, priv->cr, &rect, NULL);
		}

		cairo_paint(priv->cr);

		cairo_restore(priv->cr);
	}

	cairo_restore(priv->cr);
}

static void ocrpt_pdf_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width) {
	pdf_private_data *priv = o->output_private;

	ocrpt_cairo_create(o);

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

void ocrpt_pdf_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double total_width) {
	pdf_private_data *priv = o->output_private;
	const char *font;
	double size, w;
	bool bold = false, italic = false, newfont = false, justified = false;
	PangoAlignment align;

	ocrpt_cairo_create(o);

	if (l->current_line == 0 || (l->current_line > 0 && le->lines > 0 && l->current_line < le->lines)) {
		if (le->font_name && le->font_name->result[o->residx] && le->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && le->font_name->result[o->residx]->string)
			font = le->font_name->result[o->residx]->string->str;
		else if (l->font_name && l->font_name->result[o->residx] && l->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && l->font_name->result[o->residx]->string)
			font = l->font_name->result[o->residx]->string->str;
		else if (r && r->font_name)
			font = r->font_name;
		else if (p->font_name)
			font = p->font_name;
		else
			font = "Courier";

		if (le && le->font_size && le->font_size->result[o->residx] && le->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->font_size->result[o->residx]->number_initialized)
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
			if (le->font_description)
				pango_font_description_free(le->font_description);
			le->font_description = pango_font_description_new();

			pango_font_description_set_family(le->font_description, font);
			pango_font_description_set_absolute_size(le->font_description, size * PANGO_SCALE);
			pango_font_description_set_weight(le->font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
			pango_font_description_set_style(le->font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);

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

			double render_width = (double)le->p_rect.width / PANGO_SCALE;

			if (pd && (pd->column_width < le->start + le->width_computed)) {
				le->width_computed = pd->column_width - le->start;
				le->use_bb = true;
			}

			le->use_bb = (render_width > le->width_computed);
		}

		if (l->current_line < le->lines) {
			le->pline = pango_layout_get_line(le->layout, l->current_line);
			pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
		} else
			le->pline = NULL;
	}
}

void ocrpt_pdf_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double page_indent, double y) {
	pdf_private_data *priv = o->output_private;

	ocrpt_cairo_create(o);

	cairo_save(priv->cr);

	ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };
	if (le->bgcolor && le->bgcolor->result[o->residx] && le->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && le->bgcolor->result[o->residx]->string)
		ocrpt_get_color(le->bgcolor->result[o->residx]->string->str, &bgcolor, true);
	else if (l->bgcolor && l->bgcolor->result[o->residx] && l->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && l->bgcolor->result[o->residx]->string)
		ocrpt_get_color(l->bgcolor->result[o->residx]->string->str, &bgcolor, true);

	cairo_set_source_rgb(priv->cr, bgcolor.r, bgcolor.g, bgcolor.b);
	cairo_set_line_width(priv->cr, 0.0);
	/*
	 * The background filler is 0.1 points wider.
	 * This way, there's no lines between the line elements.
	 */
	cairo_rectangle(priv->cr, page_indent + le->start, y, le->width_computed + 0.1, l->line_height);
	cairo_fill(priv->cr);

	ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
	if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
		ocrpt_get_color(le->color->result[o->residx]->string->str, &color, true);
	else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
		ocrpt_get_color(l->color->result[o->residx]->string->str, &color, true);

	cairo_set_source_rgb(priv->cr, color.r, color.g, color.b);

	if (le->pline) {
		char *link = NULL;

		if (le->use_bb) {
			/*
			 * Apply the bounding box over the text piece
			 * so it doesn't spill out. While the text
			 * is not shown in print, by double clicking
			 * on such a masked piece of text the whole
			 * of it is shown and can be copy&pasted.
			 */
			cairo_rectangle(priv->cr, page_indent + le->start, y, le->width_computed, l->line_height);
			cairo_clip(priv->cr);
		}

		if (le->link && le->link->result[o->residx] && le->link->result[o->residx]->type == OCRPT_RESULT_STRING && le->link->result[o->residx]->string)
			link = le->link->result[o->residx]->string->str;

		if (link) {
			ocrpt_string *uri = ocrpt_mem_string_new_printf("uri='%s'", link);
			cairo_tag_begin(priv->cr, CAIRO_TAG_LINK, uri->str);
			ocrpt_mem_string_free(uri, true);
		}

		double x1;

		switch (le->p_align) {
		default:
		case PANGO_ALIGN_LEFT:
			x1 = le->start;
			break;
		case PANGO_ALIGN_CENTER:
			x1 = le->start + (le->width_computed - ((double)le->p_rect.width / PANGO_SCALE)) / 2.0;
			break;
		case PANGO_ALIGN_RIGHT:
			x1 = le->start + le->width_computed - ((double)le->p_rect.width / PANGO_SCALE);
			break;
		}

		cairo_move_to(priv->cr, page_indent + x1, y + l->ascent);
		pango_cairo_show_layout_line(priv->cr, le->pline);

		if (link)
			cairo_tag_end(priv->cr, CAIRO_TAG_LINK);
	}

	cairo_restore(priv->cr);

	if (l->current_line + 1 < le->lines) {
		le->pline = pango_layout_get_line(le->layout, l->current_line + 1);
		pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
	} else
		le->pline = NULL;
}

void ocrpt_pdf_draw_hline(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_hline *hline, double page_width, double page_indent, double page_position, double size) {
	pdf_private_data *priv = o->output_private;
	double indent, length;

	if (hline->indent && hline->indent->result[o->residx] && hline->indent->result[o->residx]->type == OCRPT_RESULT_NUMBER && hline->indent->result[o->residx]->number_initialized)
		indent = mpfr_get_d(hline->indent->result[o->residx]->number, o->rndmode);
	else
		indent = 0.0;

	if (hline->length && hline->length->result[o->residx] && hline->length->result[o->residx]->type == OCRPT_RESULT_NUMBER && hline->length->result[o->residx]->number_initialized) {
		double size_multiplier;

		if (o->size_in_points)
			size_multiplier = 1.0;
		else
			size_multiplier = hline->font_width;

		length = mpfr_get_d(hline->length->result[o->residx]->number, o->rndmode) * size_multiplier;
		if (length > page_width - indent)
			length = page_width - indent;
	} else
		length = page_width - indent;

	ocrpt_color color;
	char *color_name = NULL;

	ocrpt_cairo_create(o);

	if (hline->color && hline->color->result[o->residx] && hline->color->result[o->residx]->type == OCRPT_RESULT_STRING && hline->color->result[o->residx]->string)
		color_name = hline->color->result[o->residx]->string->str;

	ocrpt_get_color(color_name, &color, false);

	cairo_set_source_rgb(priv->cr, color.r, color.g, color.b);
	cairo_set_line_width(priv->cr, 0.0);
	cairo_rectangle(priv->cr, page_indent + indent, page_position, length, size);
	cairo_fill(priv->cr);
}

void ocrpt_pdf_draw_rectangle(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_color *color, double line_width, double x, double y, double width, double height) {
	pdf_private_data *priv = o->output_private;

	ocrpt_cairo_create(o);

	cairo_set_source_rgb(priv->cr, color->r, color->g, color->b);
	cairo_set_line_width(priv->cr, line_width);
	cairo_rectangle(priv->cr, x, y, width, height);
	cairo_stroke(priv->cr);
}

static cairo_status_t ocrpt_write_pdf(void *closure, const unsigned char *data, unsigned int length) {
	opencreport *o = closure;

	ocrpt_mem_string_append_len_binary(o->output_buffer, (char *)data, length);

	return CAIRO_STATUS_SUCCESS;
}

void ocrpt_pdf_finalize(opencreport *o) {
	pdf_private_data *priv = o->output_private;
	ocrpt_list *page;
	cairo_surface_t *pdf = cairo_pdf_surface_create_for_stream(ocrpt_write_pdf, o, o->paper->width, o->paper->height);
	char *testrun;

	cairo_pdf_surface_set_metadata(pdf, CAIRO_PDF_METADATA_TITLE, "OpenCReports report");
	cairo_pdf_surface_set_metadata(pdf, CAIRO_PDF_METADATA_AUTHOR, "OpenCReports");
	cairo_pdf_surface_set_metadata(pdf, CAIRO_PDF_METADATA_CREATOR, "OpenCReports");
	testrun = getenv("OCRPT_TEST");
	if (testrun && *testrun && (strcasecmp(testrun, "yes") == 0 || strcasecmp(testrun, "true") == 0 || atoi(testrun) > 0))
		cairo_pdf_surface_set_metadata(pdf, CAIRO_PDF_METADATA_CREATE_DATE, "2022-06-01T12:00:00Z");

	o->output_buffer = ocrpt_mem_string_new_with_len(NULL, 4096);

	for (page = priv->pages; page; page = page->next) {
		cairo_surface_t *surface = (cairo_surface_t *)page->data;
		cairo_rectangle_t rect;

		cairo_recording_surface_get_extents(surface, &rect);
		cairo_pdf_surface_set_size(pdf, rect.width, rect.height);

		cairo_t *cr = cairo_create(pdf);
		cairo_set_source_surface(cr, surface, 0.0, 0.0);
		cairo_paint(cr);
		cairo_show_page(cr);
		cairo_destroy(cr);
	}

	cairo_surface_destroy(pdf);

	ocrpt_list_free_deep(priv->pages, (ocrpt_mem_free_t)cairo_surface_destroy);
	cairo_destroy(priv->cr);
	cairo_surface_destroy(priv->nullpage_cs);

	ocrpt_mem_free(priv);
	o->output_private = NULL;

	ocrpt_string **content_type = ocrpt_mem_malloc(3 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: application/pdf");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_pdf_init(opencreport *o) {
	memset(&o->output_functions, 0, sizeof(ocrpt_output_functions));
	o->output_functions.add_new_page = ocrpt_pdf_add_new_page;
	o->output_functions.get_current_page = ocrpt_pdf_get_current_page;
	o->output_functions.set_current_page = ocrpt_pdf_set_current_page;
	o->output_functions.is_current_page_first = ocrpt_pdf_is_current_page_first;
	o->output_functions.draw_hline = ocrpt_pdf_draw_hline;
	o->output_functions.set_font_sizes = ocrpt_pdf_set_font_sizes;
	o->output_functions.get_text_sizes = ocrpt_pdf_get_text_sizes;
	o->output_functions.draw_text = ocrpt_pdf_draw_text;
	o->output_functions.draw_image = ocrpt_pdf_draw_image;
	o->output_functions.draw_rectangle = ocrpt_pdf_draw_rectangle;
	o->output_functions.finalize = ocrpt_pdf_finalize;

	o->output_private = ocrpt_mem_malloc(sizeof(pdf_private_data));
	memset(o->output_private, 0, sizeof(pdf_private_data));
}