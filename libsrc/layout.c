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
#include "ocrpt-private.h"
#include "listutil.h"
#include "exprutil.h"
#include "variables.h"
#include "datasource.h"
#include "parts.h"
#include "layout.h"

/*
 * In precalculate mode no output is produced.
 * That run is to compute page elements' locations
 * and the final number of pages.
 */

static void ocrpt_layout_line_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, double page_width, double *page_position) {
	double next_start;

	line->ascent = 0.0;
	line->descent = 0.0;
	line->maxlines = 1;

	if (o->output_functions.get_text_sizes) {
		next_start = 0;

		for (ocrpt_list *l = line->elements; l; l = l->next) {
			ocrpt_line_element *elem = (ocrpt_line_element *)l->data;

			o->output_functions.get_text_sizes(o, p, pr, pd, r, output, line, elem, page_width - ((pd && pd->border_width_set) ? 2 * pd->border_width: 0.0));

			elem->start = next_start;
			next_start += elem->width_computed;

			if (line->ascent < elem->ascent)
				line->ascent = elem->ascent;
			if (line->descent < elem->descent)
				line->descent = elem->descent;

			if (line->maxlines < elem->lines)
				line->maxlines = elem->lines;

			output->has_memo = output->has_memo || (elem->memo && (elem->lines > 1));
		}
	}

	line->line_height = line->ascent + line->descent;
}

static void ocrpt_layout_line(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, double page_width, double page_indent, double *page_position) {
	if (draw && o->output_functions.draw_text) {
		for (ocrpt_list *l = line->elements; l; l = l->next) {
			ocrpt_line_element *elem = (ocrpt_line_element *)l->data;

			if (elem->start < page_width)
				o->output_functions.draw_text(o, p, pr, pd, r, output, line, elem, page_indent, *page_position);
		}
	}

	if (line->fontsz > 0.0 && !line->elements)
		*page_position += line->fontsz;
	else
		*page_position += line->ascent + line->descent;
}

