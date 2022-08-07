/*
 * Formatting utilities
 *
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <cairo.h>
#include <cairo-svg.h>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>

#include "opencreport.h"
#include "datasource.h"
#include "layout.h"

/*
 * In precalculate mode no output is produced.
 * That run is to compute page elements' locations
 * and the final number of pages.
 */

static void ocrpt_layout_line(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_line *line, double page_width, double page_indent, double *page_position) {
	double next_start, maxascent = 0.0, maxdescent = 0.0;
	int maxrows = 1;

	if (o->output_functions.get_text_sizes) {
		next_start = page_indent;

		for (ocrpt_list *l = line->elements; l; l = l->next) {
			ocrpt_line_element *elem = (ocrpt_line_element *)l->data;
			double width = 0.0, ascent = 0.0, descent = 0.0;
			int rows = 0;

			o->output_functions.get_text_sizes(o, p, pr, pd, r, line, elem, &width, &ascent, &descent);

			elem->start = next_start;
			next_start += width;

			if (maxascent < ascent)
				maxascent = ascent;
			if (maxdescent < descent)
				maxdescent = descent;

			/* TODO: get number of rows here */
			if (maxrows < rows)
				maxrows = rows;
		}
	}

	if (draw && o->output_functions.draw_text) {
		double maxheight = maxascent + maxdescent;

		for (ocrpt_list *l = line->elements; l; l = l->next) {
			ocrpt_line_element *elem = (ocrpt_line_element *)l->data;

			o->output_functions.draw_text(o, p, pr, pd, r, line, elem, elem->start, *page_position, elem->width_computed, maxheight, maxascent - elem->ascent);
		}
	}

	if (line->fontsz > 0.0 && !line->elements)
		*page_position += line->fontsz;
	else
		*page_position += maxascent + maxdescent;
}

static void ocrpt_layout_hline(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_hline *hline, double page_width, double page_indent, double *page_position) {
	if (hline->suppress && hline->suppress->result[o->residx] && hline->suppress->result[o->residx]->type == OCRPT_RESULT_NUMBER && hline->suppress->result[o->residx]->number_initialized) {
		long suppress = mpfr_get_si(hline->suppress->result[o->residx]->number, o->rndmode);

		if (suppress)
			return;
	}

	double size;

	if (hline->size && hline->size->result[o->residx] && hline->size->result[o->residx]->type == OCRPT_RESULT_NUMBER && hline->size->result[o->residx]->number_initialized)
		size = mpfr_get_d(hline->size->result[o->residx]->number, o->rndmode);
	else
		size = 1.0;

	if (draw && o->output_functions.draw_hline)
		o->output_functions.draw_hline(o, p, pr, pd, r, hline, page_width, page_indent, *page_position, size);

	*page_position += size;
}

