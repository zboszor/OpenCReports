/*
 * Formatting utilities
 *
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <langinfo.h>
#include <libintl.h>
#include <cairo.h>
#include <cairo-svg.h>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "exprutil.h"
#include "formatting.h"
#include "variables.h"
#include "datasource.h"
#include "parts.h"
#include "layout.h"
#include "barcode.h"

/*
 * In precalculate mode no output is produced.
 * That run is to compute page elements' locations
 * and the final number of pages.
 */

static void ocrpt_layout_image_setup(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, ocrpt_image *image, double page_width, double page_indent, double *page_position);

void ocrpt_layout_compute_text(opencreport *o, ocrpt_text *le) {
	bool has_translate = false;
	bool has_format = false;
	bool has_value = false;

	if (o->textdomain && EXPR_VALID_NUMERIC(le->translate)) {
		has_translate = !!mpfr_get_si(EXPR_NUMERIC(le->translate), EXPR_RNDMODE(le->translate));
	}

	if (EXPR_VALID_STRING(le->format))
		has_format = true;

	if (EXPR_VALID_NOT_NULL(le->value) && (EXPR_VALID_STRING(le->value) || EXPR_VALID_NUMERIC(le->value) || EXPR_VALID_DATETIME(le->value)))
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

	if (has_format) {
		if (has_translate) {
			locale_t locale = uselocale(o->locale);
			ocrpt_mem_string_append_printf(fstring, "%s", dgettext(o->textdomain, EXPR_STRING_VAL(le->format)));
			uselocale(locale);
		} else
			ocrpt_mem_string_append_printf(fstring, "%s", EXPR_STRING_VAL(le->format));
		ocrpt_format_string(o, NULL, rstring, fstring, &le->value, 1);
	} else if (has_value) {
		switch (EXPR_TYPE(le->value)) {
		case OCRPT_RESULT_STRING:
		case OCRPT_RESULT_ERROR:
			ocrpt_mem_string_append_printf(fstring, "%s", "%s");
			if (has_translate) {
				locale_t locale = uselocale(o->locale);
				ocrpt_mem_string_append_printf(string, "%s", dgettext(o->textdomain, EXPR_STRING_VAL(le->value)));
				uselocale(locale);
			} else
				ocrpt_mem_string_append_printf(string, "%s", EXPR_STRING_VAL(le->value));
			ocrpt_format_string_literal(o, NULL, rstring, fstring, string);
			break;
		case OCRPT_RESULT_NUMBER:
			ocrpt_mem_string_append_printf(fstring, "%s", "%d");
			ocrpt_format_string(o, NULL, rstring, fstring, &le->value, 1);
			break;
		case OCRPT_RESULT_DATETIME:
			ocrpt_mem_string_append_printf(fstring, "%s", nl_langinfo_l(D_FMT, o->locale));
			ocrpt_format_string(o, NULL, rstring, fstring, &le->value, 1);
			break;
		}
	}

	assert(rstring);
}

static void ocrpt_layout_line_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, double page_width, double *page_position) {
	double next_start;
	double max_barcode_height;

	line->ascent = 0.0;
	line->descent = 0.0;
	line->maxlines = 1;
	next_start = 0.0;
	max_barcode_height = 0.0;

	for (ocrpt_list *l = line->elements; l; l = l->next) {
		ocrpt_text *elem = (ocrpt_text *)l->data;
		ocrpt_image *img = (ocrpt_image *)elem;
		ocrpt_barcode *bc = (ocrpt_barcode *)elem;

		switch (elem->base.le_type) {
		case OCRPT_OUTPUT_LE_UNSET:
			assert(0);
			break;
		case OCRPT_OUTPUT_LE_TEXT:
			ocrpt_layout_compute_text(o, elem);

			elem->start = next_start;

			if (o->output_functions.get_text_sizes)
				o->output_functions.get_text_sizes(o, p, pr, pd, r, output, line, elem, l->next == NULL,  page_width - ((pd && pd->border_width_expr) ? 2 * pd->border_width: 0.0));

			next_start += elem->width_computed;

			if (line->ascent < elem->ascent)
				line->ascent = elem->ascent;
			if (line->descent < elem->descent)
				line->descent = elem->descent;

			if (line->maxlines < elem->lines)
				line->maxlines = elem->lines;

			output->has_memo = output->has_memo || (elem->memo && (elem->lines > 1));
			break;
		case OCRPT_OUTPUT_LE_IMAGE:
			img->start = next_start;

			ocrpt_layout_image_setup(o, p, pr, pd, r, output, line, img, page_width, img->start, page_position);

			if (EXPR_VALID_NUMERIC(img->text_width))
				img->image_text_width = mpfr_get_d(EXPR_NUMERIC(img->text_width), EXPR_RNDMODE(img->text_width));
			else if (EXPR_VALID_NUMERIC(img->width))
				img->image_text_width = mpfr_get_d(EXPR_NUMERIC(img->width), EXPR_RNDMODE(img->width));
			else
				img->image_text_width = 0.0;

			if (!o->size_in_points)
				img->image_text_width *= line->font_width;

			next_start += img->image_text_width;

			break;
		case OCRPT_OUTPUT_LE_BARCODE:
			bc->start = next_start;

			if (EXPR_VALID_NUMERIC(bc->height))
				bc->barcode_height = mpfr_get_d(EXPR_NUMERIC(bc->height), EXPR_RNDMODE(bc->height));
			else
				bc->barcode_height = 0.0; /* it will be set in the loop below when the line height is known */

			bool encoding_ok = ocrpt_barcode_encode(o, bc);

			if (EXPR_VALID_NUMERIC(bc->width)) {
				bc->barcode_width = mpfr_get_d(EXPR_NUMERIC(bc->width), EXPR_RNDMODE(bc->width));
				if (!o->size_in_points)
					bc->barcode_width *= line->font_width;
			} else
				bc->barcode_width = encoding_ok ? bc->encoded_width : 0.0;

			next_start += bc->barcode_width;

			break;
		}
	}

	line->line_height = line->ascent + line->descent;

	for (ocrpt_list *l = line->elements; l; l = l->next) {
		ocrpt_barcode *bc = (ocrpt_barcode *)l->data;

		if (bc->base.le_type == OCRPT_OUTPUT_LE_BARCODE) {
			if (bc->barcode_height == 0.0)
				bc->barcode_height = line->line_height;

			if (max_barcode_height < bc->barcode_height)
				max_barcode_height = bc->barcode_height;
		}
	}

	for (ocrpt_list *l = line->elements; l; l = l->next) {
		ocrpt_barcode *bc = (ocrpt_barcode *)l->data;

		if (bc->base.le_type == OCRPT_OUTPUT_LE_BARCODE) {
			if (bc->barcode_height < max_barcode_height)
				bc->vertical_gap = (max_barcode_height - bc->barcode_height) / 2.0;
		}
	}

	if (max_barcode_height > line->line_height) {
		double bchdiff = (max_barcode_height - line->line_height);
		double bchdiff2 = bchdiff / 2.0;

		line->line_height += bchdiff;
		line->ascent += bchdiff2;
		line->descent += bchdiff2;
	}
}

static void ocrpt_layout_line(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, double page_width, double page_indent, double *page_position) {
	if (draw) {
		if (o->output_functions.start_data_row)
			o->output_functions.start_data_row(o, p, pr, pd, r, br, output, line, page_indent, *page_position);

		for (ocrpt_list *l = line->elements; l; l = l->next) {
			ocrpt_text *elem = (ocrpt_text *)l->data;
			ocrpt_image *img = (ocrpt_image *)elem;
			ocrpt_barcode *bc = (ocrpt_barcode *)elem;

			switch (elem->base.le_type) {
			case OCRPT_OUTPUT_LE_UNSET:
				assert(0);
				break;
			case OCRPT_OUTPUT_LE_TEXT:
				if ((!o->output_functions.support_any_font || (elem->start < page_width)) && o->output_functions.draw_text)
					o->output_functions.draw_text(o, p, pr, pd, r, br, output, line, elem, (l->next == NULL), page_width, page_indent, *page_position);
				break;
			case OCRPT_OUTPUT_LE_IMAGE:
				if ((!o->output_functions.support_any_font || (img->start < page_width)) && o->output_functions.draw_image)
					o->output_functions.draw_image(o, p, pr, pd, r, br, output, line, img, (l->next == NULL), page_width, page_indent, page_indent + img->start, *page_position, img->image_text_width, line->line_height);
				break;
			case OCRPT_OUTPUT_LE_BARCODE:
				if (img->start < page_width && o->output_functions.draw_barcode)
					o->output_functions.draw_barcode(o, p, pr, pd, r, br, output, line, bc, (l->next == NULL), page_width, page_indent, page_indent + img->start, *page_position, bc->barcode_width, line->line_height);
				break;
			}
		}

		if (o->output_functions.end_data_row)
			o->output_functions.end_data_row(o, p, pr, pd, r, br, output, line);
	}

	if (line->fontsz > 0.0 && !line->elements)
		*page_position += line->fontsz;
	else
		*page_position += line->ascent + line->descent;
}

static void ocrpt_layout_hline(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_hline *hline, double page_width, double page_indent, double *page_position) {
	if (EXPR_VALID_NUMERIC(hline->suppress)) {
		long suppress = mpfr_get_si(EXPR_NUMERIC(hline->suppress), EXPR_RNDMODE(hline->suppress));

		if (suppress)
			return;
	}

	double size;

	if (EXPR_VALID_NUMERIC(hline->size))
		size = mpfr_get_d(EXPR_NUMERIC(hline->size), EXPR_RNDMODE(hline->size));
	else
		size = 1.0;

	if (draw && o->output_functions.draw_hline)
		o->output_functions.draw_hline(o, p, pr, pd, r, br, output, hline, page_width, page_indent, *page_position, size);

	*page_position += size;
}