static void ocrpt_layout_hline(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_hline *hline, double page_width, double page_indent, double *page_position) {
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
		o->output_functions.draw_hline(o, p, pr, pd, r, output, hline, page_width, page_indent, *page_position, size);

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

static void ocrpt_layout_image_setup(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_image *image, double page_width, double page_indent, double *page_position) {
	image->suppress_image = false;

	/* Don't render the image if the filename, the width or the height are not set. */
	if (!image->value || !image->value->result[o->residx] || image->value->result[o->residx]->type != OCRPT_RESULT_STRING || !image->value->result[o->residx]->string) {
		image->suppress_image = true;
		return;
	}

	if (!image->width || !image->width->result[o->residx] || image->width->result[o->residx]->type != OCRPT_RESULT_NUMBER || !image->width->result[o->residx]->number_initialized) {
		image->suppress_image = true;
		return;
	}

	if (!image->height || !image->height->result[o->residx] || image->height->result[o->residx]->type != OCRPT_RESULT_NUMBER || !image->height->result[o->residx]->number_initialized) {
		image->suppress_image = true;
		return;
	}

	char *img_filename = ocrpt_find_file(o, image->value->result[o->residx]->string->str);

	if (!img_filename) {
		image->suppress_image = true;
		return;
	}

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

	if (image->img_file) {
		image->image_width = mpfr_get_d(image->width->result[o->residx]->number, o->rndmode);
		image->image_height = mpfr_get_d(image->height->result[o->residx]->number, o->rndmode);
	}
}

static void ocrpt_layout_image(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_image *image, double page_width, double page_indent, double *page_position) {
	if (draw && o->output_functions.draw_image && image->img_file)
		o->output_functions.draw_image(o, p, pr, pd, r, output, image, page_indent, *page_position, image->image_width, image->image_height);

	output->old_page_position = *page_position;
	output->current_image = image;
}

void ocrpt_layout_output_resolve(opencreport *o, ocrpt_part *p, ocrpt_report *r, ocrpt_output *output) {
	ocrpt_expr_resolve(o, r, output->suppress);
	ocrpt_expr_optimize(o, r, output->suppress);

	for (ocrpt_list *ol = output->output_list; ol; ol = ol->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)ol->data;

		switch (oe->type) {
		case OCRPT_OUTPUT_LINE:
			ocrpt_line *line = (ocrpt_line *)oe;

			ocrpt_expr_resolve(o, r, line->font_name);
			ocrpt_expr_optimize(o, r, line->font_name);

			ocrpt_expr_resolve(o, r, line->font_size);
			ocrpt_expr_optimize(o, r, line->font_size);

			ocrpt_expr_resolve(o, r, line->color);
			ocrpt_expr_optimize(o, r, line->color);

			ocrpt_expr_resolve(o, r, line->bgcolor);
			ocrpt_expr_optimize(o, r, line->bgcolor);

			ocrpt_expr_resolve(o, r, line->bold);
			ocrpt_expr_optimize(o, r, line->bold);

			ocrpt_expr_resolve(o, r, line->italic);
			ocrpt_expr_optimize(o, r, line->italic);

			ocrpt_expr_resolve(o, r, line->suppress);
			ocrpt_expr_optimize(o, r, line->suppress);

			for (ocrpt_list *l = line->elements; l; l = l->next) {
				ocrpt_line_element *elem = (ocrpt_line_element *)l->data;

				ocrpt_expr_resolve(o, r, elem->value);
				ocrpt_expr_optimize(o, r, elem->value);

				ocrpt_expr_resolve(o, r, elem->format);
				ocrpt_expr_optimize(o, r, elem->format);

				ocrpt_expr_resolve(o, r, elem->width);
				ocrpt_expr_optimize(o, r, elem->width);

				ocrpt_expr_resolve(o, r, elem->align);
				ocrpt_expr_optimize(o, r, elem->align);

				ocrpt_expr_resolve(o, r, elem->color);
				ocrpt_expr_optimize(o, r, elem->color);

				ocrpt_expr_resolve(o, r, elem->bgcolor);
				ocrpt_expr_optimize(o, r, elem->bgcolor);

				ocrpt_expr_resolve(o, r, elem->font_name);
				ocrpt_expr_optimize(o, r, elem->font_name);

				ocrpt_expr_resolve(o, r, elem->font_size);
				ocrpt_expr_optimize(o, r, elem->font_size);

				ocrpt_expr_resolve(o, r, elem->bold);
				ocrpt_expr_optimize(o, r, elem->bold);

				ocrpt_expr_resolve(o, r, elem->italic);
				ocrpt_expr_optimize(o, r, elem->italic);

				ocrpt_expr_resolve(o, r, elem->link);
				ocrpt_expr_optimize(o, r, elem->link);

				ocrpt_expr_resolve(o, r, elem->translate);
				ocrpt_expr_optimize(o, r, elem->translate);
			}

			break;
		case OCRPT_OUTPUT_HLINE:
			ocrpt_hline *hline = (ocrpt_hline *)oe;

			ocrpt_expr_resolve(o, r, hline->size);
			ocrpt_expr_optimize(o, r, hline->size);

			ocrpt_expr_resolve(o, r, hline->indent);
			ocrpt_expr_optimize(o, r, hline->indent);

			ocrpt_expr_resolve(o, r, hline->length);
			ocrpt_expr_optimize(o, r, hline->length);

			ocrpt_expr_resolve(o, r, hline->font_size);
			ocrpt_expr_optimize(o, r, hline->font_size);

			ocrpt_expr_resolve(o, r, hline->suppress);
			ocrpt_expr_optimize(o, r, hline->suppress);

			ocrpt_expr_resolve(o, r, hline->color);
			ocrpt_expr_optimize(o, r, hline->color);

			break;
		case OCRPT_OUTPUT_IMAGE:
		case OCRPT_OUTPUT_IMAGEEND:
			break;
		}
	}
}

