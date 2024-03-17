/*
 * OpenCReports PDF output driver
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <ctype.h>
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
#include "barcode.h"
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

static void *ocrpt_pdf_get_new_page(opencreport *o, ocrpt_part *p) {
	return ocrpt_pdf_new_page(o, p->paper, p->landscape);
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

static void ocrpt_pdf_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, bool last, double page_width, double page_indent, double x, double y, double w, double h) {
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
	 * The background filler is 0.2 points wider.
	 * This way, there's no lines between the line elements or lines.
	 */
	cairo_set_source_rgb(priv->cr, img->bg.r, img->bg.g, img->bg.b);
	cairo_set_line_width(priv->cr, 0.0);
	cairo_rectangle(priv->cr, x, y, w + (last ? 0.0 : 0.2), h + 0.2);
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

static void ocrpt_pdf_draw_barcode(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_barcode *bc, bool last, double page_width, double page_indent, double x, double y, double w, double h) {
	if (bc->barcode_width == 0.0)
		return;

	pdf_private_data *priv = o->output_private;

	ocrpt_cairo_create(o);

	cairo_save(priv->cr);

	if (pd && (page_indent + pd->column_width < x + w)) {
		cairo_rectangle(priv->cr, x, y, x + w - page_indent - pd->column_width, h);
		cairo_clip(priv->cr);
	}

	cairo_save(priv->cr);

	ocrpt_color bg = { 1.0, 1.0, 1.0 };
	ocrpt_color fg = { 0.0, 0.0, 0.0 };

	if (bc->color && bc->color->result[o->residx] && bc->color->result[o->residx]->type == OCRPT_RESULT_STRING && bc->color->result[o->residx]->string)
		ocrpt_get_color(bc->color->result[o->residx]->string->str, &fg, false);
	else if (line && line->bgcolor && line->bgcolor->result[o->residx] && line->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && line->bgcolor->result[o->residx]->string)
		ocrpt_get_color(line->bgcolor->result[o->residx]->string->str, &fg, false);

	if (bc->bgcolor && bc->bgcolor->result[o->residx] && bc->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && bc->bgcolor->result[o->residx]->string)
		ocrpt_get_color(bc->bgcolor->result[o->residx]->string->str, &bg, true);
	else if (line && line->bgcolor && line->bgcolor->result[o->residx] && line->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && line->bgcolor->result[o->residx]->string)
		ocrpt_get_color(line->bgcolor->result[o->residx]->string->str, &bg, true);

	/*
	 * The background filler is 0.2 points wider.
	 * This way, there's no lines between the line elements or lines.
	 */
	cairo_set_source_rgb(priv->cr, bg.r, bg.g, bg.b);
	cairo_set_line_width(priv->cr, 0.0);
	cairo_rectangle(priv->cr, x, y, w + (last ? 0.0 : 0.2), h + 0.2);
	cairo_fill(priv->cr);

	cairo_restore(priv->cr);

	if (bc->encoded_width > 0 && (!line || !line->current_line)) {
		cairo_save(priv->cr);

		int32_t first_space = bc->encoded->str[0] - '0';

		if (!line) {
			cairo_translate(priv->cr, x, y);
			cairo_scale(priv->cr, w / ((bc->encoded_width - first_space) + 20.0), h / bc->barcode_height);
		} else {
			double w1 = h * bc->barcode_width / bc->barcode_height;

			cairo_translate(priv->cr, x, y);
			cairo_scale(priv->cr, w1 / ((bc->encoded_width - first_space) + 20.0), h / bc->barcode_height);
		}

		int32_t pos = 10;

		for (int32_t bar = 1; bar < bc->encoded->len; bar++) {
			char e = bc->encoded->str[bar];

			/* special cases, ignore */
			if (e == '+' || e == '-')
				continue;

			int32_t width = isdigit(e) ? e - '0' : e - 'a' + 1;

			if (bar % 2) {
				cairo_set_source_rgb(priv->cr, fg.r, fg.g, fg.b);
				cairo_set_line_width(priv->cr, 0.0);
				cairo_rectangle(priv->cr, pos, 0.0, (double)width - INK_SPREADING_TOLERANCE, bc->barcode_height);
				cairo_fill(priv->cr);
			}

			pos += width;
		}

		cairo_restore(priv->cr);
	}

	cairo_restore(priv->cr);
}