static bool ocrpt_load_svg(ocrpt_image_file *img) {
	img->rsvg = rsvg_handle_new_from_file(img->name, NULL);
	if (!img->rsvg)
		return false;

#if LIBRSVG_CHECK_VERSION(2,52,0)
	rsvg_handle_get_intrinsic_size_in_pixels(img->rsvg, &img->width, &img->height);
#else
	RsvgDimensionData ddata;
	rsvg_handle_get_dimensions(img->rsvg, &ddata);
	img->width = ddata.width;
	img->height = ddata.height;
#endif

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

static void ocrpt_layout_image_setup(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, ocrpt_image *image, double page_width, double page_indent, double *page_position) {
	image->suppress_image = false;

	/* Don't render the image if the filename, the width or the height are not set. */
	if (!EXPR_VALID_STRING(image->value)) {
		image->suppress_image = true;
		return;
	}

	if (!EXPR_VALID_NUMERIC(image->width)) {
		if (!image->in_line) {
			image->suppress_image = true;
			return;
		}
		image->image_width = 0.0;
	} else
		image->image_width = mpfr_get_d(EXPR_NUMERIC(image->width), EXPR_RNDMODE(image->width));

	if (!EXPR_VALID_NUMERIC(image->height)) {
		if (!image->in_line) {
			image->suppress_image = true;
			return;
		}
		image->image_height = 0.0;
	} else
		image->image_height = mpfr_get_d(EXPR_NUMERIC(image->height), EXPR_RNDMODE(image->height));

	if (EXPR_VALID_STRING(image->bgcolor))
		ocrpt_get_color(EXPR_STRING_VAL(image->bgcolor), &image->bg, true);
	else if (line && EXPR_VALID_STRING(line->bgcolor))
		ocrpt_get_color(EXPR_STRING_VAL(line->bgcolor), &image->bg, true);
	else
		ocrpt_get_color(NULL, &image->bg, true);

	char *img_filename = ocrpt_find_file(o, EXPR_STRING_VAL(image->value));

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

	if (imgf)
		image->img_file = imgf;
	else {
		imgf = ocrpt_mem_malloc(sizeof(ocrpt_image_file));
		memset(imgf, 0, sizeof(ocrpt_image_file));
		imgf->name = img_filename;

		bool svg = false;
		bool valid = false;

		if (EXPR_VALID_STRING(image->imgtype)) {
			svg = strcasecmp(EXPR_STRING_VAL(image->imgtype), "svg") == 0;
		} else {
			ocrpt_string *s = EXPR_STRING(image->value);
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

static void ocrpt_layout_image(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_image *image, bool last, double page_width, double page_indent, double *page_position) {
	if (draw && o->output_functions.draw_image && image->img_file)
		o->output_functions.draw_image(o, p, pr, pd, r, br, output, NULL, image, last, page_width, page_indent, page_indent, *page_position, image->image_width, image->image_height);

	output->old_page_position = *page_position;
	output->current_image = image;
}

static void ocrpt_layout_barcode_setup(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, ocrpt_barcode *bc, double page_width, double page_indent, double *page_position) {
	bc->suppress_bc = false;

	/* Don't render the barcode if the value, the width or the height are not set. */
	if (!EXPR_VALID_STRING(bc->value)) {
		bc->suppress_bc = true;
		return;
	}

	if (!EXPR_VALID_NUMERIC(bc->width)) {
		if (!bc->in_line) {
			bc->suppress_bc = true;
			return;
		}
		bc->barcode_width = 0.0;
	} else
		bc->barcode_width = mpfr_get_d(EXPR_NUMERIC(bc->width), EXPR_RNDMODE(bc->width));

	if (!EXPR_VALID_NUMERIC(bc->height)) {
		if (!bc->in_line) {
			bc->suppress_bc = true;
			return;
		}
		bc->barcode_height = 0.0;
	} else
		bc->barcode_height = mpfr_get_d(EXPR_NUMERIC(bc->height), EXPR_RNDMODE(bc->height));
}

static void ocrpt_layout_barcode(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_barcode *bc, bool last, double page_width, double page_indent, double *page_position) {
	if (draw && o->output_functions.draw_barcode) {
		ocrpt_barcode_encode(o, bc);
		o->output_functions.draw_barcode(o, p, pr, pd, r, br, output, NULL, bc, last, page_width, page_indent, page_indent, *page_position, bc->barcode_width, bc->barcode_height);
	}

	output->old_page_position = *page_position;
	output->current_barcode = bc;
}

static void ocrpt_layout_imageend(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output) {
	if (draw && o->output_functions.draw_imageend)
		o->output_functions.draw_imageend(o, p, pr, pd, r, br, output);
}

void ocrpt_layout_output_evaluate_expr_params(ocrpt_output *output) {
	for (ocrpt_list *ol = output->output_list; ol; ol = ol->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)ol->data;
		ocrpt_barcode *lbc = (ocrpt_barcode *)ol->data;

		switch (oe->type) {
		case OCRPT_OUTPUT_UNSET:
			assert(0);
			break;
		case OCRPT_OUTPUT_LINE: {
			ocrpt_line *line = (ocrpt_line *)oe;

			for (ocrpt_list *l = line->elements; l; l = l->next) {
				ocrpt_text *elem = (ocrpt_text *)l->data;
				ocrpt_barcode *bc = (ocrpt_barcode *)l->data;

				switch (elem->base.le_type) {
				case OCRPT_OUTPUT_LE_UNSET:
					assert(0);
					break;
				case OCRPT_OUTPUT_LE_TEXT:
					ocrpt_expr_resolve(elem->delayed);
					ocrpt_expr_optimize(elem->delayed);
					if (elem->delayed)
						ocrpt_expr_set_delayed(elem->value, !!ocrpt_expr_get_long(elem->delayed));

					ocrpt_expr_resolve(elem->memo_expr);
					ocrpt_expr_optimize(elem->memo_expr);

					if (elem->memo_expr) {
						elem->memo = !!ocrpt_expr_get_long(elem->memo_expr);

						if (elem->memo) {
							ocrpt_expr_resolve(elem->wrap_chars);
							ocrpt_expr_optimize(elem->wrap_chars);
							if (elem->wrap_chars)
								elem->memo_wrap_chars = !!ocrpt_expr_get_long(elem->wrap_chars);

							ocrpt_expr_resolve(elem->max_lines);
							ocrpt_expr_optimize(elem->max_lines);
							if (elem->max_lines)
								elem->memo_max_lines = ocrpt_expr_get_long(elem->max_lines);
						}
					}

					break;
				case OCRPT_OUTPUT_LE_IMAGE:
					break;
				case OCRPT_OUTPUT_LE_BARCODE:
					ocrpt_expr_resolve(bc->delayed);
					ocrpt_expr_optimize(bc->delayed);
					if (bc->delayed)
						ocrpt_expr_set_delayed(bc->value, !!ocrpt_expr_get_long(bc->delayed));
					break;
				}
			}

			break;
		}
		case OCRPT_OUTPUT_HLINE:
		case OCRPT_OUTPUT_IMAGE:
		case OCRPT_OUTPUT_IMAGEEND:
			break;
		case OCRPT_OUTPUT_BARCODE:
			ocrpt_expr_resolve(lbc->delayed);
			ocrpt_expr_optimize(lbc->delayed);
			if (lbc->delayed)
				ocrpt_expr_set_delayed(lbc->value, !!ocrpt_expr_get_long(lbc->delayed));
			break;
		}
	}
}

static void ocrpt_layout_output_resolve_line(ocrpt_line *line) {
	ocrpt_expr_resolve(line->font_name);
	ocrpt_expr_optimize(line->font_name);

	ocrpt_expr_resolve(line->font_size);
	ocrpt_expr_optimize(line->font_size);

	ocrpt_expr_resolve(line->color);
	ocrpt_expr_optimize(line->color);

	ocrpt_expr_resolve(line->bgcolor);
	ocrpt_expr_optimize(line->bgcolor);

	ocrpt_expr_resolve(line->bold);
	ocrpt_expr_optimize(line->bold);

	ocrpt_expr_resolve(line->italic);
	ocrpt_expr_optimize(line->italic);

	ocrpt_expr_resolve(line->suppress);
	ocrpt_expr_optimize(line->suppress);
}

static void ocrpt_layout_output_resolve_hline(ocrpt_hline *hline) {
	ocrpt_expr_resolve(hline->size);
	ocrpt_expr_optimize(hline->size);

	ocrpt_expr_resolve(hline->indent);
	ocrpt_expr_optimize(hline->indent);

	ocrpt_expr_resolve(hline->length);
	ocrpt_expr_optimize(hline->length);

	ocrpt_expr_resolve(hline->font_size);
	ocrpt_expr_optimize(hline->font_size);

	ocrpt_expr_resolve(hline->suppress);
	ocrpt_expr_optimize(hline->suppress);

	ocrpt_expr_resolve(hline->color);
	ocrpt_expr_optimize(hline->color);
}

static void ocrpt_layout_output_resolve_text(ocrpt_text *elem) {
	ocrpt_expr_resolve(elem->value);
	ocrpt_expr_optimize(elem->value);

	ocrpt_expr_resolve(elem->format);
	ocrpt_expr_optimize(elem->format);

	ocrpt_expr_resolve(elem->width);
	ocrpt_expr_optimize(elem->width);

	ocrpt_expr_resolve(elem->align);
	ocrpt_expr_optimize(elem->align);

	ocrpt_expr_resolve(elem->color);
	ocrpt_expr_optimize(elem->color);

	ocrpt_expr_resolve(elem->bgcolor);
	ocrpt_expr_optimize(elem->bgcolor);

	ocrpt_expr_resolve(elem->font_name);
	ocrpt_expr_optimize(elem->font_name);

	ocrpt_expr_resolve(elem->font_size);
	ocrpt_expr_optimize(elem->font_size);

	ocrpt_expr_resolve(elem->bold);
	ocrpt_expr_optimize(elem->bold);

	ocrpt_expr_resolve(elem->italic);
	ocrpt_expr_optimize(elem->italic);

	ocrpt_expr_resolve(elem->link);
	ocrpt_expr_optimize(elem->link);

	ocrpt_expr_resolve(elem->translate);
	ocrpt_expr_optimize(elem->translate);
}

static void ocrpt_layout_output_resolve_image(ocrpt_image *img) {
	ocrpt_expr_resolve(img->suppress);
	ocrpt_expr_optimize(img->suppress);

	ocrpt_expr_resolve(img->value);
	ocrpt_expr_optimize(img->value);

	ocrpt_expr_resolve(img->imgtype);
	ocrpt_expr_optimize(img->imgtype);

	ocrpt_expr_resolve(img->width);
	ocrpt_expr_optimize(img->width);

	ocrpt_expr_resolve(img->height);
	ocrpt_expr_optimize(img->height);

	ocrpt_expr_resolve(img->align);
	ocrpt_expr_optimize(img->align);

	ocrpt_expr_resolve(img->bgcolor);
	ocrpt_expr_optimize(img->bgcolor);

	ocrpt_expr_resolve(img->text_width);
	ocrpt_expr_optimize(img->text_width);
}

static void ocrpt_layout_output_resolve_barcode(ocrpt_barcode *bc) {
	ocrpt_expr_resolve(bc->value);
	ocrpt_expr_optimize(bc->value);

	ocrpt_expr_resolve(bc->delayed);
	ocrpt_expr_optimize(bc->delayed);
	if (bc->delayed)
		ocrpt_expr_set_delayed(bc->value, !!ocrpt_expr_get_long(bc->delayed));

	ocrpt_expr_resolve(bc->bctype);
	ocrpt_expr_optimize(bc->bctype);

	ocrpt_expr_resolve(bc->width);
	ocrpt_expr_optimize(bc->width);

	ocrpt_expr_resolve(bc->height);
	ocrpt_expr_optimize(bc->height);

	ocrpt_expr_resolve(bc->color);
	ocrpt_expr_optimize(bc->color);

	ocrpt_expr_resolve(bc->bgcolor);
	ocrpt_expr_optimize(bc->bgcolor);
}

void ocrpt_layout_output_resolve(ocrpt_output *output) {
	ocrpt_expr_resolve(output->suppress);
	ocrpt_expr_optimize(output->suppress);

	for (ocrpt_list *ol = output->output_list; ol; ol = ol->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)ol->data;

		switch (oe->type) {
		case OCRPT_OUTPUT_UNSET:
			assert(0);
			break;
		case OCRPT_OUTPUT_LINE:
			ocrpt_layout_output_resolve_line((ocrpt_line *)oe);

			for (ocrpt_list *l = ((ocrpt_line *)oe)->elements; l; l = l->next) {
				ocrpt_output_element *le = (ocrpt_output_element *)l->data;

				switch (le->le_type) {
				case OCRPT_OUTPUT_LE_UNSET:
					assert(0);
					break;
				case OCRPT_OUTPUT_LE_TEXT:
					ocrpt_layout_output_resolve_text((ocrpt_text *)le);
					break;
				case OCRPT_OUTPUT_LE_IMAGE:
					ocrpt_layout_output_resolve_image((ocrpt_image *)le);
					break;
				case OCRPT_OUTPUT_LE_BARCODE:
					ocrpt_layout_output_resolve_barcode((ocrpt_barcode *)le);
					break;
				}
			}

			break;
		case OCRPT_OUTPUT_HLINE:
			ocrpt_layout_output_resolve_hline((ocrpt_hline *)oe);
			break;
		case OCRPT_OUTPUT_IMAGE:
			ocrpt_layout_output_resolve_image((ocrpt_image *)oe);
			break;
		case OCRPT_OUTPUT_IMAGEEND:
			/* nothing to do */
			break;
		case OCRPT_OUTPUT_BARCODE:
			ocrpt_layout_output_resolve_barcode((ocrpt_barcode *)oe);
			break;
		}
	}
}

static void ocrpt_layout_output_evaluate_hline(ocrpt_hline *hline) {
	ocrpt_expr_eval(hline->size);
	ocrpt_expr_eval(hline->indent);
	ocrpt_expr_eval(hline->length);
	ocrpt_expr_eval(hline->font_size);
	ocrpt_expr_eval(hline->suppress);
	ocrpt_expr_eval(hline->color);
}

static void ocrpt_layout_output_evaluate_image(ocrpt_image *img) {
	ocrpt_expr_eval(img->suppress);
	ocrpt_expr_eval(img->value);
	ocrpt_expr_eval(img->imgtype);
	ocrpt_expr_eval(img->width);
	ocrpt_expr_eval(img->height);
	ocrpt_expr_eval(img->align);
	ocrpt_expr_eval(img->bgcolor);
	ocrpt_expr_eval(img->text_width);
}

static void ocrpt_layout_output_evaluate_barcode(ocrpt_barcode *bc) {
	ocrpt_expr_eval(bc->value);
	ocrpt_expr_eval(bc->bctype);
	ocrpt_expr_eval(bc->width);
	ocrpt_expr_eval(bc->height);
	ocrpt_expr_eval(bc->color);
	ocrpt_expr_eval(bc->bgcolor);
}

void ocrpt_layout_output_evaluate(ocrpt_output *output) {
	for (ocrpt_list *ol = output->output_list; ol; ol = ol->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)ol->data;

		switch (oe->type) {
		case OCRPT_OUTPUT_UNSET:
			assert(0);
			break;
		case OCRPT_OUTPUT_LINE: {
			ocrpt_line *line = (ocrpt_line *)oe;

			ocrpt_expr_eval(line->font_name);
			ocrpt_expr_eval(line->font_size);
			ocrpt_expr_eval(line->color);
			ocrpt_expr_eval(line->bgcolor);
			ocrpt_expr_eval(line->bold);
			ocrpt_expr_eval(line->italic);
			ocrpt_expr_eval(line->suppress);

			for (ocrpt_list *l = line->elements; l; l = l->next) {
				ocrpt_text *elem = (ocrpt_text *)l->data;

				switch (elem->base.le_type) {
				case OCRPT_OUTPUT_LE_UNSET:
					assert(0);
					break;
				case OCRPT_OUTPUT_LE_TEXT:
					ocrpt_expr_eval(elem->value);
					ocrpt_expr_eval(elem->format);
					ocrpt_expr_eval(elem->width);
					ocrpt_expr_eval(elem->align);
					ocrpt_expr_eval(elem->color);
					ocrpt_expr_eval(elem->bgcolor);
					ocrpt_expr_eval(elem->font_name);
					ocrpt_expr_eval(elem->font_size);
					ocrpt_expr_eval(elem->bold);
					ocrpt_expr_eval(elem->italic);
					ocrpt_expr_eval(elem->link);
					ocrpt_expr_eval(elem->translate);
					break;
				case OCRPT_OUTPUT_LE_IMAGE:
					ocrpt_layout_output_evaluate_image((ocrpt_image *)elem);
					break;
				case OCRPT_OUTPUT_LE_BARCODE:
					ocrpt_layout_output_evaluate_barcode((ocrpt_barcode *)elem);
					break;
				}
			}

			break;
		}
		case OCRPT_OUTPUT_HLINE:
			ocrpt_layout_output_evaluate_hline((ocrpt_hline *)oe);
			break;
		case OCRPT_OUTPUT_IMAGE:
			ocrpt_layout_output_evaluate_image((ocrpt_image *)oe);
			break;
		case OCRPT_OUTPUT_IMAGEEND:
			break;
		case OCRPT_OUTPUT_BARCODE:
			ocrpt_layout_output_evaluate_barcode((ocrpt_barcode *)oe);
			break;
		}
	}
}

void ocrpt_layout_output_internal_preamble(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, double page_width, double page_indent, double *page_position) {
	output->suppress_output = false;

	if (EXPR_VALID_NUMERIC(output->suppress)) {
		long suppress = mpfr_get_si(EXPR_NUMERIC(output->suppress), EXPR_RNDMODE(output->suppress));

		if (suppress) {
			output->suppress_output = true;
			return;
		}
	}

	if (r)
		output->old_page_position = *page_position;

	for (ocrpt_list *ol = output->output_list; ol; ol = ol->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)ol->data;
		ocrpt_line *l = (ocrpt_line *)oe;
		ocrpt_hline *hl = (ocrpt_hline *)oe;
		ocrpt_image *img = (ocrpt_image *)oe;
		ocrpt_barcode *bc = (ocrpt_barcode *)oe;
		const char *font_name;
		double font_size;

		switch (oe->type) {
		case OCRPT_OUTPUT_UNSET:
			assert(0);
			break;
		case OCRPT_OUTPUT_LINE:
			l->suppress_line = false;
			if (EXPR_VALID_NUMERIC(l->suppress)) {
				long suppress = mpfr_get_si(EXPR_NUMERIC(l->suppress), EXPR_RNDMODE(l->suppress));

				if (suppress) {
					l->suppress_line = true;
					break;
				}
			}

			if (o->output_functions.support_any_font) {
				if (EXPR_VALID_STRING(l->font_name))
					font_name = EXPR_STRING_VAL(l->font_name);
				else
					font_name = (r && r->font_name) ? r->font_name : p->font_name;

				if (EXPR_VALID_NUMERIC(l->font_size))
					font_size = mpfr_get_d(EXPR_NUMERIC(l->font_size), EXPR_RNDMODE(l->font_size));
				else
					font_size = r ? r->font_size : p->font_size;
			} else {
				font_name = "Courier";
				font_size = OCRPT_DEFAULT_FONT_SIZE;
			}

			if (o->output_functions.set_font_sizes)
				o->output_functions.set_font_sizes(o, font_name, font_size, false, false, &l->fontsz, &l->font_width);
			if (output->current_image)
				ocrpt_layout_line_get_text_sizes(o, p, pr, pd, r, output, l, page_width - output->current_image->image_width, page_position);
			else if (output->current_barcode)
				ocrpt_layout_line_get_text_sizes(o, p, pr, pd, r, output, l, page_width - output->current_barcode->barcode_width, page_position);
			else
				ocrpt_layout_line_get_text_sizes(o, p, pr, pd, r, output, l, page_width, page_position);
			break;
		case OCRPT_OUTPUT_HLINE:
			hl->suppress_hline = false;
			if (EXPR_VALID_NUMERIC(hl->suppress)) {
				long suppress = mpfr_get_si(EXPR_NUMERIC(hl->suppress), EXPR_RNDMODE(hl->suppress));

				if (suppress) {
					hl->suppress_hline = true;
					break;
				}
			}

			if (o->output_functions.support_any_font) {
				font_name = (r && r->font_name) ? r->font_name : p->font_name;

				if (EXPR_VALID_NUMERIC(hl->font_size))
					font_size = mpfr_get_d(EXPR_NUMERIC(hl->font_size), EXPR_RNDMODE(hl->font_size));
				else
					font_size = (r ? r->font_size : p->font_size);
			} else {
				font_name = "Courier";
				font_size = OCRPT_DEFAULT_FONT_SIZE;
			}

			if (o->output_functions.set_font_sizes)
				o->output_functions.set_font_sizes(o, font_name, font_size, false, false, NULL, &hl->font_width);
			break;
		case OCRPT_OUTPUT_IMAGE:
			output->current_image = NULL;
			output->current_barcode = NULL;
			img->suppress_image = false;
			if (EXPR_VALID_NUMERIC(img->suppress)) {
				long suppress = mpfr_get_si(EXPR_NUMERIC(img->suppress), EXPR_RNDMODE(img->suppress));

				if (suppress) {
					img->suppress_image = true;
					break;
				}
			}

			ocrpt_layout_image_setup(o, p, pr, pd, r, output, NULL, img, page_width, page_indent, page_position);
			break;
		case OCRPT_OUTPUT_IMAGEEND:
			break;
		case OCRPT_OUTPUT_BARCODE:
			output->current_image = NULL;
			output->current_barcode = NULL;
			bc->suppress_bc = false;
			if (EXPR_VALID_NUMERIC(bc->suppress)) {
				long suppress = mpfr_get_si(EXPR_NUMERIC(bc->suppress), EXPR_RNDMODE(bc->suppress));

				if (suppress) {
					bc->suppress_bc = true;
					break;
				}
			}

			ocrpt_layout_barcode_setup(o, p, pr, pd, r, output, NULL, bc, page_width, page_indent, page_position);
			break;
		}
	}
}