void ocrpt_layout_output_evaluate(opencreport *o, ocrpt_part *p, ocrpt_report *r, ocrpt_output *output) {
	for (ocrpt_list *ol = output->output_list; ol; ol = ol->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)ol->data;

		switch (oe->type) {
		case OCRPT_OUTPUT_LINE:
			ocrpt_line *line = (ocrpt_line *)oe;

			ocrpt_expr_eval(o, r, line->font_name);
			ocrpt_expr_eval(o, r, line->font_size);
			ocrpt_expr_eval(o, r, line->color);
			ocrpt_expr_eval(o, r, line->bgcolor);
			ocrpt_expr_eval(o, r, line->bold);
			ocrpt_expr_eval(o, r, line->italic);
			ocrpt_expr_eval(o, r, line->suppress);

			for (ocrpt_list *l = line->elements; l; l = l->next) {
				ocrpt_line_element *elem = (ocrpt_line_element *)l->data;

				ocrpt_expr_eval(o, r, elem->value);
				ocrpt_expr_eval(o, r, elem->format);
				ocrpt_expr_eval(o, r, elem->width);
				ocrpt_expr_eval(o, r, elem->align);
				ocrpt_expr_eval(o, r, elem->color);
				ocrpt_expr_eval(o, r, elem->bgcolor);
				ocrpt_expr_eval(o, r, elem->font_name);
				ocrpt_expr_eval(o, r, elem->font_size);
				ocrpt_expr_eval(o, r, elem->bold);
				ocrpt_expr_eval(o, r, elem->italic);
				ocrpt_expr_eval(o, r, elem->link);
				ocrpt_expr_eval(o, r, elem->translate);
			}

			break;
		case OCRPT_OUTPUT_HLINE:
			ocrpt_hline *hline = (ocrpt_hline *)oe;

			ocrpt_expr_eval(o, r, hline->size);
			ocrpt_expr_eval(o, r, hline->indent);
			ocrpt_expr_eval(o, r, hline->length);
			ocrpt_expr_eval(o, r, hline->font_size);
			ocrpt_expr_eval(o, r, hline->suppress);
			ocrpt_expr_eval(o, r, hline->color);
			break;
		case OCRPT_OUTPUT_IMAGE:
		case OCRPT_OUTPUT_IMAGEEND:
			break;
		}
	}
}

