/*
 * OpenCReports HTML output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <utf8proc.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "exprutil.h"
#include "layout.h"
#include "parts.h"
#include "html-output.h"

static void ocrpt_html_start_part(opencreport *o, ocrpt_part *p) {
	ocrpt_mem_string_append(o->output_buffer, "<!--part start-->\n<table>\n");
}

static void ocrpt_html_end_part(opencreport *o, ocrpt_part *p) {
	ocrpt_mem_string_append(o->output_buffer, "<!--part end-->\n</table>\n");
}

static void ocrpt_html_start_part_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr) {
	ocrpt_mem_string_append(o->output_buffer, "<!--part row start-->\n<tr style=\"display: flex; align-items: flex-start; \">\n");
}

static void ocrpt_html_end_part_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr) {
	ocrpt_mem_string_append(o->output_buffer, "<!--part row end-->\n</tr>\n");
}

static void ocrpt_html_start_part_column(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd) {
	if (pd && (p->pageheader_printed || !p->pageheader.output_list)) {
		ocrpt_mem_string_append_printf(o->output_buffer,
										"<!--part column start-->\n<td style=\"width: %.2lfpt; text-overflow: clip; ",
										pd->real_width);
		if (pd->border_width_expr)
			ocrpt_mem_string_append_printf(o->output_buffer,
										"border: %.2lfpt solid #%02x%02x%02x; ",
										pd->border_width,
										ocrpt_common_color_value(pd->border_color.r), ocrpt_common_color_value(pd->border_color.g), ocrpt_common_color_value(pd->border_color.b));
		if (pd->height_expr)
			ocrpt_mem_string_append_printf(o->output_buffer,
										"height: %.2lfpt; overflow: clip; ",
										pd->remaining_height);

		ocrpt_mem_string_append(o->output_buffer, "\">\n");
	} else {
		ocrpt_mem_string_append_printf(o->output_buffer, "<!--part column start-->\n<td style=\"width: 100%%; \">\n");
	}
}

static void ocrpt_html_end_part_column(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd) {
	ocrpt_mem_string_append(o->output_buffer, "<!--part column end-->\n</td>\n");
}

static void ocrpt_html_start_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *out) {
	ocrpt_mem_string_append(o->output_buffer,
							"<!--output start--><section style=\"clear: both; ");
	if (pd)
		ocrpt_mem_string_append_printf(o->output_buffer,
										"width: %.2lfpt; ",
										pd->real_width);

	ocrpt_mem_string_append(o->output_buffer, "\">\n");
}

static void ocrpt_html_end_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *out) {
	html_private_data *priv = o->output_private;

	ocrpt_mem_string_append(o->output_buffer, "<!--output end--></section>\n");
	priv->image_indent = 0.0;
}

static void ocrpt_html_start_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, double page_indent, double y) {
	ocrpt_mem_string_append_printf(o->output_buffer,
										"<p style=\"line-height: %.2lfpt; \">",
										l->line_height);
}

static void ocrpt_html_end_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l) {
	ocrpt_mem_string_append(o->output_buffer, "</p>\n");
}

static void ocrpt_html_add_new_page_epilogue(opencreport *o) {
	ocrpt_mem_string_append(o->output_buffer, "<hr style=\"width: 100%%\">\n");
}

static void ocrpt_html_draw_hline(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_hline *hline, double page_width, double page_indent, double page_position, double size) {
	html_private_data *priv = o->output_private;
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

	if (hline->color && hline->color->result[o->residx] && hline->color->result[o->residx]->type == OCRPT_RESULT_STRING && hline->color->result[o->residx]->string)
		color_name = hline->color->result[o->residx]->string->str;

	ocrpt_get_color(color_name, &color, false);

	if (priv->base.current_page) {
		ocrpt_mem_string_append_printf(o->output_buffer,
										"<div style=\"display: block; "
										"margin-left: %.2lfpt; height: %.2lfpt; width: %.2lfpt; "
										"background-color: #%02x%02x%02x; "
										"\">&nbsp;</div>\n",
										indent + priv->image_indent, size, length,
										ocrpt_common_color_value(color.r), ocrpt_common_color_value(color.g), ocrpt_common_color_value(color.b));
	}
}

static inline void ocrpt_html_escape_data(opencreport *o, ocrpt_text *le) {
	html_private_data *priv = o->output_private;
	utf8proc_int32_t c;
	utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
	char cc[8];

	int startpos = le->pline ? pango_layout_line_get_start_index(le->pline) : 0;
	int length = le->pline ? pango_layout_line_get_length(le->pline) : 0;
	priv->base.data->len = 0;
	bytes_total = 0;

	while (bytes_total < (utf8proc_ssize_t)length) {
		bytes_read = utf8proc_iterate((utf8proc_uint8_t *)(le->result_str->str + startpos + bytes_total), length - bytes_total, &c);
		switch (c) {
		case ' ':
			ocrpt_mem_string_append(priv->base.data, "&nbsp;");
			break;
		case '"':
			ocrpt_mem_string_append(priv->base.data, "&quot;");
			break;
		case '&':
			ocrpt_mem_string_append(priv->base.data, "&amp;");
			break;
		case '<':
			ocrpt_mem_string_append(priv->base.data, "&lt;");
			break;
		case '>':
			ocrpt_mem_string_append(priv->base.data, "&gt;");
			break;
		default:
			bytes_written = utf8proc_encode_char(c, (utf8proc_uint8_t *)cc);
			ocrpt_mem_string_append_len(priv->base.data, cc, bytes_written);
		}
		bytes_total += bytes_read;
	}

	if (priv->base.data->len == 0)
		ocrpt_mem_string_append(priv->base.data, "&nbsp;");
}

static void ocrpt_html_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, bool last, double page_width, double page_indent, double y) {
	html_private_data *priv = o->output_private;

	if (pd && (le->start > pd->real_width))
		return;

	ocrpt_mem_string_append_printf(o->output_buffer,
									"<span style=\"line-height: %.2lfpt; ",
									l->line_height);
	if (le->width) {
		bool text_fits = (pd && (le->start + le->width_computed < pd->column_width)) || !pd;
		ocrpt_mem_string_append_printf(o->output_buffer,
									"width: %.2lfpt; ",
									(text_fits || !pd) ? le->width_computed : pd->column_width - le->start);
	} else if (ocrpt_list_length(l->elements) == 1) {
		/*
		 * This is (nr element == 1) is a best approximation
		 * for 
		 */
		ocrpt_mem_string_append_printf(o->output_buffer,
									"width: %.2lfpt; ",
									page_width);
	}

	switch (le->p_align) {
	case PANGO_ALIGN_RIGHT:
		ocrpt_mem_string_append_printf(o->output_buffer, "justify-content: end; text-align: end; ");
		break;
	case PANGO_ALIGN_CENTER:
		ocrpt_mem_string_append_printf(o->output_buffer, "justify-content: center; text-align: center; ");
		break;
	default:
		if (le->justified)
			ocrpt_mem_string_append_printf(o->output_buffer, "text-align: justify;  text-justify: inter-word; ");
		else
			ocrpt_mem_string_append_printf(o->output_buffer, "justify-content: start; text-align: start; ");
		break;
	}

	ocrpt_mem_string_append_printf(o->output_buffer, "font-family: '%s'; font-size: %.2lfpt; ", le->font, le->fontsz);

	ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };
	if (le->bgcolor && le->bgcolor->result[o->residx] && le->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && le->bgcolor->result[o->residx]->string)
		ocrpt_get_color(le->bgcolor->result[o->residx]->string->str, &bgcolor, true);
	else if (l->bgcolor && l->bgcolor->result[o->residx] && l->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && l->bgcolor->result[o->residx]->string)
		ocrpt_get_color(l->bgcolor->result[o->residx]->string->str, &bgcolor, true);

	ocrpt_mem_string_append_printf(o->output_buffer, "background-color: #%02x%02x%02x; ", ocrpt_common_color_value(bgcolor.r), ocrpt_common_color_value(bgcolor.g), ocrpt_common_color_value(bgcolor.b));

	ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
	if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
		ocrpt_get_color(le->color->result[o->residx]->string->str, &color, true);
	else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
		ocrpt_get_color(l->color->result[o->residx]->string->str, &color, true);

	ocrpt_mem_string_append_printf(o->output_buffer, "color: #%02x%02x%02x; ", ocrpt_common_color_value(color.r), ocrpt_common_color_value(color.g), ocrpt_common_color_value(color.b));

	if (le->bold_val)
		ocrpt_mem_string_append(o->output_buffer, "font-weight: bold; ");
	if (le->italic_val)
		ocrpt_mem_string_append(o->output_buffer, "font-style: italic; ");

	ocrpt_mem_string_append(o->output_buffer, "\">");

	ocrpt_html_escape_data(o, le);

	if (le->link && le->link->result[o->residx] && le->link->result[o->residx]->type == OCRPT_RESULT_STRING && le->link->result[o->residx]->string) {
		char *link = le->link->result[o->residx]->string->str;

		ocrpt_mem_string_append_printf(o->output_buffer, "<a href=\"%s\">%s</a>", link, priv->base.data->str);
	} else
		ocrpt_mem_string_append_printf(o->output_buffer, "%s", priv->base.data->str);

	ocrpt_mem_string_append(o->output_buffer, "</span>");

	if (l->current_line + 1 < le->lines) {
		le->pline = pango_layout_get_line(le->layout, l->current_line + 1);
		pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
	} else
		le->pline = NULL;
}

