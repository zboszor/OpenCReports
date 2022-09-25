/*
 * OpenCReports main module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <langinfo.h>
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

static void ocrpt_pdf_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_image *img, double x, double y, double w, double h) {
	ocrpt_image_file *img_file = img->img_file;

	if (!img_file)
		return;

	cairo_t *cr = cairo_create((cairo_surface_t *)o->current_page->data);

	cairo_translate(cr, x, y);
	cairo_scale(cr, w / img->img_file->width, h / img->img_file->height);

	cairo_set_source_surface(cr, img_file->surface, 0.0, 0.0);

	if (img->img_file->rsvg) {
		RsvgRectangle rect = { .x = 0.0, .y = 0.0, .width = img_file->width, .height = img_file->height };
		rsvg_handle_render_document(img_file->rsvg, cr, &rect, NULL);
	}

	cairo_paint(cr);

	cairo_destroy(cr);
}

void ocrpt_pdf_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_line *l, ocrpt_line_element *le, double total_width) {
	cairo_surface_t *cs = NULL;
	cairo_t *cr;
	PangoLayout *layout;
	PangoFontDescription *font_description;
	double size, w;

	if (o && o->current_page)
		cr = cairo_create((cairo_surface_t *)o->current_page->data);
	else {
		cs = ocrpt_layout_new_page(o, o->paper, false);
		cr = cairo_create(cs);
	}

	font_description = pango_font_description_new();

	if (le->font_name && le->font_name->result[o->residx] && le->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && le->font_name->result[o->residx]->string)
		le->font = le->font_name->result[o->residx]->string->str;
	else if (l->font_name && l->font_name->result[o->residx] && l->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && l->font_name->result[o->residx]->string)
		le->font = l->font_name->result[o->residx]->string->str;
	else if (r && r->font_name)
		le->font = r->font_name;
	else if (p->font_name)
		le->font = p->font_name;
	else
		le->font = "Courier";

	if (le && le->font_size && le->font_size->result[o->residx] && le->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->font_size->result[o->residx]->number_initialized)
		size = mpfr_get_d(le->font_size->result[o->residx]->number, o->rndmode);
	else if (l && l->font_size && l->font_size->result[o->residx] && l->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->font_size->result[o->residx]->number_initialized)
		size = mpfr_get_d(l->font_size->result[o->residx]->number, o->rndmode);
	else if (r && r->font_size_set)
		size = r->font_size;
	else if (p && p->font_size_set)
		size = p->font_size;
	else
		size = OCRPT_DEFAULT_FONT_SIZE;

	le->fontsz = size;

	bool has_format = false;
	bool has_value = false;

	if (le->format && le->format->result[o->residx] && le->format->result[o->residx]->type == OCRPT_RESULT_STRING && le->format->result[o->residx]->string)
		has_format = true;

	if (le->value && le->value->result[o->residx] &&
		(
			(le->value->result[o->residx]->type == OCRPT_RESULT_STRING && le->value->result[o->residx]->string) ||
			(le->value->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->value->result[o->residx]->number_initialized) ||
			(le->value->result[o->residx]->type == OCRPT_RESULT_DATETIME && (le->value->result[o->residx]->date_valid || le->value->result[o->residx]->time_valid))
		))
		has_value = true;

	ocrpt_string *string = ocrpt_mem_string_resize(le->value_str, 16);
	if (string) {
		le->value_str = string;
		string->len = 0;
	}

	if (has_format && has_value)
		ocrpt_format_string(o, NULL, string, le->format->result[o->residx]->string->str, le->format->result[o->residx]->string->len, &le->value, 1);
	else if (has_value) {
		const char *fmt = NULL;
		int fmtlen = 0;

		switch (le->value->result[o->residx]->type) {
		case OCRPT_RESULT_STRING:
		case OCRPT_RESULT_ERROR:
			fmt = "%s";
			fmtlen = 2;
			break;
		case OCRPT_RESULT_NUMBER:
			fmt = "%d";
			fmtlen = 2;
			break;
		case OCRPT_RESULT_DATETIME:
			fmt = nl_langinfo_l(D_FMT, o->locale);
			fmtlen = strlen(fmt);
			break;
		}

		ocrpt_format_string(o, NULL, string, fmt, fmtlen, &le->value, 1);
	}

	pango_font_description_set_family(font_description, le->font);

	bool bold = false, italic = false;

	if (le->bold && le->bold->result[o->residx] && le->bold->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->bold->result[o->residx]->number_initialized) {
		bold = !!mpfr_get_ui(le->bold->result[o->residx]->number, o->rndmode);
		le->bold_is_set = true;
		le->bold_value = bold;
	} else if (l->bold && l->bold->result[o->residx] && l->bold->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->bold->result[o->residx]->number_initialized) {
		bold = !!mpfr_get_ui(l->bold->result[o->residx]->number, o->rndmode);
		l->bold_is_set = true;
		l->bold_value = bold;
	}

	if (le->italic && le->italic->result[o->residx] && le->italic->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->italic->result[o->residx]->number_initialized) {
		italic = !!mpfr_get_ui(le->italic->result[o->residx]->number, o->rndmode);
		le->italic_is_set = true;
		le->italic_value = italic;
	} else if (l->italic && l->italic->result[o->residx] && l->italic->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->italic->result[o->residx]->number_initialized) {
		italic = !!mpfr_get_ui(l->italic->result[o->residx]->number, o->rndmode);
		l->italic_is_set = true;
		l->italic_value = italic;
	}

	pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_font_description_set_style(font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_absolute_size(font_description, le->fontsz * PANGO_SCALE);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, font_description);

	PangoContext *context = pango_layout_get_context(layout);
	PangoLanguage *language = pango_context_get_language(context);
	PangoFontMetrics *metrics = pango_context_get_metrics(context, font_description, language);

	int char_width = pango_font_metrics_get_approximate_char_width(metrics);
	le->font_width = (double)char_width / PANGO_SCALE;

	/* Start accounting memo lines */
	if (le->memo)
		pango_layout_set_wrap(layout, le->memo_wrap_chars ? PANGO_WRAP_CHAR : PANGO_WRAP_WORD);
	else
		pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);

	pango_layout_set_text(layout, le->value_str->str, le->value_str->len);

	if (le->width && le->width->result[o->residx] && le->width->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->width->result[o->residx]->number_initialized) {
		w = mpfr_get_d(le->width->result[o->residx]->number, o->rndmode);
		if (!o->size_in_points)
			w *= l->font_width;
	} else if (ocrpt_list_length(l->elements) == 1) {
		w = total_width;
	} else {
		PangoRectangle logical_rect;

		/* This assumes pango_layout_set_text() was called previously */
		pango_layout_get_extents(layout, NULL, &logical_rect);
		w = (double)logical_rect.width / PANGO_SCALE;
	}

	le->width_computed = w;

	if (le->memo)
		pango_layout_set_width(layout, w * PANGO_SCALE);

	if (le->memo) {
		le->lines = pango_layout_get_line_count(layout);
		if (le->memo_max_lines && le->memo_max_lines < le->lines)
			le->lines = le->memo_max_lines;
	} else
		le->lines = 1;

	le->ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
	le->descent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;

	g_object_unref(layout);
	pango_font_description_free(font_description);

	cairo_destroy(cr);
	if (cs)
		cairo_surface_destroy(cs);
}