void ocrpt_layout_output_internal_preamble(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, double page_width, double page_indent, double *page_position) {
	output->suppress_output = false;

	if (output->suppress && output->suppress->result[o->residx] && output->suppress->result[o->residx]->type == OCRPT_RESULT_NUMBER && output->suppress->result[o->residx]->number_initialized) {
		long suppress = mpfr_get_si(output->suppress->result[o->residx]->number, o->rndmode);

		if (suppress) {
			output->suppress_output = true;
			return;
		}
	}

	if (r)
		output->old_page_position = *page_position;

	for (ocrpt_list *ol = output->output_list; ol; ol = ol->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)ol->data;
		char *font_name;
		double font_size;

		switch (oe->type) {
		case OCRPT_OUTPUT_LINE:
			ocrpt_line *l = (ocrpt_line *)oe;

			l->suppress_line = false;
			if (l->suppress && l->suppress->result[o->residx] && l->suppress->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->suppress->result[o->residx]->number_initialized) {
				long suppress = mpfr_get_si(l->suppress->result[o->residx]->number, o->rndmode);

				if (suppress) {
					l->suppress_line = true;
					break;
				}
			}

			if (l->font_name && l->font_name->result[o->residx] && l->font_name->result[o->residx]->type == OCRPT_RESULT_STRING && l->font_name->result[o->residx]->string)
				font_name = l->font_name->result[o->residx]->string->str;
			else
				font_name = (r && r->font_name) ? r->font_name : (p->font_name ? p->font_name : "Courier");

			if (l->font_size && l->font_size->result[o->residx] && l->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && l->font_size->result[o->residx]->number_initialized)
				font_size = mpfr_get_d(l->font_size->result[o->residx]->number, o->rndmode);
			else
				font_size = r ? r->font_size : p->font_size;

			ocrpt_layout_set_font_sizes(o, output, font_name, font_size, false, false, &l->fontsz, &l->font_width);
			if (output->current_image)
				ocrpt_layout_line_get_text_sizes(o, p, pr, pd, r, output, l, page_width - output->current_image->image_width, page_position);
			else
				ocrpt_layout_line_get_text_sizes(o, p, pr, pd, r, output, l, page_width, page_position);
			break;
		case OCRPT_OUTPUT_HLINE:
			ocrpt_hline *hl = (ocrpt_hline *)oe;

			hl->suppress_hline = false;
			if (hl->suppress && hl->suppress->result[o->residx] && hl->suppress->result[o->residx]->type == OCRPT_RESULT_NUMBER && hl->suppress->result[o->residx]->number_initialized) {
				long suppress = mpfr_get_si(hl->suppress->result[o->residx]->number, o->rndmode);

				if (suppress) {
					hl->suppress_hline = true;
					break;
				}
			}

			font_name = (r && r->font_name) ? r->font_name : (p->font_name ? p->font_name : "Courier");

			if (hl->font_size && hl->font_size->result[o->residx] && hl->font_size->result[o->residx]->type == OCRPT_RESULT_NUMBER && hl->font_size->result[o->residx]->number_initialized)
				font_size = mpfr_get_d(hl->font_size->result[o->residx]->number, o->rndmode);
			else
				font_size = (r ? r->font_size : p->font_size);

			ocrpt_layout_set_font_sizes(o, output, font_name, font_size, false, false, NULL, &hl->font_width);
			break;
		case OCRPT_OUTPUT_IMAGE:
			ocrpt_image *img = (ocrpt_image *)oe;

			output->current_image = NULL;
			img->suppress_image = false;
			if (img->suppress && img->suppress->result[o->residx] && img->suppress->result[o->residx]->type == OCRPT_RESULT_NUMBER && img->suppress->result[o->residx]->number_initialized) {
				long suppress = mpfr_get_si(img->suppress->result[o->residx]->number, o->rndmode);

				if (suppress) {
					img->suppress_image = true;
					break;
				}
			}

			ocrpt_layout_image_setup(o, p, pr, pd, r, output, img, page_width, page_indent, page_position);
			break;
		case OCRPT_OUTPUT_IMAGEEND:
			break;
		}
	}
}

static inline void get_height_exceeded(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, double old_page_position, double new_page_position, bool *height_exceeded, bool *pd_height_exceeded, bool *r_height_exceeded) {
	*height_exceeded = (new_page_position + ((pd && pd->border_width_set) ? pd->border_width : 0.0)) > p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
	*pd_height_exceeded = pd && pd->height_set && (new_page_position > (pd->start_page_position + pd->remaining_height));
	*r_height_exceeded = r && r->height_set && (r->remaining_height < (new_page_position - old_page_position));
}