static const char *ocrpt_html_truncate_file_prefix(html_private_data *priv, const char *filename) {
	if (strncmp(filename, priv->cwd, priv->cwdlen) == 0)
		return filename + priv->cwdlen + 1;
	return filename;
}

static void ocrpt_html_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, bool last, double page_width, double page_indent, double x, double y, double w, double h) {
	html_private_data *priv = o->output_private;
	ocrpt_image_file *img_file = img->img_file;

	if (!img_file)
		return;

	if (line) {
		const char *al = "start";
		if (img->align && img->align->result[o->residx] && img->align->result[o->residx]->type == OCRPT_RESULT_STRING && img->align->result[o->residx]->string) {
			const char *alignment = img->align->result[o->residx]->string->str;
			if (strcasecmp(alignment, "right") == 0)
				al = "end";
			else if (strcasecmp(alignment, "center") == 0)
				al = "center";
		}

		ocrpt_mem_string_append_printf(o->output_buffer,
											"<span style=\"width: %.2lfpt; height: %.2lfpt; "
											"text-align: %s; "
											"align-items: %s; "
											"justify-content: %s; "
											"background-color: #%02x%02x%02x; \">",
											w, h, al, al, al,
											ocrpt_common_color_value(img->bg.r), ocrpt_common_color_value(img->bg.g), ocrpt_common_color_value(img->bg.b));

		if (!line->current_line) {
			if (img->img_file->rsvg)
				ocrpt_mem_string_append_printf(o->output_buffer,
												"<svg style=\"width: auto; height: %.2lfpt; \">"
												"<image width=\"auto\" height=\"%.2lfpt\" "
													"href=\"%s\"/>"
												"</svg>",
												line->fontsz,
												line->fontsz,
												ocrpt_html_truncate_file_prefix(priv, img->img_file->name));
			else
				ocrpt_mem_string_append_printf(o->output_buffer,
												"<img src=\"%s\" style=\"height: %.2lfpt; \">",
												ocrpt_html_truncate_file_prefix(priv, img->img_file->name),
												line->fontsz);
		} else
			ocrpt_mem_string_append(o->output_buffer, "&nbsp;");

		ocrpt_mem_string_append(o->output_buffer, "</span>");
	} else {
		ocrpt_mem_string_append(o->output_buffer,
											"<!--image closing section--></section>\n");
		ocrpt_mem_string_append_printf(o->output_buffer,
											"<!--image start section--><section style=\"clear: both; width: %.2lfpt; \">\n",
											pd->real_width);
		if (img->img_file->rsvg)
			ocrpt_mem_string_append_printf(o->output_buffer,
											"<svg style=\"float: left; width: %.2lfpt; height: %.2lfpt; \">"
											"<image width=\"%.2lfpt\" height=\"%.2lfpt\" preserveAspectRatio=\"none\" "
												"href=\"%s\"/>"
											"</svg>\n",
											w, h, w, h,
											ocrpt_html_truncate_file_prefix(priv, img->img_file->name));
		else
			ocrpt_mem_string_append_printf(o->output_buffer,
											"<img src=\"%s\" style=\"float: left; width: %.2lfpt; height: %.2lfpt; \" alt=\"background image\">\n",
											ocrpt_html_truncate_file_prefix(priv, img->img_file->name), w, h);

		priv->image_indent = w;
	}
}

