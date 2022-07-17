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
#include "formatting.h"
#include "layout.h"
#include "pdf.h"

static cairo_status_t ocrpt_write_pdf(void *closure, const unsigned char *data, unsigned int length) {
	opencreport *o = closure;

	ocrpt_mem_string_append_len_binary(o->output_buffer, (char *)data, length);

	return CAIRO_STATUS_SUCCESS;
}

void ocrpt_pdf_svg(cairo_t *cr, const char *filename, double x, double y, double w, double h) {
	struct {
		double width, height;
	} dimensions;

	RsvgHandle *rsvg = rsvg_handle_new_from_file(filename, NULL);
	if (!rsvg)
		return;

	cairo_save(cr);

	rsvg_handle_get_intrinsic_size_in_pixels(rsvg, &dimensions.width, &dimensions.height);

	cairo_surface_t *svg = cairo_svg_surface_create(NULL, dimensions.width, dimensions.height);
	cairo_set_source_surface(cr, svg, 0.0, 0.0);

	cairo_translate(cr, x, y);
	cairo_scale(cr, w / dimensions.width, h / dimensions.height);

	RsvgRectangle rect = { .x = 0.0, .y = 0.0, .width = dimensions.width, .height = dimensions.height };
	rsvg_handle_render_document(rsvg, cr, &rect, NULL);

	cairo_paint(cr);

	cairo_surface_destroy(svg);
	g_object_unref(rsvg);

	cairo_restore(cr);
}

void ocrpt_pdf_png(cairo_t *cr, const char *filename, double x, double y, double w, double h) {
	struct {
		double width, height;
	} dimensions;
	double scale_x, scale_y;

	cairo_save(cr);

	cairo_surface_t *png = cairo_image_surface_create_from_png(filename);

	dimensions.width = cairo_image_surface_get_width(png);
	dimensions.height = cairo_image_surface_get_height(png);
	scale_x = w / dimensions.width;
	scale_y = h / dimensions.height;

	cairo_translate(cr, x, y);
	cairo_scale(cr, scale_x, scale_y);

	cairo_set_source_surface(cr, png, 0.0, 0.0);
	cairo_paint(cr);

	cairo_surface_destroy(png);

	cairo_restore(cr);
}

static cairo_surface_t *_cairo_new_surface_from_pixbuf(const GdkPixbuf *pixbuf) {
	int width = gdk_pixbuf_get_width(pixbuf);
	int height = gdk_pixbuf_get_height(pixbuf);
	guchar *gdk_pixels = gdk_pixbuf_get_pixels(pixbuf);
	int gdk_rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
	int cairo_stride;
	guchar *cairo_pixels;

	cairo_format_t format;
	cairo_surface_t *surface;
	static const cairo_user_data_key_t key;
	int j;

	format = (n_channels == 3 ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_ARGB32);

	cairo_stride = cairo_format_stride_for_width(format, width);
	cairo_pixels = g_malloc(height * cairo_stride);
	surface = cairo_image_surface_create_for_data((unsigned char *)cairo_pixels, format, width, height, cairo_stride);

	cairo_surface_set_user_data(surface, &key, cairo_pixels, (cairo_destroy_func_t)g_free);

	for (j = height; j; j--) {
		guchar *p = gdk_pixels;
		guchar *q = cairo_pixels;

		if (n_channels == 3) {
			guchar *end = p + 3 * width;

			while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				q[0] = p[2]; q[1] = p[1]; q[2] = p[0];
#else
				q[1] = p[0]; q[2] = p[1]; q[3] = p[2];
#endif
				p += 3; q += 4;
			}
		} else {
			guchar *end = p + 4 * width;
			guint t1,t2,t3;

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

			while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				MULT(q[0], p[2], p[3], t1);
				MULT(q[1], p[1], p[3], t2);
				MULT(q[2], p[0], p[3], t3);
				q[3] = p[3];
#else
				q[0] = p[3];
				MULT(q[1], p[0], p[3], t1);
				MULT(q[2], p[1], p[3], t2);
				MULT(q[3], p[2], p[3], t3);
#endif
				p += 4; q += 4;
			}
#undef MULT
		}

		gdk_pixels += gdk_rowstride;
		cairo_pixels += cairo_stride;
	}

	return surface;
}