bool ocrpt_layout_output_internal(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, double page_width, double page_indent, double *page_position) {
	if (output->suppress_output)
		return false;

	for (; output->iter; output->iter = output->iter->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)output->iter->data;

		switch (oe->type) {
		case OCRPT_OUTPUT_LINE:
			ocrpt_line *l = (ocrpt_line *)oe;

			if (l->suppress_line)
				break;

			for (; l->current_line < l->maxlines; l->current_line++) {
				if (output->has_memo) {
					bool height_exceeded, pd_height_exceeded, r_height_exceeded;

					get_height_exceeded(o, p, pr, pd, r, *page_position, *page_position + l->line_height, &height_exceeded, &pd_height_exceeded, &r_height_exceeded);

					output->height_exceeded = height_exceeded;
					output->pd_height_exceeded = pd_height_exceeded;
					output->r_height_exceeded = r_height_exceeded;

					if (height_exceeded || pd_height_exceeded || r_height_exceeded) {
						return true;
					}
				}

				if (output->current_image)
					ocrpt_layout_line(draw, o, p, pr, pd, r, output, l, page_width - output->current_image->image_width, page_indent + output->current_image->image_width, page_position);
				else
					ocrpt_layout_line(draw, o, p, pr, pd, r, output, l, page_width, page_indent, page_position);
			}

			l->current_line = 0;
			break;
		case OCRPT_OUTPUT_HLINE:
			ocrpt_hline *hl = (ocrpt_hline *)oe;

			if (hl->suppress_hline)
				break;

			if (output->current_image)
				ocrpt_layout_hline(draw, o, p, pr, pd, r, output, hl, page_width - output->current_image->image_width, page_indent + output->current_image->image_width, page_position);
			else
				ocrpt_layout_hline(draw, o, p, pr, pd, r, output, hl, page_width, page_indent, page_position);
			break;
		case OCRPT_OUTPUT_IMAGE:
			ocrpt_image *img = (ocrpt_image *)oe;

			if (output->current_image)
				*page_position = output->old_page_position + output->current_image->image_height;

			output->current_image = NULL;
			if (img->suppress_image)
				break;

			ocrpt_layout_image(draw, o, p, pr, pd, r, output, img, page_width, page_indent, page_position);
			break;
		case OCRPT_OUTPUT_IMAGEEND:
			if (output->current_image)
				*page_position = output->old_page_position + output->current_image->image_height;

			output->current_image = NULL;
			break;
		}
	}

	if (output->current_image)
		*page_position = output->old_page_position + output->current_image->image_height;

	output->current_image = NULL;

	return false;
}