static bool ocrpt_load_svg(ocrpt_image_file *img) {
	img->rsvg = rsvg_handle_new_from_file(img->name, NULL);
	if (!img->rsvg)
		return false;

	rsvg_handle_get_intrinsic_size_in_pixels(img->rsvg, &img->width, &img->height);

	img->surface = cairo_svg_surface_create(NULL, img->width, img->height);

	return true;
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

static bool ocrpt_load_pixbuf(ocrpt_image_file *img) {
	img->pixbuf = gdk_pixbuf_new_from_file(img->name, NULL);
	if (!img->pixbuf)
		return false;

	img->width = gdk_pixbuf_get_width(img->pixbuf);
	img->height = gdk_pixbuf_get_height(img->pixbuf);

	img->surface = _cairo_new_surface_from_pixbuf(img->pixbuf);

	return true;
}

static void ocrpt_layout_image(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_image *image, double page_width, double page_indent, double *page_position) {
	if (!draw) {
		/* Don't render the image if the filename, the width or the height are not set. */
		if (!image->value || !image->value->result[o->residx] || image->value->result[o->residx]->type != OCRPT_RESULT_STRING || !image->value->result[o->residx]->string)
			return;

		if (!image->width || !image->width->result[o->residx] || image->width->result[o->residx]->type != OCRPT_RESULT_NUMBER || !image->width->result[o->residx]->number_initialized)
			return;

		if (!image->height || !image->height->result[o->residx] || image->height->result[o->residx]->type != OCRPT_RESULT_NUMBER || !image->height->result[o->residx]->number_initialized)
			return;

		char *img_filename = ocrpt_find_file(o, image->value->result[o->residx]->string->str);

		if (!img_filename)
			return;

		ocrpt_image_file *imgf = NULL;
		for (ocrpt_list *il = o->images; il; il = il->next) {
			ocrpt_image_file *img = (ocrpt_image_file *)il->data;
			if (strcmp(img_filename, img->name) == 0) {
				ocrpt_strfree(img_filename);
				imgf = img;
				break;
			}
		}

		if (!imgf) {
			imgf = ocrpt_mem_malloc(sizeof(ocrpt_image_file));
			memset(imgf, 0, sizeof(ocrpt_image_file));
			imgf->name = img_filename;

			bool svg = false;
			bool valid = false;

			if (image->imgtype && image->imgtype->result[o->residx] && image->imgtype->result[o->residx]->type == OCRPT_RESULT_STRING && image->imgtype->result[o->residx]->string) {
				svg = strcasecmp(image->imgtype->result[o->residx]->string->str, "svg") == 0;
			} else {
				ocrpt_string *s = image->value->result[o->residx]->string;
				if (s->len > 4)
					svg = strcasecmp(s->str + s->len - 4, ".svg") == 0;
			}

			if (svg)
				valid = ocrpt_load_svg(imgf);
			else
				valid = ocrpt_load_pixbuf(imgf);

			if (valid) {
				o->images = ocrpt_list_append(o->images, imgf);
				image->img_file = imgf;
			} else
				ocrpt_image_free(imgf);
		}
	}

	double height = 0.0;
	double width = 0.0;

	if (image->img_file) {
		height = mpfr_get_d(image->height->result[o->residx]->number, o->rndmode);
		width = mpfr_get_d(image->width->result[o->residx]->number, o->rndmode);

		if (draw && o->output_functions.draw_image)
			o->output_functions.draw_image(o, p, pr, pd, r, image, page_indent, *page_position, width, height);
	}

	p->old_page_position = *page_position;
	p->current_image_height = height;
	p->current_image_width = width;
}

void ocrpt_layout_output_internal(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_list *output_list, double page_width, double page_indent, double *page_position) {
	bool had_image = false;

	if (r) {
		p->old_page_position = *page_position;
		p->current_image_width = 0.0;
		p->current_image_height = 0.0;
	}

	for (ocrpt_list *ol = output_list; ol; ol = ol->next) {
		ocrpt_output *output = (ocrpt_output *)ol->data;
		char *font_name;
		double font_size;

		switch (output->type) {
		case OCRPT_OUTPUT_LINE:
			ocrpt_line *l = (ocrpt_line *)output;

			if (l->font_name && l->font_name->result[o->residx] && l->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && l->font_name->result[o->residx]->string)
				font_name = l->font_name->result[o->residx]->string->str;
			else
				font_name = (r && r->font_name) ? r->font_name : (p->font_name ? p->font_name : "Courier");

			if (l->font_size && l->font_size->result[o->residx] && l->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->font_size->result[o->residx]->number_initialized)
				font_size = mpfr_get_d(l->font_size->result[o->residx]->number, o->rndmode);
			else
				font_size = r ? r->font_size : p->font_size;

			ocrpt_layout_set_font_sizes(o, font_name, font_size, false, false, &l->fontsz, &l->font_width);
			ocrpt_layout_line(draw, o, p, pr, pd, r, l, page_width - p->current_image_width, page_indent + p->current_image_width, page_position);
			break;
		case OCRPT_OUTPUT_HLINE:
			ocrpt_hline *hl = (ocrpt_hline *)output;

			font_name = (r && r->font_name) ? r->font_name : (p->font_name ? p->font_name : "Courier");

			if (hl->font_size && hl->font_size->result[o->residx] && hl->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && hl->font_size->result[o->residx]->number_initialized)
				font_size = mpfr_get_d(hl->font_size->result[o->residx]->number, o->rndmode);
			else
				font_size = (r ? r->font_size : p->font_size);

			ocrpt_layout_set_font_sizes(o, font_name, font_size, false, false, NULL, &hl->font_width);
			ocrpt_layout_hline(draw, o, p, pr, pd, r, hl, page_width - p->current_image_width, page_indent + p->current_image_width, page_position);
			break;
		case OCRPT_OUTPUT_IMAGE:
			if (had_image)
				*page_position = p->old_page_position + p->current_image_height;
			ocrpt_layout_image(draw, o, p, pr, pd, r, (ocrpt_image *)output, page_width, page_indent, page_position);
			had_image = true;
			p->old_page_position = *page_position;
			break;
		}
	}

	if (had_image)
		*page_position = p->old_page_position + p->current_image_height;

	if (r) {
		p->current_image_width = 0.0;
		p->current_image_height = 0.0;
	}
}

void ocrpt_layout_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_list *output_list, unsigned int rows, double *page_indent, double *page_position) {
	ocrpt_list *old_current_page;
	double old_page_position, new_page_position;
	bool page_break = false;

	if (!o->current_page)
		ocrpt_layout_add_new_page(o, p, pr, pd, r, rows, page_indent, page_position);

	/*
	 * The bool value in o->precalculate means !draw in layout context.
	 * Since the complete <Output> section is (usually) drawn as an atomic unit
	 * ("memo" values will break this!!!) we need to calculate the last
	 * page position without drawing first. So, draw in two rounds.
	 *
	 * 1. In the first round, only detect the last page position.
	 *    If the vertical position exceeds the bottom margin (minus the
	 *    page footer height) go to the next page.
	 * 2. In the second round, Set o->precalculate to what it was and if it's false then draw
	 *    the section.
	 */
	old_current_page = o->current_page;
	old_page_position = new_page_position = *page_position;

	ocrpt_layout_output_internal(false, o, p, pr, pd, r, output_list, r->column_width, *page_indent, &new_page_position);

	if (new_page_position > p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height) {
		r->current_column++;

		if (r->current_column >= r->detail_columns)
			page_break = true;

		if (page_break) {
			r->current_column = 0;
			*page_indent = r->page_indent;

			if (!o->precalculate) {
				new_page_position = ocrpt_layout_top_margin(o, p);
				ocrpt_layout_output_internal(true, o, p, NULL, NULL, NULL, p->pageheader, p->page_width, *page_indent, &new_page_position);
				new_page_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
				ocrpt_layout_output_internal(true, o, p, NULL, NULL, NULL, p->pagefooter, p->page_width, *page_indent, &new_page_position);
			}
			ocrpt_layout_add_new_page(o, p, pr, pd, r, rows, page_indent, &new_page_position);
		} else {
			*page_indent += r->column_width + r->column_pad;
			new_page_position = r->page_start;
			ocrpt_layout_output_highprio_fieldheader(o, p, pr, pd, r, rows, page_indent, &new_page_position);
		}

		page_break = true;
	}

	if (!o->precalculate) {
		if (!page_break) {
			o->current_page = old_current_page;
			new_page_position = old_page_position;
		}
		ocrpt_layout_output_internal(true, o, p, pr, pd, r, output_list, r->column_width, *page_indent, &new_page_position);
	}

	*page_position = new_page_position;
}