void ocrpt_pdf_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_line *l, ocrpt_line_element *le, double page_indent, double y) {
	cairo_surface_t *cs = NULL;
	cairo_t *cr;
	PangoLayout *layout;
	PangoFontDescription *font_description;
	PangoAlignment align;

	if (o && o->current_page)
		cr = cairo_create((cairo_surface_t *)o->current_page->data);
	else {
		cs = ocrpt_layout_new_page(o, o->paper, false);
		cr = cairo_create(cs);
	}

	if (le->align && le->align->result[o->residx] && le->align->result[o->residx]->type == OCRPT_RESULT_STRING && le->align->result[o->residx]->string) {
		const char *alignment = le->align->result[o->residx]->string->str;
		if (strcasecmp(alignment, "right") == 0)
			align = PANGO_ALIGN_RIGHT;
		else if (strcasecmp(alignment, "center") == 0)
			align = PANGO_ALIGN_CENTER;
		else
			align = PANGO_ALIGN_LEFT;
	} else
		align = PANGO_ALIGN_LEFT;

	font_description = pango_font_description_new();

	pango_font_description_set_family(font_description, le->font);

	bool bold = false, italic = false;

	if (le->bold_is_set)
		bold = le->bold_value;
	else if (l->bold_is_set)
		bold = l->bold_value;

	if (le->italic_is_set)
		italic = le->italic;
	else if (l->italic_is_set)
		italic = l->italic;

	pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_font_description_set_style(font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_absolute_size(font_description, le->fontsz * PANGO_SCALE);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, font_description);

	pango_layout_set_alignment(layout, align);

	PangoAttrList *alist = pango_layout_get_attributes(layout);
	if (!alist)
		alist = pango_attr_list_new();

	PangoAttribute *nohyph = pango_attr_insert_hyphens_new(FALSE);
	pango_attr_list_insert(alist, nohyph);
	pango_layout_set_attributes(layout, alist);

	double field_width = le->width_computed;

	if (le->memo) {
		pango_layout_set_width(layout, le->width_computed * PANGO_SCALE);
		pango_layout_set_wrap(layout, le->memo_wrap_chars ? PANGO_WRAP_CHAR : PANGO_WRAP_WORD);
	} else {
		pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
	}

	pango_layout_set_text(layout, le->value_str->str, le->value_str->len);

	PangoLayoutLine *pline = (l->current_line < l->maxlines) ? pango_layout_get_line(layout, l->current_line) : NULL;
	PangoRectangle logical_rect = {};
	bool use_bb = false;

	if (pline)
		pango_layout_line_get_extents(pline, NULL, &logical_rect);

	if (!le->memo) {
		double render_width = (double)logical_rect.width / PANGO_SCALE;

		if (pd && (pd->column_width < le->start + le->width_computed))
			field_width = pd->column_width - le->start;

		use_bb = (l->current_line == 0 && render_width > field_width);
	}

	ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };
	if (le->bgcolor && le->bgcolor->result[o->residx] && le->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && le->bgcolor->result[o->residx]->string)
		ocrpt_get_color(o, le->bgcolor->result[o->residx]->string->str, &bgcolor, true);
	else if (l->bgcolor && l->bgcolor->result[o->residx] && l->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && l->bgcolor->result[o->residx]->string)
		ocrpt_get_color(o, l->bgcolor->result[o->residx]->string->str, &bgcolor, true);

	cairo_set_source_rgb(cr, bgcolor.r, bgcolor.g, bgcolor.b);
	cairo_set_line_width(cr, 0.0);
	/*
	 * The background filler is 0.1 points wider.
	 * This way, there's no lines between the line elements.
	 */
	cairo_rectangle(cr, page_indent + le->start, y, field_width + 0.1, l->line_height);
	cairo_fill(cr);

	ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
	if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
		ocrpt_get_color(o, le->color->result[o->residx]->string->str, &color, true);
	else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
		ocrpt_get_color(o, l->color->result[o->residx]->string->str, &color, true);

	cairo_set_source_rgb(cr, color.r, color.g, color.b);

	if (use_bb) {
		/*
		 * Apply the bounding box over the text piece
		 * so it doesn't spill out. While the text
		 * is not shown in print, by double clicking
		 * on such a masked piece of text the whole
		 * of it is shown and can be copy&pasted.
		 */
		cairo_rectangle(cr, page_indent + le->start, y, field_width, l->line_height);
		cairo_clip(cr);
	}

	if (pline) {
		char *link = NULL;

		if (le->link && le->link->result[o->residx] && le->link->result[o->residx]->type == OCRPT_RESULT_STRING && le->link->result[o->residx]->string)
			link = le->link->result[o->residx]->string->str;

		if (link) {
			ocrpt_string *uri = ocrpt_mem_string_new_printf("uri='%s'", link);
			cairo_tag_begin(cr, CAIRO_TAG_LINK, uri->str);
			ocrpt_mem_string_free(uri, true);
		}

		double x1;

		switch (align) {
		default:
		case PANGO_ALIGN_LEFT:
			x1 = le->start;
			break;
		case PANGO_ALIGN_CENTER:
			x1 = le->start + (le->width_computed - ((double)logical_rect.width / PANGO_SCALE)) / 2.0;
			break;
		case PANGO_ALIGN_RIGHT:
			x1 = le->start + le->width_computed - ((double)logical_rect.width / PANGO_SCALE);
			break;
		}

		cairo_move_to(cr, page_indent + x1, y + l->ascent);
		pango_cairo_show_layout_line(cr, pline);

		if (link)
			cairo_tag_end(cr, CAIRO_TAG_LINK);
	}

	g_object_unref(layout);
	pango_font_description_free(font_description);

	cairo_destroy(cr);
	if (cs)
		cairo_surface_destroy(cs);
}