void ocrpt_layout_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	ocrpt_list *old_current_page = NULL;
	double new_page_position;
	bool page_break = false;

	if (*newpage) {
		if (o->current_page) {
			ocrpt_layout_output_evaluate(o, p, NULL, &p->pageheader);
			ocrpt_layout_output_evaluate(o, p, NULL, &p->pagefooter);

			if (!p->suppress_pageheader_firstpage || (p->suppress_pageheader_firstpage && o->current_page != o->pages)) {
				double top_page_position = ocrpt_layout_top_margin(o, p);
				ocrpt_layout_output_init(&p->pageheader);
				ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &top_page_position);
				ocrpt_layout_output_internal(!o->precalculate, o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &top_page_position);
			}

			double bottom_page_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
			ocrpt_layout_output_init(&p->pagefooter);
			ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &bottom_page_position);
			ocrpt_layout_output_internal(!o->precalculate, o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &bottom_page_position);
		}

		/* Set newpage to false first to prevent infinite recursion. */
		*newpage = false;
		ocrpt_layout_add_new_page(o, p, pr, pd, r, rows, newpage, page_indent, page_position, old_page_position);

		if (!pr->start_page && !r->current_iteration) {
			pr->start_page = o->current_page;
			pr->start_page_position = *page_position;
			pr->end_page_position = *page_position;
			pd->start_page_position = *page_position;
		}
	}

	restart:

	if ((pd && pd->finished) || (r && r->finished))
		return;

	/*
	 * The bool value in o->precalculate means !draw in layout context.
	 * Since the complete <Output> section is (usually) drawn as an atomic unit
	 * ("memo" values break this!!!) we need to calculate the last
	 * page position without drawing first. So, draw in two rounds.
	 *
	 * 1. In the first round, only detect the last page position.
	 *    If the vertical position exceeds the bottom margin (minus the
	 *    page footer height) go to the next page.
	 * 2. In the second round, Set o->precalculate to what it was and if it's false then draw
	 *    the section.
	 */
	old_current_page = o->current_page;
	*old_page_position = new_page_position = *page_position;

	ocrpt_layout_output_position_push(output);
	ocrpt_layout_output_internal_preamble(o, p, pr, pd, r, output, pd->column_width, *page_indent, &new_page_position);
	bool memo_break = ocrpt_layout_output_internal(false, o, p, pr, pd, r, output, pd->column_width, *page_indent, &new_page_position);

	bool height_exceeded, pd_height_exceeded, r_height_exceeded;

	if (memo_break) {
		/*
		 * Memo break detected. Rewind to the previous page position and
		 * draw before handling column or page break.
		 */
		new_page_position = *old_page_position;

		ocrpt_layout_output_position_pop(output);
		ocrpt_layout_output_internal_preamble(o, p, pr, pd, r, output, pd->column_width, *page_indent, &new_page_position);
		memo_break = ocrpt_layout_output_internal(!o->precalculate, o, p, pr, pd, r, output, pd->column_width, *page_indent, &new_page_position);

		if (pd->max_page_position < new_page_position)
			pd->max_page_position = new_page_position;

		height_exceeded = output->height_exceeded;
		pd_height_exceeded = output->pd_height_exceeded;
		r_height_exceeded = output->r_height_exceeded;
	} else
		get_height_exceeded(o, p, pr, pd, r, *old_page_position, new_page_position, &height_exceeded, &pd_height_exceeded, &r_height_exceeded);

	if (r_height_exceeded)
		r->finished = true;

	if (height_exceeded || pd_height_exceeded) {
		if (!memo_break)
			pd->max_page_position = *old_page_position;
		pd->current_column++;

		if (pd->current_column >= pd->detail_columns) {
			if (pd_height_exceeded)
				pd->finished = true;
			if (height_exceeded) {
				page_break = true;
			}
		} else
			page_break = false;

		if (page_break) {
			if (pd->height_set)
				pd->remaining_height -= new_page_position - pd->start_page_position;

			pd->current_column = 0;
			*page_indent = pd->page_indent;
			if (pd->border_width_set)
				*page_indent += pd->border_width;

			double top_page_position = ocrpt_layout_top_margin(o, p);
			if (!p->suppress_pageheader_firstpage || (p->suppress_pageheader_firstpage && o->current_page != o->pages)) {
				ocrpt_layout_output_evaluate(o, p, NULL, &p->pageheader);
				ocrpt_layout_output_init(&p->pageheader);
				ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &top_page_position);
				ocrpt_layout_output_internal(!o->precalculate, o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &top_page_position);
			}

			if (!o->precalculate && pd && pd->border_width_set && o->output_functions.draw_rectangle)
				o->output_functions.draw_rectangle(o, p, pr, pd, r, output,
													&pd->border_color, pd->border_width,
													pd->page_indent0 + 0.5 * pd->border_width,
													pd->start_page_position + 0.5 * pd->border_width,
													pd->real_width - pd->border_width,
													pd->max_page_position - pd->start_page_position);

			double bottom_page_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
			ocrpt_layout_output_evaluate(o, p, NULL, &p->pagefooter);
			ocrpt_layout_output_init(&p->pagefooter);
			ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &bottom_page_position);
			ocrpt_layout_output_internal(!o->precalculate, o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &bottom_page_position);

			/* Set newpage to false first to prevent infinite recursion. */
			*newpage = false;
			ocrpt_layout_add_new_page(o, p, pr, pd, r, rows, newpage, page_indent, &new_page_position, old_page_position);

			pd->max_page_position = 0.0;
		} else if (!pd->finished) {
			*page_indent += pd->column_width + (o->size_in_points ? 1.0 : 72.0) * pd->column_pad;
			new_page_position = pd->start_page_position;
			if (pd->border_width_set)
				new_page_position += pd->border_width;
			ocrpt_layout_output_highprio_fieldheader(o, p, pr, pd, r, rows, newpage, page_indent, &new_page_position, old_page_position);
		}

		if (pd->finished)
			return;

		page_break = true;
	}

	if (!page_break && !memo_break) {
		o->current_page = old_current_page;
		new_page_position = *old_page_position;
	}

	if (!r || (r && !r->finished /* && pd && !pd->finished */)) {
		if (!output->has_memo || !memo_break)
			ocrpt_layout_output_init(output);
		ocrpt_layout_output_internal_preamble(o, p, pr, pd, r, output, pd->column_width, *page_indent, &new_page_position);
		memo_break = ocrpt_layout_output_internal(!o->precalculate, o, p, pr, pd, r, output, pd->column_width, *page_indent, &new_page_position);
	}

	if (r && r->height_set)
		r->remaining_height -= new_page_position - *old_page_position;

	*page_position = new_page_position;
	if (pd->max_page_position < new_page_position)
		pd->max_page_position = new_page_position;

	if (memo_break)
		goto restart;
}

