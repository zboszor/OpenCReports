/*
 * OpenCReports HTML output driver
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
#include "html-output.h"

static inline unsigned char ocrpt_html_color_value(double component) {
	if (component < 0.0)
		component = 0.0;
	if (component >= 1.0)
		component = 1.0;

	return (unsigned char)(0xff * component);
}

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
										ocrpt_html_color_value(pd->border_color.r), ocrpt_html_color_value(pd->border_color.g), ocrpt_html_color_value(pd->border_color.b));
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
	ocrpt_mem_string_append_printf(o->output_buffer,
										"<!--output start--><section style=\"clear: both; width: %.2lfpt; \">\n",
										pd->real_width);
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

static void ocrpt_html_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	html_private_data *priv = o->output_private;

	if (o->precalculate) {
		if (!priv->base.current_page) {
			if (!priv->base.pages)
				priv->base.pages = ocrpt_list_end_append(priv->base.pages, &priv->base.last_page, NULL);
			priv->base.current_page = priv->base.pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			priv->base.pages = ocrpt_list_end_append(priv->base.pages, &priv->base.last_page, NULL);
			priv->base.current_page = priv->base.last_page;
		}

		if (mpfr_cmp(o->totpages->number, o->pageno->number) < 0)
			mpfr_set(o->totpages->number, o->pageno->number, o->rndmode);
	} else {
		if (!priv->base.current_page) {
			priv->base.current_page = priv->base.pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			priv->base.current_page = priv->base.current_page->next;
			ocrpt_mem_string_append(o->output_buffer, "<hr style=\"width: 100%%\">\n");
		}
	}
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
										ocrpt_html_color_value(color.r), ocrpt_html_color_value(color.g), ocrpt_html_color_value(color.b));
	}
}

void ocrpt_html_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double total_width) {
	html_private_data *priv = o->output_private;
	const char *font;
	double size, w;
	bool bold = false, italic = false, newfont = false, justified = false;
	PangoAlignment align;

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
			le->font = font;
			le->fontsz = size;
			le->bold_val = bold;
			le->italic_val = italic;
			le->p_align = align;
			le->justified = justified;
		}

		if (!le->layout) {
			le->layout = pango_cairo_create_layout(priv->base.cr);
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

		if (le->memo) {
			le->lines = pango_layout_get_line_count(le->layout);
			if (le->memo_max_lines && le->memo_max_lines < le->lines)
				le->lines = le->memo_max_lines;
		} else {
			le->lines = 1;
		}

		if (l->current_line < le->lines) {
			le->pline = pango_layout_get_line(le->layout, l->current_line);
			pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
		} else
			le->pline = NULL;
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

static void ocrpt_html_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double page_width, double page_indent, double y) {
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

	ocrpt_mem_string_append_printf(o->output_buffer, "background-color: #%02x%02x%02x; ", ocrpt_html_color_value(bgcolor.r), ocrpt_html_color_value(bgcolor.g), ocrpt_html_color_value(bgcolor.b));

	ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
	if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
		ocrpt_get_color(le->color->result[o->residx]->string->str, &color, true);
	else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
		ocrpt_get_color(l->color->result[o->residx]->string->str, &color, true);

	ocrpt_mem_string_append_printf(o->output_buffer, "color: #%02x%02x%02x; ", ocrpt_html_color_value(color.r), ocrpt_html_color_value(color.g), ocrpt_html_color_value(color.b));

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

static void ocrpt_html_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, double page_width, double page_indent, double x, double y, double w, double h) {
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
											ocrpt_html_color_value(img->bg.r), ocrpt_html_color_value(img->bg.g), ocrpt_html_color_value(img->bg.b));

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
	o->output_functions.add_new_page = ocrpt_html_add_new_page;
	o->output_functions.draw_hline = ocrpt_html_draw_hline;
	o->output_functions.set_font_sizes = ocrpt_common_set_font_sizes;
	o->output_functions.get_text_sizes = ocrpt_html_get_text_sizes;
	o->output_functions.draw_text = ocrpt_html_draw_text;
	o->output_functions.draw_image = ocrpt_html_draw_image;
	o->output_functions.draw_imageend = ocrpt_html_draw_imageend;
	o->output_functions.finalize = ocrpt_html_finalize;
	o->output_functions.reopen_tags_across_pages = true;

	html_private_data *priv = o->output_private;

	priv->cwd = ocrpt_mem_malloc(PATH_MAX);
	if (!getcwd(priv->cwd, PATH_MAX)) {
		ocrpt_mem_free(priv->cwd);
		priv->cwd = NULL;
		priv->cwdlen = 0;
	} else
		priv->cwdlen = strlen(priv->cwd);

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
