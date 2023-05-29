/*
 * OpenCReports XML output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <string.h>

#include "ocrpt-private.h"
#include "exprutil.h"
#include "parts.h"
#include "breaks.h"
#include "xml-output.h"

static void ocrpt_xml_start_part(opencreport *o, ocrpt_part *p) {
	xml_private_data *priv = o->output_private;

	priv->part = xmlNewDocNode(priv->doc, NULL, BAD_CAST "part", NULL);
	xmlAddChild(priv->toplevel, priv->part);

	priv->parttbl = xmlNewDocNode(priv->doc, NULL, BAD_CAST "table", NULL);
}

static void ocrpt_xml_end_part(opencreport *o, ocrpt_part *p) {
	xml_private_data *priv = o->output_private;

	if (priv->ph)
		xmlAddChild(priv->part, priv->ph);
	priv->ph = NULL;
	priv->pho = NULL;

	xmlAddChild(priv->part, priv->parttbl);
	priv->parttbl = NULL;

	if (priv->pf)
		xmlAddChild(priv->part, priv->pf);
	priv->pf = NULL;
	priv->pfo = NULL;
}

static void ocrpt_xml_start_part_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr) {
	xml_private_data *priv = o->output_private;

	priv->tr = xmlNewDocNode(priv->doc, NULL, BAD_CAST "tr", NULL);
	xmlAddChild(priv->parttbl, priv->tr);
}

static void ocrpt_xml_start_part_column(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd) {
	xml_private_data *priv = o->output_private;

	priv->td = xmlNewDocNode(priv->doc, NULL, BAD_CAST "td", NULL);
	xmlAddChild(priv->tr, priv->td);

	priv->tmp->len = 0;
	ocrpt_mem_string_append_printf(priv->tmp, "%.2lf", pd->real_width);
	xmlSetProp(priv->td, BAD_CAST "width", BAD_CAST priv->tmp->str);

	if (pd->height_expr) {
		priv->tmp->len = 0;
		ocrpt_mem_string_append_printf(priv->tmp, "%.2lf", pd->height);
		xmlSetProp(priv->td, BAD_CAST "width", BAD_CAST priv->tmp->str);
	}
}

static void ocrpt_xml_start_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r) {
	xml_private_data *priv = o->output_private;

	if (!o->xml_rlib_compat) {
		priv->r = xmlNewDocNode(priv->doc, NULL, BAD_CAST "report", NULL);
		xmlAddChild(priv->td, priv->r);
	}
}

static void ocrpt_xml_end_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r) {
	xml_private_data *priv = o->output_private;

	if (!o->xml_rlib_compat) {
		if (priv->rh)
			xmlAddChild(priv->r, priv->rh);
		priv->rh = NULL;
		priv->rho = NULL;

		for (ocrpt_list *l = priv->rl; l; l = l->next) {
			xmlNodePtr node = (xmlNodePtr)l->data;
			xmlAddChild(priv->r, node);
		}

		ocrpt_list_free(priv->rl);
		priv->rl = NULL;

		if (priv->rf)
			xmlAddChild(priv->r, priv->rf);
		priv->rf = NULL;
		priv->rfo = NULL;
	}
}

static void ocrpt_xml_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	xml_private_data *priv = o->output_private;

	if (o->precalculate) {
		if (!priv->current_page) {
			if (!priv->pages)
				priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, NULL);
			priv->current_page = priv->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, NULL);
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

static void *ocrpt_xml_get_current_page(opencreport *o) {
	xml_private_data *priv = o->output_private;

	return priv->current_page;
}

static void ocrpt_xml_set_current_page(opencreport *o, void *page) {
	xml_private_data *priv = o->output_private;

	priv->current_page = page;
}

static bool ocrpt_xml_is_current_page_first(opencreport *o) {
	xml_private_data *priv = o->output_private;

	return priv->current_page == priv->pages;
}

static void ocrpt_xml_start_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output) {
	xml_private_data *priv = o->output_private;

	/*
	 * Part's page header and page footer are out of band.
	 * They are created in ocrpt_xml_get_current_output() below.
	 */

	if (r) {
		if (output == &r->reportheader) {
			priv->rh = xmlNewDocNode(priv->doc, NULL, BAD_CAST "report_header", NULL);
			priv->rho = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
			priv->rho = xmlAddChild(priv->rh, priv->rho);
		}

		if (output == &r->reportfooter) {
			priv->rf = xmlNewDocNode(priv->doc, NULL, BAD_CAST "report_footer", NULL);
			priv->rfo = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
			priv->rfo = xmlAddChild(priv->rf, priv->rfo);
		}

		if (output == &r->fieldheader) {
			priv->fh = xmlNewDocNode(priv->doc, NULL, BAD_CAST "field_headers", NULL);
			priv->fho = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
			priv->fho = xmlAddChild(priv->fh, priv->fho);
			priv->rl = ocrpt_list_end_append(priv->rl, &priv->rl_last,  priv->fh);
		}

		if (output == &r->fielddetails) {
			priv->fd = xmlNewDocNode(priv->doc, NULL, BAD_CAST "field_details", NULL);
			priv->fdo = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
			priv->fdo = xmlAddChild(priv->fd, priv->fdo);
			priv->rl = ocrpt_list_end_append(priv->rl, &priv->rl_last,  priv->fd);
		}

		if (output == &r->nodata) {
			priv->nd = xmlNewDocNode(priv->doc, NULL, BAD_CAST "no_data", NULL);
			priv->ndo = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
			priv->ndo = xmlAddChild(priv->nd, priv->ndo);
			priv->rl = ocrpt_list_end_append(priv->rl, &priv->rl_last,  priv->nd);
		}
	}

	if (br) {
		if (output == &br->header) {
			priv->bh = xmlNewDocNode(priv->doc, NULL, BAD_CAST "break_header", NULL);
			priv->bho = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
			priv->bho = xmlAddChild(priv->bh, priv->bho);
			priv->rl = ocrpt_list_end_append(priv->rl, &priv->rl_last,  priv->bh);
			xmlSetProp(priv->bh, BAD_CAST "name", BAD_CAST br->name);
		}

		if (output == &br->footer) {
			priv->bf = xmlNewDocNode(priv->doc, NULL, BAD_CAST "break_footer", NULL);
			priv->bfo = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
			priv->bfo = xmlAddChild(priv->bf, priv->bfo);
			priv->rl = ocrpt_list_end_append(priv->rl, &priv->rl_last,  priv->bf);
			xmlSetProp(priv->bf, BAD_CAST "name", BAD_CAST br->name);
		}
	}
}

