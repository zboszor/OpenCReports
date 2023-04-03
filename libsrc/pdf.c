/*
 * OpenCReports main module
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <langinfo.h>
#include <libintl.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "exprutil.h"
#include "formatting.h"
#include "layout.h"
#include "parts.h"
#include "pdf.h"

static cairo_status_t ocrpt_write_pdf(void *closure, const unsigned char *data, unsigned int length) {
	opencreport *o = closure;

	ocrpt_mem_string_append_len_binary(o->output_buffer, (char *)data, length);

	return CAIRO_STATUS_SUCCESS;
}

static void ocrpt_pdf_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, double page_indent, double x, double y, double w, double h) {
	ocrpt_image_file *img_file = img->img_file;

	if (!img_file)
		return;

	ocrpt_cairo_create(o);

	cairo_save(o->cr);

	if (pd && (page_indent + pd->column_width < x + w)) {
		cairo_rectangle(o->cr, x, y, x + w - page_indent - pd->column_width, h);
		cairo_clip(o->cr);
	}

	cairo_save(o->cr);

	/*
	 * The background filler is 0.1 points wider.
	 * This way, there's no lines between the line elements.
	 */
	cairo_set_source_rgb(o->cr, img->bg.r, img->bg.g, img->bg.b);
	cairo_set_line_width(o->cr, 0.0);
	cairo_rectangle(o->cr, x, y, w + 0.1, h);
	cairo_fill(o->cr);

	cairo_restore(o->cr);

	if (!line || !line->current_line) {
		cairo_save(o->cr);

		if (!line) {
			cairo_translate(o->cr, x, y);
			cairo_scale(o->cr, w / img->img_file->width, h / img->img_file->height);
		} else {
			double w1 = h * img->img_file->width / img->img_file->height;

			if (img->align && img->align->result[o->residx] && img->align->result[o->residx]->type == OCRPT_RESULT_STRING && img->align->result[o->residx]->string) {
				const char *alignment = img->align->result[o->residx]->string->str;
				if (strcasecmp(alignment, "right") == 0)
					cairo_translate(o->cr, x + w - w1, y);
				else if (strcasecmp(alignment, "center") == 0)
					cairo_translate(o->cr, x + (w - w1) / 2.0, y);
				else
					cairo_translate(o->cr, x, y);
			} else {
				cairo_translate(o->cr, x, y);
			}

			cairo_scale(o->cr, w1 / img->img_file->width, h / img->img_file->height);
		}

		cairo_set_source_surface(o->cr, img_file->surface, 0.0, 0.0);

		if (img->img_file->rsvg) {
			RsvgRectangle rect = { .x = 0.0, .y = 0.0, .width = img_file->width, .height = img_file->height };
			rsvg_handle_render_document(img_file->rsvg, o->cr, &rect, NULL);
		}

		cairo_paint(o->cr);

		cairo_restore(o->cr);
	}

	cairo_restore(o->cr);
}