void ocrpt_pdf_pixbuf(cairo_t *cr, const char *filename, double x, double y, double w, double h) {
	struct {
		double width, height;
	} dimensions;
	double scale_x, scale_y;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

	if (!pixbuf)
		return;

	cairo_save(cr);

	dimensions.width = gdk_pixbuf_get_width(pixbuf);
	dimensions.height = gdk_pixbuf_get_height(pixbuf);

	cairo_surface_t *surface = _cairo_new_surface_from_pixbuf(pixbuf);

	scale_x = w / dimensions.width;
	scale_y = h / dimensions.height;

	cairo_translate(cr, x, y);
	cairo_scale(cr, scale_x, scale_y);

	cairo_set_source_surface(cr, surface, 0.0, 0.0);
	cairo_paint(cr);

	cairo_surface_destroy(surface);

	cairo_restore(cr);

	g_object_unref(pixbuf);
}

static double ocrpt_pdf_get_font_size_multiplier_for_width(opencreport *o, ocrpt_part *p, ocrpt_report *r, ocrpt_line *l) {
	bool size_in_points = false;

	if (r->size_unit_set)
		size_in_points = r->size_in_points;
	else if (p->size_unit_set)
		size_in_points = p->size_unit_set;
	else if (o->size_unit_set)
		size_in_points = p->size_unit_set;

	if (size_in_points)
		return 1.0;

	return l->font_width;
}

static double ocrpt_pdf_get_left_margin(opencreport *o, ocrpt_part *p, ocrpt_report *r) {
	double left_margin;

	if (r->left_margin_set)
		left_margin = r->left_margin;
	else if (p->left_margin_set)
		left_margin = p->left_margin;
	else
		left_margin = OCRPT_DEFAULT_LEFT_MARGIN;

	return left_margin;
}

void ocrpt_pdf_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_line *l, ocrpt_line_element *le, double *width, double *ascent, double *descent) {
	cairo_t *cr = cairo_create((cairo_surface_t *)o->current_page->data);
	PangoLayout *layout;
	PangoFontDescription *font_description;
	double size, w;

	font_description = pango_font_description_new();

	if (le->font_name && le->font_name->result[o->residx] && le->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && le->font_name->result[o->residx]->string)
		le->font = le->font_name->result[o->residx]->string->str;
	else if (l->font_name && l->font_name->result[o->residx] && l->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && l->font_name->result[o->residx]->string)
		le->font = l->font_name->result[o->residx]->string->str;
	else if (r->font_name)
		le->font = r->font_name;
	else if (p->font_name)
		le->font = p->font_name;
	else
		le->font = "Courier";

	if (le->font_size && le->font_size->result[o->residx] && le->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->font_size->result[o->residx]->number_initialized)
		size = mpfr_get_d(le->font_size->result[o->residx]->number, o->rndmode);
	else if (l->font_size && l->font_size->result[o->residx] && l->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->font_size->result[o->residx]->number_initialized)
		size = mpfr_get_d(l->font_size->result[o->residx]->number, o->rndmode);
	else if (r->font_size_set)
		size = r->font_size;
	else if (p->font_size_set)
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

	if (le->width && le->width->result[o->residx] && le->width->result[o->residx]->type == OCRPT_RESULT_NUMBER && le->width->result[o->residx]->number_initialized) {
		w = mpfr_get_d(le->width->result[o->residx]->number, o->rndmode);
		w *= ocrpt_pdf_get_font_size_multiplier_for_width(o, p, r, l);
	} else {
		int l;
		ocrpt_utf8forward(le->value_str->str, -1, &l, -1, NULL);

		w = l * le->font_width;
	}

	le->width_computed = w;
	*width = w;

	le->ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
	*ascent = le->ascent;
	le->descent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
	*descent = le->descent;

	g_object_unref(layout);
	pango_font_description_free(font_description);

	cairo_destroy(cr);
}