double ocrpt_layout_top_margin(opencreport *o, ocrpt_part *p) {
	double top_margin;

	if (p->top_margin_set)
		top_margin = p->top_margin;
	else
		top_margin = OCRPT_DEFAULT_TOP_MARGIN;

	return top_margin;
}

double ocrpt_layout_bottom_margin(opencreport *o, ocrpt_part *p) {
	double bottom_margin;

	if (p->bottom_margin_set)
		bottom_margin = p->bottom_margin;
	else
		bottom_margin = OCRPT_DEFAULT_BOTTOM_MARGIN;

	return bottom_margin;
}

double ocrpt_layout_left_margin(opencreport *o, ocrpt_part *p, ocrpt_report *r) {
	double left_margin;

	if (r && r->left_margin_set)
		left_margin = r->left_margin;
	else if (p->left_margin_set)
		left_margin = p->left_margin;
	else
		left_margin = OCRPT_DEFAULT_LEFT_MARGIN;

	return left_margin;
}

double ocrpt_layout_right_margin(opencreport *o, ocrpt_part *p, ocrpt_report *r) {
	double right_margin;

	if (r && r->right_margin_set)
		right_margin = r->right_margin;
	else if (p->right_margin_set)
		right_margin = p->right_margin;
	else
		right_margin = ocrpt_layout_left_margin(o, p, r);

	return right_margin;
}