void ocrpt_pdf_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double total_width) {
	char *font;
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
		else if (r && r->font_size_set)
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
			le->layout = pango_cairo_create_layout(o->cr);
			newfont = true;
		} else if (o->drawing_page != le->drawing_page) {
			if (le->layout)
				g_object_unref(le->layout);
			le->layout = pango_cairo_create_layout(o->cr);
			newfont = true;
		}

		if (newfont) {
			pango_layout_set_font_description(le->layout, le->font_description);

			PangoContext *context = pango_layout_get_context(le->layout);
			PangoLanguage *language = pango_context_get_language(context);
			PangoFontMetrics *metrics = pango_context_get_metrics(context, le->font_description, language);

			PangoAttrList *alist = pango_layout_get_attributes(le->layout);
			if (!alist)
				alist = pango_attr_list_new();

			PangoAttribute *nohyph = pango_attr_insert_hyphens_new(TRUE);
			pango_attr_list_insert(alist, nohyph);
			pango_layout_set_attributes(le->layout, alist);

			pango_layout_set_alignment(le->layout, le->p_align);
			if (le->justified) {
				pango_layout_set_justify(le->layout, true);
				pango_layout_set_justify_last_line(le->layout, false);
			}

			int char_width = pango_font_metrics_get_approximate_char_width(metrics);
			le->font_width = (double)char_width / PANGO_SCALE;

			le->ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
			le->descent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
		}

		bool has_translate = false;
		bool has_format = false;
		bool has_value = false;
		bool string_value = false;

		if (o->textdomain && le->translate && le->translate->result[o->residx] && le->translate->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->translate->result[o->residx]->number_initialized) {
			has_translate = !!mpfr_get_si(le->translate->result[o->residx]->number, o->rndmode);
		}

		if (le->format && le->format->result[o->residx] && le->format->result[o->residx]->type == OCRPT_RESULT_STRING && le->format->result[o->residx]->string)
			has_format = true;

		if (le->value && le->value->result[o->residx] &&
			(
				(le->value->result[o->residx]->type == OCRPT_RESULT_STRING && le->value->result[o->residx]->string) ||
				(le->value->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->value->result[o->residx]->number_initialized) ||
				(le->value->result[o->residx]->type == OCRPT_RESULT_DATETIME && (le->value->result[o->residx]->date_valid || le->value->result[o->residx]->time_valid))
			))
			has_value = true;

		ocrpt_string *fstring = ocrpt_mem_string_resize(le->format_str, 16);
		if (fstring) {
			le->format_str = fstring;
			fstring->len = 0;
		}

		ocrpt_string *string = ocrpt_mem_string_resize(le->value_str, 16);
		if (string) {
			le->value_str = string;
			string->len = 0;
		}

		ocrpt_string *rstring = ocrpt_mem_string_resize(le->result_str, 16);
		if (rstring) {
			le->result_str = rstring;
			rstring->len = 0;
		}

		if (has_value) {
			if (has_format) {
				if (has_translate) {
					locale_t locale = uselocale(o->locale);
					ocrpt_mem_string_append_printf(fstring, "%s", dgettext(o->textdomain, le->format->result[o->residx]->string->str));
					uselocale(locale);
				} else
					ocrpt_mem_string_append_printf(fstring, "%s", le->format->result[o->residx]->string->str);
			} else {
				switch (le->value->result[o->residx]->type) {
				case OCRPT_RESULT_STRING:
				case OCRPT_RESULT_ERROR:
					ocrpt_mem_string_append_printf(fstring, "%s", "%s");
					string_value = true;
					break;
				case OCRPT_RESULT_NUMBER:
					ocrpt_mem_string_append_printf(fstring, "%s", "%d");
					break;
				case OCRPT_RESULT_DATETIME:
					ocrpt_mem_string_append_printf(fstring, "%s", nl_langinfo_l(D_FMT, o->locale));
					break;
				}
			}
		}

		if (string_value) {
			if (has_translate) {
				locale_t locale = uselocale(o->locale);
				ocrpt_mem_string_append_printf(string, "%s", dgettext(o->textdomain, le->value->result[o->residx]->string->str));
				uselocale(locale);
			} else
				ocrpt_mem_string_append_printf(string, "%s", le->value->result[o->residx]->string->str);
			ocrpt_format_string_literal(o, NULL, rstring, fstring, string);
		} else if (has_value)
			ocrpt_format_string(o, NULL, rstring, fstring, &le->value, 1);

		assert(rstring);
		pango_layout_set_text(le->layout, rstring->str, rstring->len);

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
	ocrpt_cairo_create(o);

	cairo_save(o->cr);

	ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };
	if (le->bgcolor && le->bgcolor->result[o->residx] && le->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && le->bgcolor->result[o->residx]->string)
		ocrpt_get_color(le->bgcolor->result[o->residx]->string->str, &bgcolor, true);
	else if (l->bgcolor && l->bgcolor->result[o->residx] && l->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && l->bgcolor->result[o->residx]->string)
		ocrpt_get_color(l->bgcolor->result[o->residx]->string->str, &bgcolor, true);

	cairo_set_source_rgb(o->cr, bgcolor.r, bgcolor.g, bgcolor.b);
	cairo_set_line_width(o->cr, 0.0);
	/*
	 * The background filler is 0.1 points wider.
	 * This way, there's no lines between the line elements.
	 */
	cairo_rectangle(o->cr, page_indent + le->start, y, le->width_computed + 0.1, l->line_height);
	cairo_fill(o->cr);

	ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
	if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
		ocrpt_get_color(le->color->result[o->residx]->string->str, &color, true);
	else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
		ocrpt_get_color(l->color->result[o->residx]->string->str, &color, true);

	cairo_set_source_rgb(o->cr, color.r, color.g, color.b);

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
			cairo_rectangle(o->cr, page_indent + le->start, y, le->width_computed, l->line_height);
			cairo_clip(o->cr);
		}

		if (le->link && le->link->result[o->residx] && le->link->result[o->residx]->type == OCRPT_RESULT_STRING && le->link->result[o->residx]->string)
			link = le->link->result[o->residx]->string->str;

		if (link) {
			ocrpt_string *uri = ocrpt_mem_string_new_printf("uri='%s'", link);
			cairo_tag_begin(o->cr, CAIRO_TAG_LINK, uri->str);
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

		cairo_move_to(o->cr, page_indent + x1, y + l->ascent);
		pango_cairo_show_layout_line(o->cr, le->pline);

		if (link)
			cairo_tag_end(o->cr, CAIRO_TAG_LINK);
	}

	cairo_restore(o->cr);

	if (l->current_line + 1 < le->lines) {
		le->pline = pango_layout_get_line(le->layout, l->current_line + 1);
		pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
	} else
		le->pline = NULL;
}