void ocrpt_pdf_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_line *l, ocrpt_line_element *le, double x, double y, double width, double maxheight, double ascentdiff) {
	cairo_t *cr = cairo_create((cairo_surface_t *)o->current_page->data);
	PangoLayout *layout;
	PangoFontDescription *font_description;
	PangoAlignment align;

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
	pango_font_description_set_absolute_size(font_description, 12.0 * PANGO_SCALE);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, font_description);

	pango_layout_set_alignment(layout, align);

	if (le->memo) {
		pango_layout_set_wrap(layout, le->memo_wrap_chars ? PANGO_WRAP_CHAR : PANGO_WRAP_WORD);
	} else {
		PangoAttrList *alist = pango_layout_get_attributes(layout);
		if (!alist)
			alist = pango_attr_list_new();
		PangoAttribute *nohyph = pango_attr_insert_hyphens_new(FALSE);
		pango_attr_list_insert(alist, nohyph);
		pango_layout_set_attributes(layout, alist);
		pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
	}

	pango_layout_set_text(layout, le->value_str->str, le->value_str->len);

	PangoRectangle logical_rect;

	if (le->memo) {
		pango_layout_set_width(layout, width * PANGO_SCALE);
		fprintf(stderr, "ocrpt_pdf_draw_text: MEMO line '%s'\n", le->value_str->str);
		pango_layout_get_extents(layout, NULL, &logical_rect);
	} else {
		pango_layout_get_extents(layout, NULL, &logical_rect);
		double render_width = (double)logical_rect.width / PANGO_SCALE;

		const char *pos = le->value_str->str;
		int len = le->value_str->len;
		while (render_width > width) {
			int len1;
			ocrpt_utf8forward(pos, 1, NULL, len, &len1);
			pos += len1;
			len -= len1;

			pango_layout_set_text(layout, pos, len);
			pango_layout_get_extents(layout, NULL, &logical_rect);
			render_width = (double)logical_rect.width / PANGO_SCALE;
		}
	}

	double left_margin = ocrpt_pdf_get_left_margin(o, p, r);

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
	cairo_rectangle(cr, left_margin + x, y, width + 0.1, maxheight);
	cairo_fill(cr);

	ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
	if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
		ocrpt_get_color(o, le->color->result[o->residx]->string->str, &color, true);
	else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
		ocrpt_get_color(o, l->color->result[o->residx]->string->str, &color, true);

	cairo_set_source_rgb(cr, color.r, color.g, color.b);

	double x1;

	switch (align) {
	default:
	case PANGO_ALIGN_LEFT:
		x1 = x;
		break;
	case PANGO_ALIGN_CENTER:
		x1 = x + (width - ((double)logical_rect.width / PANGO_SCALE)) / 2.0;
		break;
	case PANGO_ALIGN_RIGHT:
		x1 = x + width - ((double)logical_rect.width / PANGO_SCALE);
		break;
	}

	cairo_move_to(cr, left_margin + x1, y + ascentdiff);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
	pango_font_description_free(font_description);

	cairo_destroy(cr);
}

void ocrpt_pdf_draw_hline(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_hline *hline, double page_width, double page_indent, double page_position, double size) {
	double indent, length;
	double left_margin = ocrpt_pdf_get_left_margin(o, p, r);

	if (hline->indent && hline->indent->result[o->residx] && hline->indent->result[o->residx]->type == OCRPT_RESULT_NUMBER && hline->indent->result[o->residx]->number_initialized)
		indent = mpfr_get_d(hline->indent->result[o->residx]->number, o->rndmode);
	else
		indent = 0.0;

	if (hline->length && hline->length->result[o->residx] && hline->length->result[o->residx]->type == OCRPT_RESULT_NUMBER && hline->length->result[o->residx]->number_initialized) {
		double size_multiplier;

		if (r->size_unit_set && r->size_in_points)
			size_multiplier = 1.0;
		else if (p->size_unit_set && p->size_in_points)
			size_multiplier = 1.0;
		else if (o->size_unit_set && o->size_in_points)
			size_multiplier = 1.0;
		else
			size_multiplier = hline->font_width;

		length = mpfr_get_d(hline->length->result[o->residx]->number, o->rndmode) * size_multiplier;
	} else {
		double right_margin;

		if (r->right_margin_set)
			right_margin = r->left_margin;
		else if (p->right_margin_set)
			right_margin = p->right_margin;
		else
			right_margin = left_margin;

		length = page_width - page_indent - left_margin - indent - right_margin;
	}

	cairo_t *cr = cairo_create((cairo_surface_t *)o->current_page->data);
	ocrpt_color color;
	char *color_name = NULL;

	if (hline->color && hline->color->result[o->residx] && hline->color->result[o->residx]->type == OCRPT_RESULT_STRING && hline->color->result[o->residx]->string)
		color_name = hline->color->result[o->residx]->string->str;

	ocrpt_get_color(o, color_name, &color, false);

	cairo_set_source_rgb(cr, color.r, color.g, color.b);
	cairo_set_line_width(cr, 0.0);
	cairo_rectangle(cr, left_margin + indent, page_position, length, size);
	cairo_fill(cr);

	cairo_destroy(cr);
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
	o->output_functions.finalize = ocrpt_pdf_finalize;
}