static inline void get_height_exceeded(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, double old_page_position, double new_page_position, bool *height_exceeded, bool *pd_height_exceeded, bool *r_height_exceeded) {
	*height_exceeded = o->output_functions.supports_page_break && ((new_page_position + ((pd && pd->border_width_expr) ? pd->border_width : 0.0)) > (p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height));
	*pd_height_exceeded = o->output_functions.supports_pd_height &&pd && pd->height_expr && (new_page_position > (pd->start_page_position + pd->remaining_height));
	*r_height_exceeded = o->output_functions.supports_report_height && r && r->height_valid && (r->remaining_height < (new_page_position - old_page_position));
}

void ocrpt_layout_output_adjust_page_position(ocrpt_output *output, double *page_position) {
	if (output->current_image) {
		if (*page_position < output->old_page_position + output->current_image->image_height)
			*page_position = output->old_page_position + output->current_image->image_height;
	} else if (output->current_barcode) {
		if (*page_position < output->old_page_position + output->current_barcode->barcode_height)
			*page_position = output->old_page_position + output->current_barcode->barcode_height;
	}
}

bool ocrpt_layout_output_internal(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, double page_width, double page_indent, double *page_position) {
	if (output->suppress_output)
		return false;

	for (; output->iter; output->iter = output->iter->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)output->iter->data;
		ocrpt_hline *hl = (ocrpt_hline *)oe;
		ocrpt_image *img = (ocrpt_image *)oe;
		ocrpt_barcode *bc = (ocrpt_barcode *)oe;

		switch (oe->type) {
		case OCRPT_OUTPUT_UNSET:
			assert(0);
			break;
		case OCRPT_OUTPUT_LINE: {
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
					ocrpt_layout_line(draw, o, p, pr, pd, r, br, output, l, page_width - output->current_image->image_width, page_indent + output->current_image->image_width, page_position);
				else if (output->current_barcode)
					ocrpt_layout_line(draw, o, p, pr, pd, r, br, output, l, page_width - output->current_barcode->barcode_width, page_indent + output->current_barcode->barcode_width, page_position);
				else
					ocrpt_layout_line(draw, o, p, pr, pd, r, br, output, l, page_width, page_indent, page_position);
			}

			l->current_line = 0;
			break;
		}
		case OCRPT_OUTPUT_HLINE:
			if (hl->suppress_hline)
				break;

			if (output->current_image)
				ocrpt_layout_hline(draw, o, p, pr, pd, r, br, output, hl, page_width - output->current_image->image_width, page_indent + output->current_image->image_width, page_position);
			else if (output->current_barcode)
				ocrpt_layout_hline(draw, o, p, pr, pd, r, br, output, hl, page_width - output->current_barcode->barcode_width, page_indent + output->current_barcode->barcode_width, page_position);
			else
				ocrpt_layout_hline(draw, o, p, pr, pd, r, br, output, hl, page_width, page_indent, page_position);
			break;
		case OCRPT_OUTPUT_IMAGE:
			ocrpt_layout_output_adjust_page_position(output, page_position);

			output->current_image = NULL;
			output->current_barcode = NULL;
			if (img->suppress_image)
				break;

			ocrpt_layout_image(draw, o, p, pr, pd, r, br, output, img, false, page_width, page_indent, page_position);
			break;
		case OCRPT_OUTPUT_IMAGEEND:
			ocrpt_layout_output_adjust_page_position(output, page_position);

			output->current_image = NULL;
			output->current_barcode = NULL;

			ocrpt_layout_imageend(draw, o, p, pr, pd, r, br, output);
			break;
		case OCRPT_OUTPUT_BARCODE:
			ocrpt_layout_output_adjust_page_position(output, page_position);

			output->current_image = NULL;
			output->current_barcode = NULL;
			if (bc->suppress_bc)
				break;

			ocrpt_layout_barcode(draw, o, p, pr, pd, r, br, output, bc, false, page_width, page_indent, page_position);
			break;
		}
	}

	ocrpt_layout_output_adjust_page_position(output, page_position);

	output->current_image = NULL;

	return false;
}