double ocrpt_layout_top_margin(opencreport *o, ocrpt_part *p) {
	double top_margin;

	if (p->top_margin_set)
		top_margin = p->top_margin;
	else
		top_margin = OCRPT_DEFAULT_TOP_MARGIN;

	if (o->size_in_points)
		return top_margin;
	else
		return top_margin * 72.0;
}

double ocrpt_layout_bottom_margin(opencreport *o, ocrpt_part *p) {
	double bottom_margin;

	if (p->bottom_margin_set)
		bottom_margin = p->bottom_margin;
	else
		bottom_margin = OCRPT_DEFAULT_BOTTOM_MARGIN;

	if (o->size_in_points)
		return bottom_margin;
	else
		return bottom_margin * 72.0;
}

double ocrpt_layout_left_margin(opencreport *o, ocrpt_part *p) {
	double left_margin;

	if (p->left_margin_set)
		left_margin = p->left_margin;
	else
		left_margin = OCRPT_DEFAULT_LEFT_MARGIN;

	if (o->size_in_points)
		return left_margin;
	else
		return left_margin * 72.0;
}

double ocrpt_layout_right_margin(opencreport *o, ocrpt_part *p) {
	double right_margin;

	if (p->right_margin_set)
		right_margin = p->right_margin;
	else
		right_margin = OCRPT_DEFAULT_RIGHT_MARGIN;

	if (o->size_in_points)
		return right_margin;
	else
		return right_margin * 72.0;
}

void ocrpt_layout_output_highprio_fieldheader(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	if (r && r->fieldheader.output_list && r->fieldheader_high_priority) {
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
		if (rows > 1) {
			ocrpt_expr_init_iterative_results(o, r->detailcnt, OCRPT_RESULT_NUMBER);
			ocrpt_expr_eval(o, r, r->detailcnt);
			ocrpt_report_evaluate_detailcnt_dependees(o, r);
			ocrpt_layout_output_init(&r->fieldheader);
			ocrpt_layout_output(o, p, pr, pd, r, &r->fieldheader, rows, newpage, page_indent, page_position, old_page_position);
		}
	}
}

void ocrpt_layout_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
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

	*page_position = ocrpt_layout_top_margin(o, p);
	if (!p->suppress_pageheader_firstpage || (p->suppress_pageheader_firstpage && o->current_page != o->pages))
		*page_position += p->page_header_height;

	if (rows == 1 && !pr->start_page) {
		if (r->current_iteration == 0) {
			pr->start_page = o->current_page;
			pr->start_page_position = *page_position;
			pd->start_page_position = *page_position;
		}
		if (pd->border_width_set)
			*page_position += pd->border_width;
		ocrpt_layout_output_init(&r->reportheader);
		ocrpt_layout_output(o, p, pr, pd, r, &r->reportheader, rows, newpage, page_indent, page_position, old_page_position);
	} else {
		pd->start_page_position = *page_position;
		if (pd->border_width_set)
			*page_position += pd->border_width;
	}
	ocrpt_layout_output_highprio_fieldheader(o, p, pr, pd, r, rows, newpage, page_indent, page_position, old_page_position);
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

void ocrpt_layout_set_font_sizes(opencreport *o, ocrpt_output *output, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width) {
	ocrpt_cairo_create(o);

	PangoLayout *layout;
	PangoFontDescription *font_description;

	font_description = pango_font_description_new();

	pango_font_description_set_family(font_description, font);
	pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_font_description_set_style(font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_absolute_size(font_description, wanted_font_size * PANGO_SCALE);

	layout = pango_cairo_create_layout(o->cr);
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
}