void ocrpt_pdf_draw_hline(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_hline *hline, double page_width, double page_indent, double page_position, double size) {
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

	cairo_surface_t *cs = NULL;
	cairo_t *cr;
	ocrpt_color color;
	char *color_name = NULL;

	if (o && o->current_page)
		cr = cairo_create((cairo_surface_t *)o->current_page->data);
	else {
		cs = ocrpt_layout_new_page(o, o->paper, false);
		cr = cairo_create(cs);
	}

	if (hline->color && hline->color->result[o->residx] && hline->color->result[o->residx]->type == OCRPT_RESULT_STRING && hline->color->result[o->residx]->string)
		color_name = hline->color->result[o->residx]->string->str;

	ocrpt_get_color(o, color_name, &color, false);

	cairo_set_source_rgb(cr, color.r, color.g, color.b);
	cairo_set_line_width(cr, 0.0);
	cairo_rectangle(cr, page_indent + indent, page_position, length, size);
	cairo_fill(cr);

	cairo_destroy(cr);
	if (cs)
		cairo_surface_destroy(cs);
}

void ocrpt_pdf_draw_rectangle(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_color *color, double line_width, double x, double y, double width, double height) {
	cairo_surface_t *cs = NULL;
	cairo_t *cr;

	if (o && o->current_page)
		cr = cairo_create((cairo_surface_t *)o->current_page->data);
	else {
		cs = ocrpt_layout_new_page(o, o->paper, false);
		cr = cairo_create(cs);
	}

	cairo_set_source_rgb(cr, color->r, color->g, color->b);
	cairo_set_line_width(cr, line_width);
	cairo_rectangle(cr, x, y, width, height);
	cairo_stroke(cr);

	cairo_destroy(cr);
	if (cs)
		cairo_surface_destroy(cs);
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

		//cairo_surface_destroy(surface);
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