static xmlNodePtr ocrpt_xml_get_current_output(xml_private_data *priv, ocrpt_part *p, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output) {
	/* Part's page header and page footers must be create out of band */
	if (p) {
		if (output == &p->pageheader) {
			if (!priv->ph) {
				priv->ph = xmlNewDocNode(priv->doc, NULL, BAD_CAST "part_page_header", NULL);
				priv->pho = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
				priv->pho = xmlAddChild(priv->ph, priv->pho);
			}
			return priv->pho;
		}

		if (output == &p->pagefooter) {
			if (!priv->pf) {
				priv->pf = xmlNewDocNode(priv->doc, NULL, BAD_CAST "part_page_footer", NULL);
				priv->pfo = xmlNewDocNode(priv->doc, NULL, BAD_CAST "output", NULL);
				priv->pfo = xmlAddChild(priv->pf, priv->pfo);
			}
			return priv->pfo;
		}
	}

	if (r) {
		if (output == &r->reportheader)
			return priv->rho;

		if (output == &r->reportfooter)
			return priv->rfo;

		if (output == &r->fieldheader)
			return priv->fho;

		if (output == &r->fielddetails)
			return priv->fdo;

		if (output == &r->nodata)
			return priv->ndo;
	}

	if (br) {
		if (output == &br->header)
			return priv->bho;

		if (output == &br->footer)
			return priv->bfo;
	}

	return NULL;
}