static cairo_status_t ocrpt_html_write_png(void *closure, const unsigned char *data, unsigned int length) {
	html_private_data *priv = closure;

	ocrpt_mem_string_append_len_binary(priv->png, (const char *)data, length);

	return CAIRO_STATUS_SUCCESS;
}

static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void ocrpt_html_base64_encode(html_private_data *priv) {
	ocrpt_string *b64;
	unsigned char *pos;
	const unsigned char *end, *in;
	size_t olen;

	olen = priv->png->len * 4 / 3 + 4 + 1; /* 3-byte blocks to 4-byte + nul termination */
	if (olen < priv->png->len)
		return; /* integer overflow */

	b64 = ocrpt_mem_string_resize(priv->pngbase64, olen);
	if (b64) {
		if (!priv->pngbase64)
			priv->pngbase64 = b64;
		priv->pngbase64->len = 0;
	} else {
		if (priv->pngbase64)
			priv->pngbase64->len = 0;
		return;
	}

	end = (unsigned char *)priv->png->str + priv->png->len;
	in = (unsigned char *)priv->png->str;
	pos = (unsigned char *)priv->pngbase64->str;

	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}

	*pos = '\0';
	priv->pngbase64->len = (char *)pos - priv->pngbase64->str;
}

static void ocrpt_html_draw_barcode(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_barcode *bc, bool last, double page_width, double page_indent, double x, double y, double w, double h) {
	html_private_data *priv = o->output_private;
	int32_t first_space = bc->encoded->str[0] - '0';
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bc->encoded_width - first_space + 20.0, bc->barcode_height);
	cairo_t *cr = cairo_create(surface);

	ocrpt_color bg, fg;

	char *color_name = NULL;
	if (bc->color && bc->color->result[o->residx] && bc->color->result[o->residx]->type == OCRPT_RESULT_STRING && bc->color->result[o->residx]->string)
		color_name = bc->color->result[o->residx]->string->str;

	ocrpt_get_color(color_name, &fg, false);

	if (bc->encoded_width > 0) {
		color_name = NULL;
		if (bc->bgcolor && bc->bgcolor->result[o->residx] && bc->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && bc->bgcolor->result[o->residx]->string)
			color_name = bc->bgcolor->result[o->residx]->string->str;

		ocrpt_get_color(color_name, &bg, true);
	}

	cairo_save(cr);

	cairo_set_source_rgb(cr, bg.r, bg.g, bg.b);
	cairo_set_line_width(cr, 0.0);
	cairo_rectangle(cr, 0, 0, bc->barcode_width - 1, bc->barcode_height - 1);
	cairo_fill(cr);

	cairo_restore(cr);

	if (bc->encoded_width > 0 && (!line || !line->current_line)) {
		cairo_save(cr);

		int32_t pos = 10;

		for (int32_t bar = 1; bar < bc->encoded->len; bar++) {
			char e = bc->encoded->str[bar];

			/* special cases, ignore */
			if (e == '+' || e == '-')
				continue;

			int32_t width = isdigit(e) ? width = e - '0' : e - 'a' + 1;

			if (bar % 2) {
				cairo_set_source_rgb(cr, fg.r, fg.g, fg.b);
				cairo_set_line_width(cr, 0.0);
				cairo_rectangle(cr, pos, 0.0, (double)width, bc->barcode_height - 1);
				cairo_fill(cr);
			}

			pos += width;
		}

		cairo_restore(cr);
	}

	cairo_destroy(cr);

	priv->png->len = 0;
	cairo_surface_write_to_png_stream(surface, ocrpt_html_write_png, priv);

	ocrpt_html_base64_encode(priv);

	cairo_surface_destroy(surface);

	if (line) {
		ocrpt_mem_string_append_printf(o->output_buffer,
											"<span style=\"width: %.2lfpt; height: %.2lfpt; "
											"text-align: start; "
											"align-items: start; "
											"justify-content: start; \">",
											w, h);

		if (!line->current_line) {
			ocrpt_mem_string_append_printf(o->output_buffer,
											"<img src=\"data:image/png;base64,%s\" style=\"width: 100%%; height: 100%%; \">",
											priv->pngbase64->str);
		} else
			ocrpt_mem_string_append(o->output_buffer, "&nbsp;");

		ocrpt_mem_string_append(o->output_buffer, "</span>");
	} else {
		ocrpt_mem_string_append(o->output_buffer,
											"<!--image closing section--></section>\n");
		ocrpt_mem_string_append_printf(o->output_buffer,
											"<!--image start section--><section style=\"clear: both; width: %.2lfpt; \">\n",
											pd->real_width);
		ocrpt_mem_string_append_printf(o->output_buffer,
										"<img src=\"%s\" style=\"float: left; width: %.2lfpt; height: %.2lfpt; \" alt=\"background image\">\n",
										priv->pngbase64->str, w, h);

		priv->image_indent = w;
	}
}