static void ocrpt_pdf_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, bool last, double page_width, double page_indent, double y) {
	pdf_private_data *priv = o->output_private;

	ocrpt_cairo_create(o);

	cairo_save(priv->cr);

	ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };
	if (le->bgcolor && le->bgcolor->result[o->residx] && le->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && le->bgcolor->result[o->residx]->string)
		ocrpt_get_color(le->bgcolor->result[o->residx]->string->str, &bgcolor, true);
	else if (l->bgcolor && l->bgcolor->result[o->residx] && l->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && l->bgcolor->result[o->residx]->string)
		ocrpt_get_color(l->bgcolor->result[o->residx]->string->str, &bgcolor, true);

	double rest_start = 0.0;
	double rest_width = 0.0;
	if (last) {
		rest_start = le->start + le->width_computed;
		rest_width = page_width - rest_start;
	}

	cairo_set_source_rgb(priv->cr, bgcolor.r, bgcolor.g, bgcolor.b);
	cairo_set_line_width(priv->cr, 0.0);
	/*
	 * The background filler is 0.2 points wider.
	 * This way, there's no lines between the line elements or lines.
	 */
	cairo_rectangle(priv->cr, page_indent + le->start, y, le->width_computed + (last ? 0.0 : 0.2), l->line_height + 0.2);
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

	if (last && le->start + le->width_computed < page_width) {
		ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };

		cairo_set_source_rgb(priv->cr, bgcolor.r, bgcolor.g, bgcolor.b);
		cairo_set_line_width(priv->cr, 0.0);
		/*
		 * The background filler is 0.2 points wider.
		 * This way, there's no lines between the line elements or lines.
		 */
		cairo_rectangle(priv->cr, page_indent + rest_start, y, rest_width, l->line_height + 0.2);
		cairo_fill(priv->cr);
	}

	cairo_restore(priv->cr);

	if (l->current_line + 1 < le->lines) {
		le->pline = pango_layout_get_line(le->layout, l->current_line + 1);
		pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
	} else
		le->pline = NULL;
}

static void ocrpt_pdf_draw_hline(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_hline *hline, double page_width, double page_indent, double page_position, double size) {
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
	double wider = 0.0;
	if (output->iter->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)output->iter->next->data;
		if (oe->type != OCRPT_OUTPUT_HLINE)
			wider = 0.2;
	}
	cairo_rectangle(priv->cr, page_indent + indent, page_position, length, size + wider);
	cairo_fill(priv->cr);
}

static void ocrpt_pdf_draw_rectangle(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_color *color, double line_width, double x, double y, double width, double height) {
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

static void ocrpt_pdf_finalize(opencreport *o) {
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
	priv->pages = NULL;

	ocrpt_common_finalize(o);

	ocrpt_string **content_type = ocrpt_mem_malloc(3 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: application/pdf");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_pdf_init(opencreport *o) {
	ocrpt_common_init(o, sizeof(pdf_private_data), 4096, 65536);
	o->output_functions.get_new_page = ocrpt_pdf_get_new_page;
	o->output_functions.draw_hline = ocrpt_pdf_draw_hline;
	o->output_functions.prepare_set_font_sizes = ocrpt_cairo_create;
	o->output_functions.set_font_sizes = ocrpt_common_set_font_sizes;
	o->output_functions.prepare_get_text_sizes = ocrpt_cairo_create;
	o->output_functions.get_text_sizes = ocrpt_common_get_text_sizes;
	o->output_functions.draw_text = ocrpt_pdf_draw_text;
	o->output_functions.draw_image = ocrpt_pdf_draw_image;
	o->output_functions.draw_barcode = ocrpt_pdf_draw_barcode;
	o->output_functions.draw_rectangle = ocrpt_pdf_draw_rectangle;
	o->output_functions.finalize = ocrpt_pdf_finalize;
	o->output_functions.supports_page_break = true;
	o->output_functions.supports_column_break = true;
	o->output_functions.supports_pd_height = true;
	o->output_functions.supports_report_height = true;
	o->output_functions.support_bbox = true;
	o->output_functions.support_any_font = true;
	o->output_functions.support_fontdesc = true;
	o->output_functions.line_element_font = true;
}
