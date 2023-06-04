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

	priv->base.data->len = 0;
	ocrpt_mem_string_append_printf(priv->base.data, "%.2lf", pd->real_width);
	xmlSetProp(priv->td, BAD_CAST "width", BAD_CAST priv->base.data->str);

	if (pd->height_expr) {
		priv->base.data->len = 0;
		ocrpt_mem_string_append_printf(priv->base.data, "%.2lf", pd->height);
		xmlSetProp(priv->td, BAD_CAST "width", BAD_CAST priv->base.data->str);
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

static void ocrpt_xml_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, double page_width, double page_indent, double y) {
	xml_private_data *priv = o->output_private;

	ocrpt_xml_get_current_output(priv, p, r, br, output);
	if (le->value) {
		xmlNodePtr data = xmlNewTextChild(priv->line, NULL, BAD_CAST "data", BAD_CAST le->result_str->str);

		priv->base.data->len = 0;
		ocrpt_mem_string_append_printf(priv->base.data, "%.2lf", le->width_computed);
		xmlSetProp(data, BAD_CAST "width", BAD_CAST priv->base.data->str);

		priv->base.data->len = 0;
		ocrpt_mem_string_append_printf(priv->base.data, "%.2lf", le->fontsz);
		xmlSetProp(data, BAD_CAST "font_point", BAD_CAST priv->base.data->str);

		xmlSetProp(data, BAD_CAST "font_face", BAD_CAST le->font);

		priv->base.data->len = 0;
		ocrpt_mem_string_append_printf(priv->base.data, "%d", le->bold_val);
		xmlSetProp(data, BAD_CAST "bold", BAD_CAST priv->base.data->str);

		priv->base.data->len = 0;
		ocrpt_mem_string_append_printf(priv->base.data, "%d", le->italic_val);
		xmlSetProp(data, BAD_CAST "italics", BAD_CAST priv->base.data->str);

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
		priv->base.data->len = 0;
		ocrpt_mem_string_append_printf(priv->base.data, "#%02x%02x%02x", ocrpt_xml_color_value(bgcolor.r), ocrpt_xml_color_value(bgcolor.g), ocrpt_xml_color_value(bgcolor.b));
		xmlSetProp(data, BAD_CAST "bgcolor", BAD_CAST priv->base.data->str);

		ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
		if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
			ocrpt_get_color(le->color->result[o->residx]->string->str, &color, true);
		else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
			ocrpt_get_color(l->color->result[o->residx]->string->str, &color, true);
		priv->base.data->len = 0;
		ocrpt_mem_string_append_printf(priv->base.data, "#%02x%02x%02x", ocrpt_xml_color_value(color.r), ocrpt_xml_color_value(color.g), ocrpt_xml_color_value(color.b));
		xmlSetProp(data, BAD_CAST "color", BAD_CAST priv->base.data->str);
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

	ocrpt_common_finalize(o);

	ocrpt_string **content_type = ocrpt_mem_malloc(4 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: text/xml; charset=utf-8\n");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = ocrpt_mem_string_new_printf("Content-Disposition: attachment; filename=%s", o->csv_filename ? o->csv_filename : "report.xml");
	content_type[3] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_xml_init(opencreport *o) {
	ocrpt_common_init(o, sizeof(xml_private_data), 4096, 65536);
	o->output_functions.start_part = ocrpt_xml_start_part;
	o->output_functions.end_part = ocrpt_xml_end_part;
	o->output_functions.start_part_row = ocrpt_xml_start_part_row;
	o->output_functions.start_part_column = ocrpt_xml_start_part_column;
	o->output_functions.start_report = ocrpt_xml_start_report;
	o->output_functions.end_report = ocrpt_xml_end_report;
	o->output_functions.start_output = ocrpt_xml_start_output;
	o->output_functions.start_data_row = ocrpt_xml_start_data_row;
	o->output_functions.draw_image = ocrpt_xml_draw_image;
	o->output_functions.set_font_sizes = ocrpt_common_set_font_sizes;
	o->output_functions.get_text_sizes = ocrpt_common_get_text_sizes;
	o->output_functions.draw_text = ocrpt_xml_draw_text;
	o->output_functions.finalize = ocrpt_xml_finalize;

	xml_private_data *priv = o->output_private;

	priv->doc = xmlNewDoc(NULL);
	priv->toplevel = xmlNewDocNode(priv->doc, NULL, BAD_CAST (o->xml_rlib_compat ? "rlib" : "ocrpt"), NULL);
	xmlDocSetRootElement(priv->doc, priv->toplevel);
}