static void ocrpt_html_draw_imageend(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output) {
	html_private_data *priv = o->output_private;

	ocrpt_mem_string_append(o->output_buffer,
										"<!--imageend closing section--></section>\n");
	ocrpt_mem_string_append_printf(o->output_buffer,
										"<!--imageend start section--><section style=\"clear: both; width: %.2lfpt; \">\n",
										pd->real_width);
	priv->image_indent = 0.0;
}

static void ocrpt_html_finalize(opencreport *o) {
	html_private_data *priv = o->output_private;

	ocrpt_mem_string_append(o->output_buffer, "</body></html>\n");

	ocrpt_mem_free(priv->cwd);
	ocrpt_mem_string_free(priv->png, true);
	ocrpt_mem_string_free(priv->pngbase64, true);

	ocrpt_common_finalize(o);

	ocrpt_string **content_type = ocrpt_mem_malloc(3 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: text/html; charset=utf-8\n");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_html_init(opencreport *o) {
	ocrpt_common_init(o, sizeof(html_private_data), 256, 65536);
	o->output_functions.start_part = ocrpt_html_start_part;
	o->output_functions.end_part = ocrpt_html_end_part;
	o->output_functions.start_part_row = ocrpt_html_start_part_row;
	o->output_functions.end_part_row = ocrpt_html_end_part_row;
	o->output_functions.start_part_column = ocrpt_html_start_part_column;
	o->output_functions.end_part_column = ocrpt_html_end_part_column;
	o->output_functions.start_data_row = ocrpt_html_start_data_row;
	o->output_functions.end_data_row = ocrpt_html_end_data_row;
	o->output_functions.start_output = ocrpt_html_start_output;
	o->output_functions.end_output = ocrpt_html_end_output;
	o->output_functions.add_new_page_epilogue = ocrpt_html_add_new_page_epilogue;
	o->output_functions.draw_hline = ocrpt_html_draw_hline;
	o->output_functions.set_font_sizes = ocrpt_common_set_font_sizes;
	o->output_functions.get_text_sizes = ocrpt_common_get_text_sizes;
	o->output_functions.draw_text = ocrpt_html_draw_text;
	o->output_functions.draw_image = ocrpt_html_draw_image;
	o->output_functions.draw_barcode = ocrpt_html_draw_barcode;
	o->output_functions.draw_imageend = ocrpt_html_draw_imageend;
	o->output_functions.finalize = ocrpt_html_finalize;
	o->output_functions.reopen_tags_across_pages = true;
	o->output_functions.support_any_font = true;
	o->output_functions.line_element_font = true;

	html_private_data *priv = o->output_private;

	if (o->html_docroot) {
		priv->cwd = ocrpt_mem_strdup(o->html_docroot);
		priv->cwdlen = strlen(priv->cwd);
	} else {
		priv->cwd = ocrpt_mem_malloc(PATH_MAX);
		if (!getcwd(priv->cwd, PATH_MAX)) {
			ocrpt_mem_free(priv->cwd);
			priv->cwd = NULL;
			priv->cwdlen = 0;
		} else
			priv->cwdlen = strlen(priv->cwd);
	}

	priv->png = ocrpt_mem_string_new_with_len(NULL, 1024);
	priv->pngbase64 = ocrpt_mem_string_new_with_len(NULL, 4096);

	ocrpt_mem_string_append(o->output_buffer, "<!DOCTYPE html>\n");
	ocrpt_mem_string_append(o->output_buffer, "<html lang=\"en\">\n");
	/*
	 * Output parameters that should be applied here:
	 * - suppress <head>
	 * - set font size for <head>
	 * - meta
	 */
	if (!o->suppress_html_head) {
		ocrpt_mem_string_append(o->output_buffer, "<head>\n");
		ocrpt_mem_string_append_printf(o->output_buffer, "<meta charset=\"utf-8\" %s>\n", o->html_meta ? o->html_meta : "");

		ocrpt_mem_string_append(o->output_buffer, "<link rel=\"preconnect\" href=\"//fonts.gstatic.com/\" >\n");
		ocrpt_mem_string_append(o->output_buffer, "<link rel=\"preconnect\" href=\"//fonts.gstatic.com/\" crossorigin >\n");
		/*
		 * TODO:
		 * Iterate over all outputs, collect every font and add them here.
		 */
		ocrpt_mem_string_append(o->output_buffer, "<link rel=\"stylesheet\" type=\"text/css\" href=\"//fonts.googleapis.com/css?family=Courier\" >\n");

		ocrpt_mem_string_append(o->output_buffer, "<style>\n");
		/* At least Courier font should work properly instead of Sans Serif */
		ocrpt_mem_string_append(o->output_buffer, "@import url('//fonts.googleapis.com/css?family=Courier'); \n");
		ocrpt_mem_string_append(o->output_buffer, "body { background-color: #ffffff; }\n");
		ocrpt_mem_string_append(o->output_buffer, "table { border: 0; border-spacing: 0; padding: 0; width:100%; }\n");
		ocrpt_mem_string_append(o->output_buffer, "p { margin: 0; padding: 0; }\n");
		ocrpt_mem_string_append(o->output_buffer, "span { display: inline-flex; flex-wrap: nowrap; white-space: nowrap; text-overflow: clip; overflow: hidden; vertical-align: middle; margin: 0; padding: 0; }\n");
		ocrpt_mem_string_append(o->output_buffer, "</style>\n");

		ocrpt_mem_string_append(o->output_buffer, "<title>OpenCReports report</title></head>\n");
	}
	ocrpt_mem_string_append(o->output_buffer, "<body>\n");
}