static void ocrpt_layout_output_headers_after_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	*page_position = ocrpt_layout_top_margin(o, p);
	bool is_current_page_first = (o->output_functions.is_current_page_first ? o->output_functions.is_current_page_first(o) : true);
	if (!p->suppress_pageheader_firstpage || (p->suppress_pageheader_firstpage && !is_current_page_first)) {
		if (o->output_functions.reopen_tags_across_pages && !o->precalculate) {
			if (o->output_functions.end_part_column)
				o->output_functions.end_part_column(o, p, pr, pd);

			if (o->output_functions.end_part_row)
				o->output_functions.end_part_row(o, p, pr);

			if (o->output_functions.start_part_row)
				o->output_functions.start_part_row(o, p, NULL);

			if (o->output_functions.start_part_column)
				o->output_functions.start_part_column(o, p, NULL, NULL);
		}

		if (!o->precalculate && p->pageheader.output_list && o->output_functions.start_output)
			o->output_functions.start_output(o, p, NULL, NULL, NULL, NULL, &p->pageheader);

		ocrpt_layout_output_evaluate(&p->pageheader);
		ocrpt_layout_output_init(&p->pageheader);
		ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, page_position);
		ocrpt_layout_output_internal(!o->precalculate, o, p, NULL, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, page_position);
		if (!o->precalculate)
			p->pageheader_printed = true;

		if (!o->precalculate && p->pageheader.output_list && o->output_functions.end_output)
			o->output_functions.end_output(o, p, NULL, NULL, NULL, NULL, &p->pageheader);

		if (o->output_functions.reopen_tags_across_pages && !o->precalculate) {
			if (o->output_functions.end_part_column)
				o->output_functions.end_part_column(o, p, NULL, NULL);

			if (o->output_functions.end_part_row)
				o->output_functions.end_part_row(o, p, NULL);

			if (o->output_functions.start_part_row)
				o->output_functions.start_part_row(o, p, pr);

			if (o->output_functions.start_part_column)
				o->output_functions.start_part_column(o, p, pr, pd);
		}
	}

	if (rows == 1 && !pr->start_page) {
		if (r) {
			if (r->current_iteration == 0) {
				if (o->output_functions.get_current_page)
					pr->start_page = o->output_functions.get_current_page(o);
				/* else? */
				pr->start_page_position = *page_position;
				pd->start_page_position = *page_position;
			}
			if (pd->border_width_expr)
				*page_position += pd->border_width;
			ocrpt_layout_output_init(&r->reportheader);
			ocrpt_layout_output(o, p, pr, pd, r, br, &r->reportheader, rows, newpage, page_indent, page_position, old_page_position);
		}
	} else {
		pd->start_page_position = *page_position;
		if (pd->border_width_expr)
			*page_position += pd->border_width;
	}
	ocrpt_layout_output_highprio_fieldheader(o, p, pr, pd, r, br, rows, newpage, page_indent, page_position, old_page_position);
}