static void ocrpt_xml_start_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, double page_indent, double y) {
	xml_private_data *priv = o->output_private;

	xmlNodePtr outn = ocrpt_xml_get_current_output(priv, p, r, br, output);

	if (outn) {
		priv->line = xmlNewDocNode(priv->doc, NULL, BAD_CAST "line", NULL);
		xmlAddChild(outn, priv->line);
	}
}

static inline unsigned char ocrpt_xml_color_value(double component) {
	if (component < 0.0)
		component = 0.0;
	if (component >= 1.0)
		component = 1.0;

	return (unsigned char)(0xff * component);
}

static void ocrpt_xml_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width) {
	xml_private_data *priv = o->output_private;

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

void ocrpt_xml_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double total_width) {
	xml_private_data *priv = o->output_private;
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

static void ocrpt_xml_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double page_width, double page_indent, double y) {
	xml_private_data *priv = o->output_private;

	ocrpt_xml_get_current_output(priv, p, r, br, output);
	if (le->value) {
		xmlNodePtr data = xmlNewTextChild(priv->line, NULL, BAD_CAST "data", BAD_CAST le->result_str->str);

		priv->tmp->len = 0;
		ocrpt_mem_string_append_printf(priv->tmp, "%.2lf", le->width_computed);
		xmlSetProp(data, BAD_CAST "width", BAD_CAST priv->tmp->str);

		priv->tmp->len = 0;
		ocrpt_mem_string_append_printf(priv->tmp, "%.2lf", le->fontsz);
		xmlSetProp(data, BAD_CAST "font_point", BAD_CAST priv->tmp->str);

		xmlSetProp(data, BAD_CAST "font_face", BAD_CAST le->font);

		priv->tmp->len = 0;
		ocrpt_mem_string_append_printf(priv->tmp, "%d", le->bold_val);
		xmlSetProp(data, BAD_CAST "bold", BAD_CAST priv->tmp->str);

		priv->tmp->len = 0;
		ocrpt_mem_string_append_printf(priv->tmp, "%d", le->italic_val);
		xmlSetProp(data, BAD_CAST "italics", BAD_CAST priv->tmp->str);

		const char *align = "left";
		if (le->align && le->align->result[o->residx] && le->align->result[o->residx]->type == OCRPT_RESULT_STRING && le->align->result[o->residx]->string) {
			const char *alignment = le->align->result[o->residx]->string->str;
			if (strcasecmp(alignment, "right") == 0)
				align = "right";
			else if (strcasecmp(alignment, "center") == 0)
				align = "center";
			else if (strcasecmp(alignment, "justified") == 0)
				align = "justified";
		}
		xmlSetProp(data, BAD_CAST "align", BAD_CAST align);

		ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };
		if (le->bgcolor && le->bgcolor->result[o->residx] && le->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && le->bgcolor->result[o->residx]->string)
			ocrpt_get_color(le->bgcolor->result[o->residx]->string->str, &bgcolor, true);
		else if (l->bgcolor && l->bgcolor->result[o->residx] && l->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && l->bgcolor->result[o->residx]->string)
			ocrpt_get_color(l->bgcolor->result[o->residx]->string->str, &bgcolor, true);
		priv->tmp->len = 0;
		ocrpt_mem_string_append_printf(priv->tmp, "#%02x%02x%02x", ocrpt_xml_color_value(bgcolor.r), ocrpt_xml_color_value(bgcolor.g), ocrpt_xml_color_value(bgcolor.b));
		xmlSetProp(data, BAD_CAST "bgcolor", BAD_CAST priv->tmp->str);

		ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
		if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
			ocrpt_get_color(le->color->result[o->residx]->string->str, &color, true);
		else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
			ocrpt_get_color(l->color->result[o->residx]->string->str, &color, true);
		priv->tmp->len = 0;
		ocrpt_mem_string_append_printf(priv->tmp, "#%02x%02x%02x", ocrpt_xml_color_value(color.r), ocrpt_xml_color_value(color.g), ocrpt_xml_color_value(color.b));
		xmlSetProp(data, BAD_CAST "color", BAD_CAST priv->tmp->str);
	}
}