void ocrpt_pdf_draw_hline(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_hline *hline, double page_width, double page_indent, double page_position, double size) {
	double indent, length;

	if (hline->indent && hline->indent->result[o->residx] && hline->indent->result[o->residx]->type == OCRPT_RESULT_NUMBER && hline->indent->result[o->residx]->number_initialized)
		indent = mpfr_get_d(hline->indent->result[o->residx]->number, o->rndmode);
	else
		indent = 0.0;

	if (hline->length && hline->length->result[o->residx] && hline->length->result[o->residx]->type == OCRPT_RESULT_NUMBER && hline->length->result[o->residx]->number_initialized) {
		double size_multiplier;

		if (o->size_unit_set && o->size_in_points)
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

	cairo_set_source_rgb(o->cr, color.r, color.g, color.b);
	cairo_set_line_width(o->cr, 0.0);
	cairo_rectangle(o->cr, page_indent + indent, page_position, length, size);
	cairo_fill(o->cr);
}

void ocrpt_pdf_draw_rectangle(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_color *color, double line_width, double x, double y, double width, double height) {
	ocrpt_cairo_create(o);

	cairo_set_source_rgb(o->cr, color->r, color->g, color->b);
	cairo_set_line_width(o->cr, line_width);
	cairo_rectangle(o->cr, x, y, width, height);
	cairo_stroke(o->cr);
}

void ocrpt_pdf_finalize(opencreport *o) {
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

	for (page = o->pages; page; page = page->next) {
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
}

void ocrpt_pdf_init(opencreport *o) {
	memset(&o->output_functions, 0, sizeof(ocrpt_output_functions));
	o->output_functions.draw_hline = ocrpt_pdf_draw_hline;
	o->output_functions.get_text_sizes = ocrpt_pdf_get_text_sizes;
	o->output_functions.draw_text = ocrpt_pdf_draw_text;
	o->output_functions.draw_image = ocrpt_pdf_draw_image;
	o->output_functions.draw_rectangle = ocrpt_pdf_draw_rectangle;
	o->output_functions.finalize = ocrpt_pdf_finalize;
}