void ocrpt_layout_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	ocrpt_list *old_current_page = NULL;
	double new_page_position;
	bool page_break = false;

	if (*newpage) {
		if ((o->output_functions.get_current_page && o->output_functions.get_current_page(o)) || !o->output_functions.get_current_page) {
			ocrpt_layout_output_evaluate(&p->pageheader);
			ocrpt_layout_output_evaluate(&p->pagefooter);

			bool is_current_page_first = (o->output_functions.is_current_page_first ? o->output_functions.is_current_page_first(o) : true);
			if (!p->suppress_pageheader_firstpage || (p->suppress_pageheader_firstpage && !is_current_page_first)) {
				if (o->output_functions.reopen_tags_across_pages && !o->precalculate) {
					if (o->output_functions.end_part_column)
						o->output_functions.end_part_column(o, p, pr, pd);

					if (o->output_functions.end_part_row)
						o->output_functions.end_part_row(o, p, pr);

					if (o->output_functions.start_part_row)
						o->output_functions.start_part_row(o, p, NULL);

					if (o->output_functions.start_part_column)
						o->output_functions.start_part_column(o, p, NULL, NULL);
				}

				if (!o->precalculate && p->pageheader.output_list && o->output_functions.start_output)
					o->output_functions.start_output(o, p, NULL, NULL, NULL, NULL, &p->pageheader);

				double top_page_position = ocrpt_layout_top_margin(o, p);
				ocrpt_layout_output_init(&p->pageheader);
				ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &top_page_position);
				ocrpt_layout_output_internal(!o->precalculate, o, p, NULL, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &top_page_position);
				if (!o->precalculate)
					p->pageheader_printed = true;

				if (!o->precalculate && p->pageheader.output_list && o->output_functions.end_output)
					o->output_functions.end_output(o, p, pr, pd, r, br, &p->pageheader);

				if (o->output_functions.reopen_tags_across_pages && !o->precalculate) {
					if (o->output_functions.end_part_column)
						o->output_functions.end_part_column(o, p, NULL, NULL);

					if (o->output_functions.end_part_row)
						o->output_functions.end_part_row(o, p, NULL);

					if (o->output_functions.start_part_row)
						o->output_functions.start_part_row(o, p, pr);

					if (o->output_functions.start_part_column)
						o->output_functions.start_part_column(o, p, pr, pd);
				}
			}

			if (!o->precalculate && p->pagefooter.output_list && o->output_functions.start_output)
				o->output_functions.start_output(o, p, pr, pd, r, br, &p->pagefooter);

			double bottom_page_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
			ocrpt_layout_output_init(&p->pagefooter);
			ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &bottom_page_position);
			ocrpt_layout_output_internal(!o->precalculate, o, p, NULL, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &bottom_page_position);
			if (!o->precalculate)
				p->pageheader_printed = false;

			if (!o->precalculate && p->pagefooter.output_list && o->output_functions.end_output)
				o->output_functions.end_output(o, p, pr, pd, r, br, &p->pagefooter);
		}

		/* Set newpage to false first to prevent infinite recursion. */
		*newpage = false;
		if (o->output_functions.add_new_page)
			o->output_functions.add_new_page(o, p, pr, pd, r, rows, newpage, page_indent, page_position, old_page_position);

		ocrpt_layout_output_headers_after_add_new_page(o, p, pr, pd, r, br, rows, newpage, page_indent, page_position, old_page_position);

		if (!pr->start_page && !r->current_iteration) {
			if (o->output_functions.get_current_page)
				pr->start_page = o->output_functions.get_current_page(o);
			pr->start_page_position = *page_position;
			pr->end_page_position = *page_position;
			pd->start_page_position = *page_position;
		}
	}

	if (!o->precalculate && output->output_list && o->output_functions.start_output)
		o->output_functions.start_output(o, p, pr, pd, r, br, output);

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
	if (o->output_functions.get_current_page)
		old_current_page = o->output_functions.get_current_page(o);
	*old_page_position = new_page_position = *page_position;

	ocrpt_layout_output_position_push(output);
	ocrpt_layout_output_internal_preamble(o, p, pr, pd, r, output, pd ? pd->column_width : p->page_width, *page_indent, &new_page_position);
	bool memo_break = ocrpt_layout_output_internal(false, o, p, pr, pd, r, br, output, pd ? pd->column_width : p->page_width, *page_indent, &new_page_position);

	bool height_exceeded, pd_height_exceeded, r_height_exceeded;

	if (memo_break) {
		/*
		 * Memo break detected. Rewind to the previous page position and
		 * draw before handling column or page break.
		 */
		new_page_position = *old_page_position;

		ocrpt_layout_output_position_pop(output);
		ocrpt_layout_output_internal_preamble(o, p, pr, pd, r, output, pd ? pd->column_width : p->page_width, *page_indent, &new_page_position);
		memo_break = ocrpt_layout_output_internal(!o->precalculate, o, p, pr, pd, r, br, output, pd ? pd->column_width : p->page_width, *page_indent, &new_page_position);

		if (pd && pd->max_page_position < new_page_position)
			pd->max_page_position = new_page_position;

		height_exceeded = output->height_exceeded;
		pd_height_exceeded = output->pd_height_exceeded;
		r_height_exceeded = output->r_height_exceeded;
	} else
		get_height_exceeded(o, p, pr, pd, r, *old_page_position, new_page_position, &height_exceeded, &pd_height_exceeded, &r_height_exceeded);

	if (r && r_height_exceeded)
		r->finished = true;

	if (height_exceeded || pd_height_exceeded) {
		if (!memo_break && pd)
			pd->max_page_position = *old_page_position;
		if (pd)
			pd->current_column++;

		if (pd && pd->current_column >= pd->detail_columns) {
			if (pd_height_exceeded)
				pd->finished = true;
			if (height_exceeded) {
				page_break = true;
			}
		} else
			page_break = false;

		if (page_break) {
			if (pd->height_expr && o->output_functions.supports_pd_height)
				pd->remaining_height -= new_page_position - pd->start_page_position;

			pd->current_column = 0;
			*page_indent = pd->page_indent;
			if (pd->border_width_expr)
				*page_indent += pd->border_width;

			/* Use the previous row data temporarily */
			o->residx = ocrpt_expr_prev_residx(o->residx);

			if (!o->precalculate && pd && pd->border_width_expr && o->output_functions.draw_rectangle)
				o->output_functions.draw_rectangle(o, p, pr, pd, r,
													&pd->border_color, pd->border_width,
													pd->page_indent0 + 0.5 * pd->border_width,
													pd->start_page_position + 0.5 * pd->border_width,
													pd->real_width - pd->border_width,
													pd->max_page_position - pd->start_page_position);

			if (!o->precalculate && p->pagefooter.output_list && o->output_functions.start_output)
				o->output_functions.start_output(o, p, pr, pd, r, br, &p->pagefooter);

			double bottom_page_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
			ocrpt_layout_output_evaluate(&p->pagefooter);
			ocrpt_layout_output_init(&p->pagefooter);
			ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &bottom_page_position);
			ocrpt_layout_output_internal(!o->precalculate, o, p, NULL, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &bottom_page_position);
			if (!o->precalculate)
				p->pageheader_printed = false;

			if (!o->precalculate && p->pagefooter.output_list && o->output_functions.end_output)
				o->output_functions.end_output(o, p, pr, pd, r, br, &p->pagefooter);

			/* Switch back to the current row data */
			o->residx = ocrpt_expr_next_residx(o->residx);

			/* Set newpage to false first to prevent infinite recursion. */
			*newpage = false;
			if (o->output_functions.add_new_page)
				o->output_functions.add_new_page(o, p, pr, pd, r, rows, newpage, page_indent, &new_page_position, old_page_position);

			ocrpt_layout_output_headers_after_add_new_page(o, p, pr, pd, r, br, rows, newpage, page_indent, &new_page_position, old_page_position);

			pd->max_page_position = 0.0;
		} else if (pd && !pd->finished) {
			*page_indent += pd->column_width + (o->size_in_points ? 1.0 : 72.0) * pd->column_pad;
			new_page_position = pd->start_page_position;
			if (pd->border_width_expr)
				new_page_position += pd->border_width;
			ocrpt_layout_output_highprio_fieldheader(o, p, pr, pd, r, br, rows, newpage, page_indent, &new_page_position, old_page_position);
		}

		if (pd && pd->finished)
			return;

		page_break = true;
	}

	if (!page_break && !memo_break) {
		if (o->output_functions.set_current_page)
			o->output_functions.set_current_page(o, old_current_page);
		new_page_position = *old_page_position;
	}

	if (!r || (r && !r->finished /* && pd && !pd->finished */)) {
		if (!output->has_memo || !memo_break)
			ocrpt_layout_output_init(output);
		ocrpt_layout_output_internal_preamble(o, p, pr, pd, r, output, pd ? pd->column_width : p->page_width, *page_indent, &new_page_position);
		memo_break = ocrpt_layout_output_internal(!o->precalculate, o, p, pr, pd, r, br, output, pd ? pd->column_width : p->page_width, *page_indent, &new_page_position);
	}

	if (r && r->height_valid && o->output_functions.supports_report_height)
		r->remaining_height -= new_page_position - *old_page_position;

	*page_position = new_page_position;
	if (pd && pd->max_page_position < new_page_position)
		pd->max_page_position = new_page_position;

	if (memo_break)
		goto restart;

	if (!o->precalculate && output->output_list && o->output_functions.end_output)
		o->output_functions.end_output(o, p, pr, pd, r, br, output);
}

double ocrpt_layout_top_margin(opencreport *o, ocrpt_part *p) {
	double top_margin;

	if (p->top_margin_expr)
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

	if (p->bottom_margin_expr)
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

	if (p->left_margin_expr)
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

	if (p->right_margin_expr)
		right_margin = p->right_margin;
	else
		right_margin = OCRPT_DEFAULT_RIGHT_MARGIN;

	if (o->size_in_points)
		return right_margin;
	else
		return right_margin * 72.0;
}

void ocrpt_layout_output_highprio_fieldheader(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
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
			ocrpt_expr_init_iterative_results(r->detailcnt, OCRPT_RESULT_NUMBER);
			ocrpt_expr_eval(r->detailcnt);
			ocrpt_report_evaluate_detailcnt_dependees(r);
			ocrpt_layout_output_init(&r->fieldheader);
			ocrpt_layout_output(o, p, pr, pd, r, br, &r->fieldheader, rows, newpage, page_indent, page_position, old_page_position);
		}
	}
}

ocrpt_expr *ocrpt_layout_expr_parse(opencreport *o, ocrpt_report *r, const char *expr, bool report, bool create_string) {
	ocrpt_expr *e;
	char *err = NULL;

	if (!expr)
		return NULL;

	if (r)
		e = ocrpt_report_expr_parse(r, expr, &err);
	else
		e = ocrpt_expr_parse(o, expr, &err);

	if (!e) {
		if (report)
			ocrpt_err_printf("Cannot parse: %s\n", expr);

		if (create_string)
			e = ocrpt_newstring_add_to_list(o, r, expr);
	}
	ocrpt_strfree(err);

	return e;
}