void ocrpt_layout_output_highprio_fieldheader(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, unsigned int rows, double *page_indent, double *page_position) {
	if (r && r->fieldheader && r->fieldheader_high_priority) {
		/*
		 * Debatable preference in taste:
		 * a) field headers have higher precedence than break headers
		 *    and break footers, meaning the field headers are printed
		 *    once per page at the top, with break headers and footers
		 *    printed after it, or
		 * b) break headers and footers have higher precedence than
		 *    field headers, with break headers printed first, then
		 *    the field headers, followed by all the field details,
		 *    then finallly the break footers.
		 *
		 * It is configurable via <Report field_header_preference="high/low">
		 * with the default "high" value.
		 */
		ocrpt_layout_output(o, p, pr, pd, r, r->fieldheader, rows, page_indent, page_position);
	}
}

void ocrpt_layout_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, unsigned int rows, double *page_indent, double *page_position) {
	if (o->precalculate) {
		if (!o->current_page) {
			if (!o->pages) {
				void *page = ocrpt_layout_new_page(o, p->paper, p->landscape);
				o->pages = ocrpt_list_end_append(o->pages, &o->last_page, page);
			}
			o->current_page = o->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			void *page = ocrpt_layout_new_page(o, p->paper, p->landscape);
			o->pages = ocrpt_list_end_append(o->pages, &o->last_page, page);
			o->current_page = o->last_page;
		}

		if (mpfr_cmp(o->totpages->number, o->pageno->number) < 0)
			mpfr_set(o->totpages->number, o->pageno->number, o->rndmode);
	} else {
		if (!o->current_page) {
			o->current_page = o->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			o->current_page = o->current_page->next;
		}
	}

	r->page_start = *page_position = ocrpt_layout_top_margin(o, p) + p->page_header_height;

	if (rows == 1)
		ocrpt_layout_output(o, p, pr, pd, r, r->reportheader, rows, page_indent, page_position);

	ocrpt_layout_output_highprio_fieldheader(o, p, pr, pd, r, rows, page_indent, page_position);
}

void *ocrpt_layout_new_page(opencreport *o, const ocrpt_paper *paper, bool landscape) {
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

void ocrpt_layout_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width) {
	cairo_surface_t *cs = NULL;
	cairo_t *cr;

	if (o && o->current_page)
		cr = cairo_create((cairo_surface_t *)o->current_page->data);
	else {
		cs = ocrpt_layout_new_page(o, o->paper, false);
		cr = cairo_create(cs);
	}

	PangoLayout *layout;
	PangoFontDescription *font_description;

	font_description = pango_font_description_new();

	pango_font_description_set_family(font_description, font);
	pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_font_description_set_style(font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_absolute_size(font_description, wanted_font_size * PANGO_SCALE);

	layout = pango_cairo_create_layout(cr);
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

	g_object_unref(layout);
	pango_font_description_free(font_description);

	cairo_destroy(cr);
	if (cs)
		cairo_surface_destroy(cs);
}