static void ocrpt_xml_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, double page_width, double page_indent, double x, double y, double w, double h) {
	xml_private_data *priv = o->output_private;

	xmlNodePtr outn = ocrpt_xml_get_current_output(priv, p, r, br, output);

	if (outn) {
		xmlNodePtr imgn = xmlNewDocNode(priv->doc, NULL, BAD_CAST "image", NULL);

		if (line)
			xmlAddChild(priv->line, imgn);
		else
			xmlAddChild(outn, imgn);
	}
}

static void ocrpt_xml_finalize(opencreport *o) {
	xml_private_data *priv = o->output_private;

	xmlChar *xmlbuf;
	int bufsz;
	int old_xmlIndentTreeOutput = xmlIndentTreeOutput;

	xmlIndentTreeOutput = 1;
	xmlDocDumpFormatMemoryEnc(priv->doc, &xmlbuf, &bufsz, "utf-8", 1);

	xmlIndentTreeOutput = old_xmlIndentTreeOutput;

	ocrpt_mem_string_append_len(o->output_buffer, (char *)xmlbuf, bufsz);

	xmlFree(xmlbuf);
	xmlFreeDoc(priv->doc);

	ocrpt_mem_string_free(priv->tmp, true);
	cairo_destroy(priv->cr);
	cairo_surface_destroy(priv->nullpage_cs);
	ocrpt_list_free(priv->pages);
	ocrpt_mem_free(priv);
	o->output_private = NULL;

	ocrpt_string **content_type = ocrpt_mem_malloc(4 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: text/xml; charset=utf-8\n");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = ocrpt_mem_string_new_printf("Content-Disposition: attachment; filename=%s", o->csv_filename ? o->csv_filename : "report.xml");
	content_type[3] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_xml_init(opencreport *o) {
	memset(&o->output_functions, 0, sizeof(ocrpt_output_functions));
	o->output_functions.start_part = ocrpt_xml_start_part;
	o->output_functions.end_part = ocrpt_xml_end_part;
	o->output_functions.start_part_row = ocrpt_xml_start_part_row;
	o->output_functions.start_part_column = ocrpt_xml_start_part_column;
	o->output_functions.start_report = ocrpt_xml_start_report;
	o->output_functions.end_report = ocrpt_xml_end_report;
	o->output_functions.start_output = ocrpt_xml_start_output;
	o->output_functions.start_data_row = ocrpt_xml_start_data_row;
	o->output_functions.draw_image = ocrpt_xml_draw_image;
	o->output_functions.set_font_sizes = ocrpt_xml_set_font_sizes;
	o->output_functions.get_text_sizes = ocrpt_xml_get_text_sizes;
	o->output_functions.draw_text = ocrpt_xml_draw_text;
	o->output_functions.add_new_page = ocrpt_xml_add_new_page;
	o->output_functions.get_current_page = ocrpt_xml_get_current_page;
	o->output_functions.set_current_page = ocrpt_xml_set_current_page;
	o->output_functions.is_current_page_first = ocrpt_xml_is_current_page_first;
	o->output_functions.finalize = ocrpt_xml_finalize;

	o->output_private = ocrpt_mem_malloc(sizeof(xml_private_data));
	memset(o->output_private, 0, sizeof(xml_private_data));

	xml_private_data *priv = o->output_private;

	priv->doc = xmlNewDoc(NULL);
	priv->toplevel = xmlNewDocNode(priv->doc, NULL, BAD_CAST (o->xml_rlib_compat ? "rlib" : "ocrpt"), NULL);
	xmlDocSetRootElement(priv->doc, priv->toplevel);

	cairo_rectangle_t page = { .x = o->paper->width, .y = o->paper->height };
	priv->nullpage_cs = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &page);
	priv->cr = cairo_create(priv->nullpage_cs);
	priv->tmp = ocrpt_mem_string_new_with_len(NULL, 64);

	o->output_buffer = ocrpt_mem_string_new_with_len(NULL, 65536);
}