DLL_EXPORT_SYM void ocrpt_layout_part_page_header_set_report(ocrpt_part *p, ocrpt_report *r) {
	if (!p || !p->o || p->o->executing || (r && r->executing))
		return;

	p->pageheader.r = r;
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_layout_part_page_header(ocrpt_part *p) {
	if (!p)
		return NULL;

	return &p->pageheader;
}

DLL_EXPORT_SYM void ocrpt_layout_part_page_footer_set_report(ocrpt_part *p, ocrpt_report *r) {
	if (!p || !p->o || p->o->executing || (r && r->executing))
		return;

	p->pagefooter.r = r;
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_layout_part_page_footer(ocrpt_part *p) {
	if (!p)
		return NULL;

	return &p->pagefooter;
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_layout_report_nodata(ocrpt_report *r) {
	if (!r)
		return NULL;

	return &r->nodata;
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_layout_report_header(ocrpt_report *r) {
	if (!r)
		return NULL;

	return &r->reportheader;
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_layout_report_footer(ocrpt_report *r) {
	if (!r)
		return NULL;

	return &r->reportfooter;
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_layout_report_field_header(ocrpt_report *r) {
	if (!r)
		return NULL;

	return &r->fieldheader;
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_layout_report_field_details(ocrpt_report *r) {
	if (!r)
		return NULL;

	return &r->fielddetails;
}

DLL_EXPORT_SYM ocrpt_line *ocrpt_output_add_line(ocrpt_output *output) {
	if (!output || (output->o && output->o->executing))
		return NULL;

	ocrpt_line *line = ocrpt_mem_malloc(sizeof(ocrpt_line));
	memset(line, 0, sizeof(ocrpt_line));
	line->base.type = OCRPT_OUTPUT_LINE;
	line->output = output;
	output->output_list = ocrpt_list_append(output->output_list, line);

	return line;
}

DLL_EXPORT_SYM ocrpt_text *ocrpt_line_add_text(ocrpt_line *line) {
	if (!line || (line->output->o && line->output->o->executing))
		return NULL;

	ocrpt_text *elem = ocrpt_mem_malloc(sizeof(ocrpt_text));
	memset(elem, 0, sizeof(ocrpt_text));
	elem->base.le_type = OCRPT_OUTPUT_LE_TEXT;
	elem->output = line->output;
	line->elements = ocrpt_list_append(line->elements, elem);

	return elem;
}

DLL_EXPORT_SYM ocrpt_image *ocrpt_line_add_image(ocrpt_line *line) {
	if (!line || (line->output->o && line->output->o->executing))
		return NULL;

	ocrpt_image *image = ocrpt_mem_malloc(sizeof(ocrpt_image));
	memset(image, 0, sizeof(ocrpt_image));
	image->base.le_type = OCRPT_OUTPUT_LE_IMAGE;
	image->output = line->output;
	image->in_line = true;
	line->elements = ocrpt_list_append(line->elements, image);

	return image;
}

DLL_EXPORT_SYM ocrpt_barcode *ocrpt_line_add_barcode(ocrpt_line *line) {
	if (!line || (line->output->o && line->output->o->executing))
		return NULL;

	ocrpt_barcode *bc = ocrpt_mem_malloc(sizeof(ocrpt_barcode));
	memset(bc, 0, sizeof(ocrpt_barcode));
	bc->base.le_type = OCRPT_OUTPUT_LE_BARCODE;
	bc->output = line->output;
	bc->in_line = true;
	line->elements = ocrpt_list_append(line->elements, bc);

	return bc;
}

DLL_EXPORT_SYM ocrpt_hline *ocrpt_output_add_hline(ocrpt_output *output) {
	if (!output || (output->o && output->o->executing))
		return NULL;

	ocrpt_hline *hline = ocrpt_mem_malloc(sizeof(ocrpt_hline));
	memset(hline, 0, sizeof(ocrpt_hline));
	hline->base.type = OCRPT_OUTPUT_HLINE;
	hline->output = output;
	output->output_list = ocrpt_list_append(output->output_list, hline);

	return hline;
}

DLL_EXPORT_SYM ocrpt_image *ocrpt_output_add_image(ocrpt_output *output) {
	if (!output || (output->o && output->o->executing))
		return NULL;

	ocrpt_image *image = ocrpt_mem_malloc(sizeof(ocrpt_image));
	memset(image, 0, sizeof(ocrpt_image));
	image->base.type = OCRPT_OUTPUT_IMAGE;
	image->output = output;
	output->output_list = ocrpt_list_append(output->output_list, image);

	return image;
}

DLL_EXPORT_SYM void ocrpt_output_add_image_end(ocrpt_output *output) {
	if (!output || (output->o && output->o->executing))
		return;

	ocrpt_output_element *elem = ocrpt_mem_malloc(sizeof(ocrpt_output_element));
	memset(elem, 0, sizeof(ocrpt_output_element));
	elem->type = OCRPT_OUTPUT_IMAGEEND;
	output->output_list = ocrpt_list_append(output->output_list, elem);
}

#define SET_IMAGE_EXPR(exprname, report, create_string) { \
		if (!image || !(image->base.type == OCRPT_OUTPUT_IMAGE || image->base.le_type == OCRPT_OUTPUT_LE_IMAGE) || (image->output->o && image->output->o->executing)) \
			return NULL; \
		ocrpt_expr_free(image->exprname); \
		image->exprname = expr_string ? ocrpt_layout_expr_parse(image->output->o, image->output->r, expr_string, report, create_string) : NULL; \
		return image->exprname; \
	}

#define GET_IMAGE_EXPR(exprname) { \
		return image ? image->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_set_value(ocrpt_image *image, const char *expr_string) {
	SET_IMAGE_EXPR(value, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_get_value(ocrpt_image *image) {
	GET_IMAGE_EXPR(value);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_set_suppress(ocrpt_image *image, const char *expr_string) {
	SET_IMAGE_EXPR(suppress, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_get_suppress(ocrpt_image *image) {
	GET_IMAGE_EXPR(suppress);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_set_type(ocrpt_image *image, const char *expr_string) {
	SET_IMAGE_EXPR(imgtype, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_get_type(ocrpt_image *image) {
	GET_IMAGE_EXPR(imgtype);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_set_width(ocrpt_image *image, const char *expr_string) {
	SET_IMAGE_EXPR(width, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_get_width(ocrpt_image *image) {
	GET_IMAGE_EXPR(width);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_set_height(ocrpt_image *image, const char *expr_string) {
	SET_IMAGE_EXPR(height, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_get_height(ocrpt_image *image) {
	GET_IMAGE_EXPR(height);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_set_alignment(ocrpt_image *image, const char *expr_string) {
	if (!image || !(image->base.type == OCRPT_OUTPUT_IMAGE || image->base.le_type == OCRPT_OUTPUT_LE_IMAGE) || (image->output->o && image->output->o->executing))
		return NULL;

	if (image->align)
		ocrpt_expr_free(image->align);

	if (expr_string) {
		if (strcasecmp(expr_string, "left") == 0 || strcasecmp(expr_string, "right") == 0 || strcasecmp(expr_string, "center") == 0)
			image->align = ocrpt_newstring_add_to_list(image->output->o, image->output->r, expr_string);
		else
			image->align = ocrpt_layout_expr_parse(image->output->o, image->output->r, expr_string, true, false);
	} else
		image->align = NULL;

	return image->align;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_get_alignment(ocrpt_image *image) {
	GET_IMAGE_EXPR(align);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_set_bgcolor(ocrpt_image *image, const char *expr_string) {
	SET_IMAGE_EXPR(bgcolor, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_get_bgcolor(ocrpt_image *image) {
	GET_IMAGE_EXPR(bgcolor);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_set_text_width(ocrpt_image *image, const char *expr_string) {
	SET_IMAGE_EXPR(text_width, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_image_get_text_width(ocrpt_image *image) {
	GET_IMAGE_EXPR(text_width);
}

DLL_EXPORT_SYM ocrpt_barcode *ocrpt_output_add_barcode(ocrpt_output *output) {
	if (!output || (output->o && output->o->executing))
		return NULL;

	ocrpt_barcode *barcode = ocrpt_mem_malloc(sizeof(ocrpt_barcode));
	memset(barcode, 0, sizeof(ocrpt_barcode));
	barcode->base.type = OCRPT_OUTPUT_BARCODE;
	barcode->output = output;
	output->output_list = ocrpt_list_append(output->output_list, barcode);

	return barcode;
}

static void ocrpt_text_set_rvalue_internal(ocrpt_text *text) {
	if (text->format)
		text->format->rvalue = text->value;
	if (text->width)
		text->width->rvalue = text->value;
	if (text->align)
		text->align->rvalue = text->value;
	if (text->color)
		text->color->rvalue = text->value;
	if (text->bgcolor)
		text->bgcolor->rvalue = text->value;
	if (text->font_name)
		text->font_name->rvalue = text->value;
	if (text->font_size)
		text->font_size->rvalue = text->value;
	if (text->bold)
		text->bold->rvalue = text->value;
	if (text->italic)
		text->italic->rvalue = text->value;
	if (text->link)
		text->link->rvalue = text->value;
	if (text->translate)
		text->translate->rvalue = text->value;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_value_string(ocrpt_text *text, const char *string) {
	if (!text || text->base.le_type != OCRPT_OUTPUT_LE_TEXT || (text->output->o && text->output->o->executing))
		return NULL;

	ocrpt_expr_free(text->value);
	text->value = string ? ocrpt_newstring_add_to_list(text->output->o, text->output->r, string) : NULL;
	ocrpt_text_set_rvalue_internal(text);
	return text->value;
}

#define SET_TEXT_EXPR(exprname, report, create_string) { \
		if (!text || text->base.le_type != OCRPT_OUTPUT_LE_TEXT || (text->output->o && text->output->o->executing)) \
			return NULL; \
		ocrpt_expr_free(text->exprname); \
		text->exprname = expr_string ? ocrpt_layout_expr_parse(text->output->o, text->output->r, expr_string, report, create_string) : NULL; \
		ocrpt_text_set_rvalue_internal(text); \
		return text->exprname; \
	}

#define GET_TEXT_EXPR(exprname) { \
		return text ? text->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_value_expr(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(value, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_value(ocrpt_text *text) {
	GET_TEXT_EXPR(value);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_value_delayed(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(delayed, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_value_delayed(ocrpt_text *text) {
	GET_TEXT_EXPR(delayed);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_format(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(format, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_format(ocrpt_text *text) {
	GET_TEXT_EXPR(format);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_translate(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(translate, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_translate(ocrpt_text *text) {
	GET_TEXT_EXPR(translate);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_width(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(width, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_width(ocrpt_text *text) {
	GET_TEXT_EXPR(width);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_alignment(ocrpt_text *text, const char *expr_string) {
	if (!text || text->base.le_type != OCRPT_OUTPUT_LE_TEXT || (text->output->o && text->output->o->executing))
		return NULL;

	ocrpt_expr_free(text->align);

	if (expr_string) {
		if (strcasecmp(expr_string, "left") == 0 || strcasecmp(expr_string, "right") == 0 || strcasecmp(expr_string, "center") == 0 || strcasecmp(expr_string, "justified") == 0)
			text->align = expr_string ? ocrpt_newstring_add_to_list(text->output->o, text->output->r, expr_string) : NULL;
		else
			text->align = expr_string ? ocrpt_layout_expr_parse(text->output->o, text->output->r, expr_string, true, false) : NULL;
	} else
		text->align = NULL;

	ocrpt_text_set_rvalue_internal(text);
	return text->align;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_alignment(ocrpt_text *text) {
	GET_TEXT_EXPR(align);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_color(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(color, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_color(ocrpt_text *text) {
	GET_TEXT_EXPR(color);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_bgcolor(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(bgcolor, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_bgcolor(ocrpt_text *text) {
	GET_TEXT_EXPR(bgcolor);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_font_name(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(font_name, false, true);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_font_name(ocrpt_text *text) {
	GET_TEXT_EXPR(font_name);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_font_size(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(font_size, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_font_size(ocrpt_text *text) {
	GET_TEXT_EXPR(font_size);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_bold(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(bold, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_bold(ocrpt_text *text) {
	GET_TEXT_EXPR(bold);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_italic(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(italic, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_italic(ocrpt_text *text) {
	GET_TEXT_EXPR(italic);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_link(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(link, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_link(ocrpt_text *text) {
	GET_TEXT_EXPR(link);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_memo(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(memo_expr, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_memo(ocrpt_text *text) {
	GET_TEXT_EXPR(memo_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_memo_wrap_chars(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(wrap_chars, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_memo_wrap_chars(ocrpt_text *text) {
	GET_TEXT_EXPR(wrap_chars);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_set_memo_max_lines(ocrpt_text *text, const char *expr_string) {
	SET_TEXT_EXPR(max_lines, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_text_get_memo_max_lines(ocrpt_text *text) {
	GET_TEXT_EXPR(max_lines);
}

#define SET_BARCODE_EXPR(exprname, report, create_string) { \
		if (!bc || !(bc->base.type == OCRPT_OUTPUT_BARCODE || bc->base.le_type == OCRPT_OUTPUT_LE_BARCODE) || (bc->output->o && bc->output->o->executing)) \
			return NULL; \
		ocrpt_expr_free(bc->exprname); \
		bc->exprname = expr_string ? ocrpt_layout_expr_parse(bc->output->o, bc->output->r, expr_string, report, create_string) : NULL; \
		return bc->exprname; \
	}

#define GET_BARCODE_EXPR(exprname) { \
		return bc ? bc->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_set_value(ocrpt_barcode *bc, const char *expr_string) {
	SET_BARCODE_EXPR(value, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_get_value(ocrpt_barcode *bc) {
	GET_BARCODE_EXPR(value);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_set_value_delayed(ocrpt_barcode *bc, const char *expr_string) {
	SET_BARCODE_EXPR(delayed, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_get_value_delayed(ocrpt_barcode *bc) {
	GET_BARCODE_EXPR(delayed);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_set_suppress(ocrpt_barcode *bc, const char *expr_string) {
	SET_BARCODE_EXPR(suppress, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_get_suppress(ocrpt_barcode *bc) {
	GET_BARCODE_EXPR(suppress);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_set_type(ocrpt_barcode *bc, const char *expr_string) {
	SET_BARCODE_EXPR(bctype, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_get_type(ocrpt_barcode *bc) {
	GET_BARCODE_EXPR(bctype);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_set_width(ocrpt_barcode *bc, const char *expr_string) {
	SET_BARCODE_EXPR(width, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_get_width(ocrpt_barcode *bc) {
	GET_BARCODE_EXPR(width);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_set_height(ocrpt_barcode *bc, const char *expr_string) {
	SET_BARCODE_EXPR(height, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_get_height(ocrpt_barcode *bc) {
	GET_BARCODE_EXPR(height);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_set_color(ocrpt_barcode *bc, const char *expr_string) {
	SET_BARCODE_EXPR(color, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_get_color(ocrpt_barcode *bc) {
	GET_BARCODE_EXPR(color);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_set_bgcolor(ocrpt_barcode *bc, const char *expr_string) {
	SET_BARCODE_EXPR(bgcolor, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_barcode_get_bgcolor(ocrpt_barcode *bc) {
	GET_BARCODE_EXPR(bgcolor);
}

#define SET_LINE_EXPR(exprname, report, create_string) { \
		if (!line || line->base.type != OCRPT_OUTPUT_LINE || (line->output->o && line->output->o->executing)) \
			return NULL; \
		ocrpt_expr_free(line->exprname); \
		line->exprname = expr_string ? ocrpt_layout_expr_parse(line->output->o, line->output->r, expr_string, report, create_string) : NULL; \
		return line->exprname; \
	}

#define GET_LINE_EXPR(exprname) { \
		return line ? line->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_set_font_name(ocrpt_line *line, const char *expr_string) {
	SET_LINE_EXPR(font_name, false, true);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_get_font_name(ocrpt_line *line) {
	GET_LINE_EXPR(font_name);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_set_font_size(ocrpt_line *line, const char *expr_string) {
	SET_LINE_EXPR(font_size, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_get_font_size(ocrpt_line *line) {
	GET_LINE_EXPR(font_size);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_set_bold(ocrpt_line *line, const char *expr_string) {
	SET_LINE_EXPR(bold, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_get_bold(ocrpt_line *line) {
	GET_LINE_EXPR(bold);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_set_italic(ocrpt_line *line, const char *expr_string) {
	SET_LINE_EXPR(italic, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_get_italic(ocrpt_line *line) {
	GET_LINE_EXPR(italic);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_set_suppress(ocrpt_line *line, const char *expr_string) {
	SET_LINE_EXPR(suppress, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_get_suppress(ocrpt_line *line) {
	GET_LINE_EXPR(suppress);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_set_color(ocrpt_line *line, const char *expr_string) {
	SET_LINE_EXPR(color, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_get_color(ocrpt_line *line) {
	GET_LINE_EXPR(color);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_set_bgcolor(ocrpt_line *line, const char *expr_string) {
	SET_LINE_EXPR(bgcolor, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_line_get_bgcolor(ocrpt_line *line) {
	GET_LINE_EXPR(bgcolor);
}

#define SET_HLINE_EXPR(exprname, report, create_string) { \
		if (!hline || hline->base.type != OCRPT_OUTPUT_HLINE || (hline->output->o && hline->output->o->executing)) \
			return NULL; \
		ocrpt_expr_free(hline->exprname); \
		hline->exprname = expr_string ? ocrpt_layout_expr_parse(hline->output->o, hline->output->r, expr_string, report, create_string) : NULL; \
		return hline->exprname; \
	}

#define GET_HLINE_EXPR(exprname) { \
		return hline ? hline->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_set_size(ocrpt_hline *hline, const char *expr_string) {
	SET_HLINE_EXPR(size, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_get_size(ocrpt_hline *hline) {
	GET_HLINE_EXPR(size);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_set_alignment(ocrpt_hline *hline, const char *expr_string) {
	SET_HLINE_EXPR(align, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_get_alignment(ocrpt_hline *hline) {
	GET_HLINE_EXPR(align);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_set_indentation(ocrpt_hline *hline, const char *expr_string) {
	SET_HLINE_EXPR(indent, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_get_indentation(ocrpt_hline *hline) {
	GET_HLINE_EXPR(indent);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_set_length(ocrpt_hline *hline, const char *expr_string) {
	SET_HLINE_EXPR(length, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_get_length(ocrpt_hline *hline) {
	GET_HLINE_EXPR(length);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_set_font_size(ocrpt_hline *hline, const char *expr_string) {
	SET_HLINE_EXPR(font_size, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_get_font_size(ocrpt_hline *hline) {
	GET_HLINE_EXPR(font_size);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_set_suppress(ocrpt_hline *hline, const char *expr_string) {
	SET_HLINE_EXPR(suppress, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_get_suppress(ocrpt_hline *hline) {
	GET_HLINE_EXPR(suppress);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_set_color(ocrpt_hline *hline, const char *expr_string) {
	SET_HLINE_EXPR(color, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_hline_get_color(ocrpt_hline *hline) {
	GET_HLINE_EXPR(color);
}

#define SET_OUTPUT_EXPR(exprname, report, create_string) { \
		if (!output || (output->o && output->o->executing)) \
			return NULL; \
		ocrpt_expr_free(output->exprname); \
		output->exprname = expr_string ? ocrpt_layout_expr_parse(output->o, output->r, expr_string, report, create_string) : NULL; \
		return output->exprname; \
	}

#define GET_OUTPUT_EXPR(exprname) { \
		return output ? output->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_output_set_suppress(ocrpt_output *output, const char *expr_string) {
	SET_OUTPUT_EXPR(suppress, true, false);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_output_get_suppress(ocrpt_output *output) {
	GET_OUTPUT_EXPR(suppress);
}

#define SET_O_EXPR_REPORT(exprname) { \
		if (!o || o->executing) \
			return NULL; \
		ocrpt_expr_free(o->exprname); \
		char *err = NULL; \
		o->exprname = expr_string ? ocrpt_expr_parse(o, expr_string, &err) : NULL; \
		if (err) { \
			ocrpt_err_printf("%s: %s\n", __func__, err); \
			ocrpt_strfree(err); \
		} \
		return o->exprname; \
	}

#define GET_O_EXPR_REPORT(exprname) { \
		return o ? o->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_set_noquery_show_nodata(opencreport *o, const char *expr_string) {
	SET_O_EXPR_REPORT(noquery_show_nodata_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_get_noquery_show_nodata(opencreport *o) {
	GET_O_EXPR_REPORT(noquery_show_nodata_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_set_report_height_after_last(opencreport *o, const char *expr_string) {
	SET_O_EXPR_REPORT(report_height_after_last_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_get_report_height_after_last(opencreport *o) {
	GET_O_EXPR_REPORT(report_height_after_last_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_set_size_unit(opencreport *o, const char *expr_string) {
	SET_O_EXPR_REPORT(size_unit_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_get_size_unit(opencreport *o) {
	GET_O_EXPR_REPORT(size_unit_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_set_follower_match_single(opencreport *o, const char *expr_string) {
	SET_O_EXPR_REPORT(follower_match_single_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_get_follower_match_single(opencreport *o) {
	GET_O_EXPR_REPORT(follower_match_single_expr);
}

DLL_EXPORT_SYM void ocrpt_set_follower_match_single_direct(opencreport *o, bool value) {
	if (!o || o->executing)
		return;

	o->follower_match_single = value;
}

DLL_EXPORT_SYM bool ocrpt_get_follower_match_single_direct(opencreport *o) {
	return o ? o->follower_match_single : false;
}

#define SET_PART_EXPR(exprname) { \
		if (!p || (p->o && p->o->executing)) \
			return NULL; \
		ocrpt_expr_free(p->exprname); \
		p->exprname = expr_string ? ocrpt_expr_parse(p->o, expr_string, NULL) : NULL; \
		return p->exprname; \
	}

#define GET_PART_EXPR(exprname) { \
		return p ? p->exprname : NULL; \
	}

#define SET_PART_EXPR_OR_STR(exprname) { \
		if (!p || (p->o && p->o->executing)) \
			return NULL; \
		ocrpt_expr_free(p->exprname); \
		p->exprname = expr_string ? ocrpt_expr_parse(p->o, expr_string, NULL) : NULL; \
		if (!p->exprname) \
			p->exprname = expr_string ? ocrpt_newstring(p->o, NULL, expr_string) : NULL; \
		return p->exprname; \
	}

#define SET_PART_EXPR_REPORT(exprname) { \
		if (!p || (p->o && p->o->executing)) \
			return NULL; \
		ocrpt_expr_free(p->exprname); \
		char *err = NULL; \
		p->exprname = expr_string ? ocrpt_expr_parse(p->o, expr_string, &err) : NULL; \
		if (err) { \
			ocrpt_err_printf("%s: %s\n", __func__, err); \
			ocrpt_strfree(err); \
		} \
		return p->exprname; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_iterations(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR_REPORT(iterations_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_iterations(ocrpt_part *p) {
	GET_PART_EXPR(iterations_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_font_name(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR_OR_STR(font_name_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_font_name(ocrpt_part *p) {
	GET_PART_EXPR(font_name_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_font_size(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR(font_size_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_font_size(ocrpt_part *p) {
	GET_PART_EXPR(font_size_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_paper_type(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR_OR_STR(paper_type_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_paper_type(ocrpt_part *p) {
	GET_PART_EXPR(paper_type_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_orientation(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR(orientation_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_orientation(ocrpt_part *p) {
	GET_PART_EXPR(orientation_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_top_margin(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR(top_margin_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_top_margin(ocrpt_part *p) {
	GET_PART_EXPR(top_margin_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_bottom_margin(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR(bottom_margin_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_bottom_margin(ocrpt_part *p) {
	GET_PART_EXPR(bottom_margin_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_left_margin(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR(left_margin_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_left_margin(ocrpt_part *p) {
	GET_PART_EXPR(left_margin_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_right_margin(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR(right_margin_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_right_margin(ocrpt_part *p) {
	GET_PART_EXPR(right_margin_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_suppress(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR(suppress_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_suppress(ocrpt_part *p) {
	GET_PART_EXPR(suppress_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_set_suppress_pageheader_firstpage(ocrpt_part *p, const char *expr_string) {
	SET_PART_EXPR(suppress_pageheader_firstpage_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_get_suppress_pageheader_firstpage(ocrpt_part *p) {
	GET_PART_EXPR(suppress_pageheader_firstpage_expr);
}

#define SET_PART_ROW_EXPR(exprname) { \
		if (!pr || (pr->o && pr->o->executing)) \
			return NULL; \
		ocrpt_expr_free(pr->exprname); \
		pr->exprname = expr_string ? ocrpt_expr_parse(pr->o, expr_string, NULL) : NULL; \
		return pr->exprname; \
	}

#define GET_PART_ROW_EXPR(exprname) { \
		return pr ? pr->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_row_set_suppress(ocrpt_part_row *pr, const char *expr_string) {
	SET_PART_ROW_EXPR(suppress_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_row_get_suppress(ocrpt_part_row *pr) {
	GET_PART_ROW_EXPR(suppress_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_row_set_newpage(ocrpt_part_row *pr, const char *expr_string) {
	SET_PART_ROW_EXPR(newpage_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_row_get_newpage(ocrpt_part_row *pr) {
	GET_PART_ROW_EXPR(newpage_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_row_set_layout(ocrpt_part_row *pr, const char *expr_string) {
	SET_PART_ROW_EXPR(layout_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_row_get_layout(ocrpt_part_row *pr) {
	GET_PART_ROW_EXPR(layout_expr);
}

#define SET_PART_COLUMN_EXPR(exprname) { \
		if (!pd || (pd->o && pd->o->executing)) \
			return NULL; \
		ocrpt_expr_free(pd->exprname); \
		pd->exprname = expr_string ? ocrpt_expr_parse(pd->o, expr_string, NULL) : NULL; \
		return pd->exprname; \
	}

#define GET_PART_COLUMN_EXPR(exprname) { \
		return pd ? pd->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_set_suppress(ocrpt_part_column *pd, const char *expr_string) {
	SET_PART_COLUMN_EXPR(suppress_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_get_suppress(ocrpt_part_column *pd) {
	GET_PART_COLUMN_EXPR(suppress_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_set_width(ocrpt_part_column *pd, const char *expr_string) {
	SET_PART_COLUMN_EXPR(width_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_get_width(ocrpt_part_column *pd) {
	GET_PART_COLUMN_EXPR(width_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_set_height(ocrpt_part_column *pd, const char *expr_string) {
	SET_PART_COLUMN_EXPR(height_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_get_height(ocrpt_part_column *pd) {
	GET_PART_COLUMN_EXPR(height_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_set_border_width(ocrpt_part_column *pd, const char *expr_string) {
	SET_PART_COLUMN_EXPR(border_width_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_get_border_width(ocrpt_part_column *pd) {
	GET_PART_COLUMN_EXPR(border_width_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_set_border_color(ocrpt_part_column *pd, const char *expr_string) {
	SET_PART_COLUMN_EXPR(border_color_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_get_border_color(ocrpt_part_column *pd) {
	GET_PART_COLUMN_EXPR(border_color_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_set_detail_columns(ocrpt_part_column *pd, const char *expr_string) {
	SET_PART_COLUMN_EXPR(detail_columns_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_get_detail_columns(ocrpt_part_column *pd) {
	GET_PART_COLUMN_EXPR(detail_columns_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_set_column_padding(ocrpt_part_column *pd, const char *expr_string) {
	SET_PART_COLUMN_EXPR(column_pad_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_part_column_get_column_padding(ocrpt_part_column *pd) {
	GET_PART_COLUMN_EXPR(column_pad_expr);
}

#define SET_REPORT_EXPR(exprname) { \
		if (!r || (r->o && r->o->executing)) \
			return NULL; \
		ocrpt_expr_free(r->exprname); \
		r->exprname = expr_string ? ocrpt_expr_parse(r->o, expr_string, NULL) : NULL; \
		return r->exprname; \
	}

#define SET_REPORT_EXPR_OR_STR(exprname) { \
		if (!r || (r->o && r->o->executing)) \
			return NULL; \
		ocrpt_expr_free(r->exprname); \
		r->exprname = expr_string ? ocrpt_expr_parse(r->o, expr_string, NULL) : NULL; \
		if (!r->exprname) \
			r->exprname = expr_string ? ocrpt_newstring(r->o, r, expr_string) : NULL; \
		return r->exprname; \
	}

#define GET_REPORT_EXPR(exprname) { \
		return r ? r->exprname : NULL; \
	}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_set_suppress(ocrpt_report *r, const char *expr_string) {
	SET_REPORT_EXPR(suppress_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_get_suppress(ocrpt_report *r) {
	GET_REPORT_EXPR(suppress_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_set_iterations(ocrpt_report *r, const char *expr_string) {
	SET_REPORT_EXPR(iterations_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_get_iterations(ocrpt_report *r) {
	GET_REPORT_EXPR(iterations_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_set_font_name(ocrpt_report *r, const char *expr_string) {
	SET_REPORT_EXPR_OR_STR(font_name_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_get_font_name(ocrpt_report *r) {
	GET_REPORT_EXPR(font_name_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_set_font_size(ocrpt_report *r, const char *expr_string) {
	SET_REPORT_EXPR(font_size_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_get_font_size(ocrpt_report *r) {
	GET_REPORT_EXPR(font_size_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_set_height(ocrpt_report *r, const char *expr_string) {
	SET_REPORT_EXPR(height_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_get_height(ocrpt_report *r) {
	GET_REPORT_EXPR(height_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_set_fieldheader_priority(ocrpt_report *r, const char *expr_string) {
	SET_REPORT_EXPR(fieldheader_priority_expr);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_report_get_fieldheader_priority(ocrpt_report *r) {
	GET_REPORT_EXPR(fieldheader_priority_expr);
}

DLL_EXPORT_SYM ocrpt_output_element *ocrpt_output_element_get_next(ocrpt_output *output, ocrpt_list **list) {
	if (!output || !list)
		return NULL;

	*list = *list ? (*list)->next : output->output_list;
	return (ocrpt_output_element *)(*list ? (*list)->data : NULL);
}

DLL_EXPORT_SYM bool ocrpt_output_element_is_line(ocrpt_output_element *elem) {
	return (elem && elem->type == OCRPT_OUTPUT_LINE);
}

DLL_EXPORT_SYM bool ocrpt_output_element_is_hline(ocrpt_output_element *elem) {
	return (elem && elem->type == OCRPT_OUTPUT_HLINE);
}

DLL_EXPORT_SYM bool ocrpt_output_element_is_image(ocrpt_output_element *elem) {
	return (elem && elem->type == OCRPT_OUTPUT_IMAGE);
}

DLL_EXPORT_SYM bool ocrpt_output_element_is_barcode(ocrpt_output_element *elem) {
	return (elem && elem->type == OCRPT_OUTPUT_BARCODE);
}

DLL_EXPORT_SYM ocrpt_line_element *ocrpt_line_element_get_next(ocrpt_line *line, ocrpt_list **list) {
	if (!line || !list)
		return NULL;

	*list = *list ? (*list)->next : line->elements;
	return (ocrpt_line_element *)(*list ? (*list)->data : NULL);
}

DLL_EXPORT_SYM bool ocrpt_line_element_is_text(ocrpt_line_element *elem) {
	ocrpt_output_element *elem0 = (ocrpt_output_element *)elem;

	return (elem0 && elem0->le_type == OCRPT_OUTPUT_LE_TEXT);
}

DLL_EXPORT_SYM bool ocrpt_line_element_is_image(ocrpt_line_element *elem) {
	ocrpt_output_element *elem0 = (ocrpt_output_element *)elem;

	return (elem0 && elem0->le_type == OCRPT_OUTPUT_LE_IMAGE);
}

DLL_EXPORT_SYM bool ocrpt_line_element_is_barcode(ocrpt_line_element *elem) {
	ocrpt_output_element *elem0 = (ocrpt_output_element *)elem;

	return (elem0 && elem0->le_type == OCRPT_OUTPUT_LE_BARCODE);
}
