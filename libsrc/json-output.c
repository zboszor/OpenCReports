/*
 * OpenCReports JSON output driver
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <unistd.h>

#include "ocrpt-private.h"
#include "memutil.h"
#include "exprutil.h"
#include "parts.h"
#include "breaks.h"
#include "json-output.h"

typedef unsigned char *ystr;

static void ocrpt_json_start_part(opencreport *o, ocrpt_part *p) {
	json_private_data *priv = o->output_private;

	yajl_gen_string(priv->yajl_gen, (ystr)"part", 4);
	yajl_gen_map_open(priv->yajl_gen);
}

static void ocrpt_json_end_part(opencreport *o, ocrpt_part *p) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_close(priv->yajl_gen);
}

static void ocrpt_json_start_part_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr) {
	json_private_data *priv = o->output_private;
	json_backlog_data *bld = ocrpt_mem_malloc(sizeof(json_backlog_data));

	memset(bld, 0, sizeof(json_backlog_data));
	bld->type = part_row;
	bld->o = o;
	bld->p = p;
	bld->pr = pr;

	priv->backlog = ocrpt_list_append(priv->backlog, bld);
}

static void ocrpt_json_end_part_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_close(priv->yajl_gen);
}

static void ocrpt_json_start_part_column(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd) {
	json_private_data *priv = o->output_private;
	json_backlog_data *bld = ocrpt_mem_malloc(sizeof(json_backlog_data));

	memset(bld, 0, sizeof(json_backlog_data));
	bld->type = part_column;
	bld->o = o;
	bld->p = p;
	bld->pr = pr;
	bld->pd = pd;

	priv->backlog = ocrpt_list_append(priv->backlog, bld);
}

static void ocrpt_json_end_part_column(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_close(priv->yajl_gen);
}

static void ocrpt_json_start_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r) {
	json_private_data *priv = o->output_private;
	json_backlog_data *bld = ocrpt_mem_malloc(sizeof(json_backlog_data));

	memset(bld, 0, sizeof(json_backlog_data));
	bld->type = part_report;
	bld->o = o;
	bld->p = p;
	bld->pr = pr;
	bld->pd = pd;
	bld->r = r;

	priv->backlog = ocrpt_list_append(priv->backlog, bld);
}

static void ocrpt_json_end_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_close(priv->yajl_gen);
}

static void ocrpt_json_emit_backlog(json_private_data *priv) {
	for (ocrpt_list *l = priv->backlog; l; l = l->next) {
		json_backlog_data *bld = (json_backlog_data *)l->data;

		switch (bld->type) {
		case part_row:
			yajl_gen_string(priv->yajl_gen, (ystr)"pr", 2);
			yajl_gen_map_open(priv->yajl_gen);
			break;
		case part_column:
			yajl_gen_string(priv->yajl_gen, (ystr)"pd", 2);
			yajl_gen_map_open(priv->yajl_gen);
			break;
		case part_report:
			yajl_gen_string(priv->yajl_gen, (ystr)"report", 6);
			yajl_gen_map_open(priv->yajl_gen);
			break;
		}
	}

	ocrpt_list_free_deep(priv->backlog, ocrpt_mem_free);
	priv->backlog = NULL;
}

static void ocrpt_json_start_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *out) {
	json_private_data *priv = o->output_private;

	if (out != &p->pageheader)
		ocrpt_json_emit_backlog(priv);

	if (out == &p->pageheader) {
		yajl_gen_string(priv->yajl_gen, (ystr)"pageheader", 10);
	} else if (out == &p->pagefooter) {
		yajl_gen_string(priv->yajl_gen, (ystr)"pagefooter", 10);
	} if (r && out == &r->nodata) {
		yajl_gen_string(priv->yajl_gen, (ystr)"nodata", 6);
	} if (r && out == &r->reportheader) {
		yajl_gen_string(priv->yajl_gen, (ystr)"reportheader", 12);
	} if (r && out == &r->reportfooter) {
		yajl_gen_string(priv->yajl_gen, (ystr)"reportfooter", 12);
	} if (r && out == &r->fieldheader) {
		yajl_gen_string(priv->yajl_gen, (ystr)"fieldheader", 11);
	} if (r && out == &r->fielddetails) {
		yajl_gen_string(priv->yajl_gen, (ystr)"fielddetails", 12);
	} else if (br && (out == &br->header || out == &br->footer)) {
		yajl_gen_string(priv->yajl_gen, (ystr)"break_header", 12);
		yajl_gen_map_open(priv->yajl_gen);
		yajl_gen_string(priv->yajl_gen, (ystr)"break_name", 10);
		yajl_gen_string(priv->yajl_gen, (ystr)br->name, strlen(br->name));
	}

	yajl_gen_array_open(priv->yajl_gen);
}

static void ocrpt_json_end_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *out) {
	json_private_data *priv = o->output_private;

	yajl_gen_array_close(priv->yajl_gen);
	if (br && (out == &br->header || out == &br->footer))
		yajl_gen_map_close(priv->yajl_gen);

	if (out == &p->pageheader)
		ocrpt_json_emit_backlog(priv);

	priv->image_indent = 0.0;
}

static void ocrpt_json_start_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, double page_indent, double y) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_open(priv->yajl_gen);

	yajl_gen_string(priv->yajl_gen, (ystr)"type", 4);
	yajl_gen_string(priv->yajl_gen, (ystr)"line", 4);

	yajl_gen_string(priv->yajl_gen, (ystr)"elements", 8);
	yajl_gen_array_open(priv->yajl_gen);
}

static void ocrpt_json_end_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l) {
	json_private_data *priv = o->output_private;

	yajl_gen_array_close(priv->yajl_gen);
	yajl_gen_map_close(priv->yajl_gen);
}

static void ocrpt_json_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, bool last, double page_width, double page_indent, double y) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_open(priv->yajl_gen);

	yajl_gen_string(priv->yajl_gen, (ystr)"type", 4);
	yajl_gen_string(priv->yajl_gen, (ystr)"text", 4);

	yajl_gen_string(priv->yajl_gen, (ystr)"width", 5);
	yajl_gen_double(priv->yajl_gen, le->width_computed);

	yajl_gen_string(priv->yajl_gen, (ystr)"font_point", 10);
	yajl_gen_double(priv->yajl_gen, le->fontsz);

	yajl_gen_string(priv->yajl_gen, (ystr)"font_face", 9);
	yajl_gen_string(priv->yajl_gen, (ystr)le->font, strlen(le->font));

	yajl_gen_string(priv->yajl_gen, (ystr)"bold", 4);
	yajl_gen_bool(priv->yajl_gen, le->bold_val);

	yajl_gen_string(priv->yajl_gen, (ystr)"italic", 6);
	yajl_gen_bool(priv->yajl_gen, le->italic_val);

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
	yajl_gen_string(priv->yajl_gen, (ystr)"align", 5);
	yajl_gen_string(priv->yajl_gen, (ystr)align, strlen(align));

	ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };
	if (le->bgcolor && le->bgcolor->result[o->residx] && le->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && le->bgcolor->result[o->residx]->string)
		ocrpt_get_color(le->bgcolor->result[o->residx]->string->str, &bgcolor, true);
	else if (l->bgcolor && l->bgcolor->result[o->residx] && l->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && l->bgcolor->result[o->residx]->string)
		ocrpt_get_color(l->bgcolor->result[o->residx]->string->str, &bgcolor, true);

	priv->base.data->len = 0;
	ocrpt_mem_string_append_printf(priv->base.data, "#%02x%02x%02x", ocrpt_common_color_value(bgcolor.r), ocrpt_common_color_value(bgcolor.g), ocrpt_common_color_value(bgcolor.b));
	yajl_gen_string(priv->yajl_gen, (ystr)"bgcolor", 7);
	yajl_gen_string(priv->yajl_gen, (ystr)priv->base.data->str, priv->base.data->len);

	ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
	if (le->color && le->color->result[o->residx] && le->color->result[o->residx]->type == OCRPT_RESULT_STRING && le->color->result[o->residx]->string)
		ocrpt_get_color(le->color->result[o->residx]->string->str, &color, true);
	else if (l->color && l->color->result[o->residx] && l->color->result[o->residx]->type == OCRPT_RESULT_STRING && l->color->result[o->residx]->string)
		ocrpt_get_color(l->color->result[o->residx]->string->str, &color, true);
	priv->base.data->len = 0;
	ocrpt_mem_string_append_printf(priv->base.data, "#%02x%02x%02x", ocrpt_common_color_value(color.r), ocrpt_common_color_value(color.g), ocrpt_common_color_value(color.b));
	yajl_gen_string(priv->yajl_gen, (ystr)"color", 5);
	yajl_gen_string(priv->yajl_gen, (ystr)priv->base.data->str, priv->base.data->len);

	yajl_gen_string(priv->yajl_gen, (ystr)"data", 4);
	yajl_gen_string(priv->yajl_gen, (ystr)le->result_str->str, le->result_str->len);

	yajl_gen_map_close(priv->yajl_gen);
}

static const char *ocrpt_json_truncate_file_prefix(json_private_data *priv, const char *filename) {
	if (strncmp(filename, priv->cwd, priv->cwdlen) == 0)
		return filename + priv->cwdlen + 1;
	return filename;
}

static void ocrpt_json_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, bool last, double page_width, double page_indent, double x, double y, double w, double h) {
	json_private_data *priv = o->output_private;

	if (line) {
		yajl_gen_map_open(priv->yajl_gen);

		yajl_gen_string(priv->yajl_gen, (ystr)"type", 4);
		yajl_gen_string(priv->yajl_gen, (ystr)"image", 5);

		yajl_gen_string(priv->yajl_gen, (ystr)"width", 5);
		yajl_gen_double(priv->yajl_gen, w);

		yajl_gen_string(priv->yajl_gen, (ystr)"file", 4);
		const char *fname = ocrpt_json_truncate_file_prefix(priv, img->img_file->name);
		yajl_gen_string(priv->yajl_gen, (ystr)fname, strlen(fname));

		yajl_gen_map_close(priv->yajl_gen);
	} else {
		yajl_gen_map_open(priv->yajl_gen);

		yajl_gen_string(priv->yajl_gen, (ystr)"type", 4);
		yajl_gen_string(priv->yajl_gen, (ystr)"image", 5);

		yajl_gen_string(priv->yajl_gen, (ystr)"width", 5);
		yajl_gen_double(priv->yajl_gen, w);

		yajl_gen_string(priv->yajl_gen, (ystr)"height", 6);
		yajl_gen_double(priv->yajl_gen, h);

		yajl_gen_string(priv->yajl_gen, (ystr)"file", 4);
		const char *fname = ocrpt_json_truncate_file_prefix(priv, img->img_file->name);
		yajl_gen_string(priv->yajl_gen, (ystr)fname, strlen(fname));

		yajl_gen_map_close(priv->yajl_gen);

		priv->image_indent = w;
	}
}

static void ocrpt_json_draw_barcode(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_barcode *bc, bool last, double page_width, double page_indent, double x, double y, double w, double h) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_open(priv->yajl_gen);

	yajl_gen_string(priv->yajl_gen, (ystr)"type", 4);
	yajl_gen_string(priv->yajl_gen, (ystr)"barcode", 7);

	yajl_gen_string(priv->yajl_gen, (ystr)"width", 5);
	yajl_gen_double(priv->yajl_gen, bc->barcode_width);

	yajl_gen_string(priv->yajl_gen, (ystr)"height", 6);
	yajl_gen_double(priv->yajl_gen, bc->barcode_height);

	ocrpt_color bgcolor = { .r = 1.0, .g = 1.0, .b = 1.0 };
	if (bc->bgcolor && bc->bgcolor->result[o->residx] && bc->bgcolor->result[o->residx]->type == OCRPT_RESULT_STRING && bc->bgcolor->result[o->residx]->string)
		ocrpt_get_color(bc->bgcolor->result[o->residx]->string->str, &bgcolor, true);

	priv->base.data->len = 0;
	ocrpt_mem_string_append_printf(priv->base.data, "#%02x%02x%02x", ocrpt_common_color_value(bgcolor.r), ocrpt_common_color_value(bgcolor.g), ocrpt_common_color_value(bgcolor.b));
	yajl_gen_string(priv->yajl_gen, (ystr)"bgcolor", 7);
	yajl_gen_string(priv->yajl_gen, (ystr)priv->base.data->str, priv->base.data->len);

	ocrpt_color color = { .r = 0.0, .g = 0.0, .b = 0.0 };
	if (bc->color && bc->color->result[o->residx] && bc->color->result[o->residx]->type == OCRPT_RESULT_STRING && bc->color->result[o->residx]->string)
		ocrpt_get_color(bc->color->result[o->residx]->string->str, &color, true);
	priv->base.data->len = 0;
	ocrpt_mem_string_append_printf(priv->base.data, "#%02x%02x%02x", ocrpt_common_color_value(color.r), ocrpt_common_color_value(color.g), ocrpt_common_color_value(color.b));
	yajl_gen_string(priv->yajl_gen, (ystr)"color", 5);
	yajl_gen_string(priv->yajl_gen, (ystr)priv->base.data->str, priv->base.data->len);

	yajl_gen_string(priv->yajl_gen, (ystr)"value", 5);
	if (bc->value && bc->value->result[o->residx] && bc->value->result[o->residx]->type == OCRPT_RESULT_STRING)
		yajl_gen_string(priv->yajl_gen, (ystr)bc->value->result[o->residx]->string->str, bc->value->result[o->residx]->string->len);
	else
		yajl_gen_string(priv->yajl_gen, (ystr)"", 0);

	yajl_gen_map_close(priv->yajl_gen);

	if (!line)
		priv->image_indent = w;
}

static void ocrpt_json_draw_imageend(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *out) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_open(priv->yajl_gen);

	yajl_gen_string(priv->yajl_gen, (ystr)"type", 4);
	yajl_gen_string(priv->yajl_gen, (ystr)"imageend", 8);

	yajl_gen_map_close(priv->yajl_gen);

	priv->image_indent = 0.0;
}

static void ocrpt_json_draw_hline(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_hline *hline, double page_width, double page_indent, double page_position, double size) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_open(priv->yajl_gen);

	yajl_gen_string(priv->yajl_gen, (ystr)"type", 4);
	yajl_gen_string(priv->yajl_gen, (ystr)"horizontal_line", 15);

	double indent, length;
	char *align = NULL;
	int align_len = 0;

	if (hline->align && hline->align->result[o->residx] && hline->align->result[o->residx]->type == OCRPT_RESULT_STRING && hline->align->result[o->residx]->string) {
		const char *alignment = hline->align->result[o->residx]->string->str;

		if (strcasecmp(alignment, "right") == 0) {
			align = "right";
			align_len = 5;
		} else if (strcasecmp(alignment, "center") == 0) {
			align = "center";
			align_len = 6;
		}
	}

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

	priv->base.data->len = 0;
	ocrpt_mem_string_append_printf(priv->base.data, "#%02x%02x%02x", ocrpt_common_color_value(color.r), ocrpt_common_color_value(color.g), ocrpt_common_color_value(color.b));

	yajl_gen_string(priv->yajl_gen, (ystr)"color", 5);
	yajl_gen_string(priv->yajl_gen, (ystr)priv->base.data->str, priv->base.data->len);

	if (align) {
		yajl_gen_string(priv->yajl_gen, (ystr)"align", 5);
		yajl_gen_string(priv->yajl_gen, (ystr)align, align_len);
	}

	yajl_gen_string(priv->yajl_gen, (ystr)"indent", 6);
	yajl_gen_double(priv->yajl_gen, indent + priv->image_indent);

	yajl_gen_string(priv->yajl_gen, (ystr)"size", 4);
	yajl_gen_double(priv->yajl_gen, size);

	yajl_gen_string(priv->yajl_gen, (ystr)"length", 6);
	yajl_gen_double(priv->yajl_gen, length);

	yajl_gen_map_close(priv->yajl_gen);
}

static void ocrpt_json_write(void *ctx, const char *str, size_t len) {
	opencreport *o = ctx;

	if (o->output_buffer->len + len > o->output_buffer->allocated_len)
		ocrpt_mem_string_resize(o->output_buffer, o->output_buffer->allocated_len * 2);

	ocrpt_mem_string_append_len(o->output_buffer, str, len);
}

static void ocrpt_json_finalize(opencreport *o) {
	json_private_data *priv = o->output_private;

	yajl_gen_map_close(priv->yajl_gen);
	yajl_gen_free(priv->yajl_gen);

	ocrpt_mem_free(priv->cwd);

	ocrpt_common_finalize(o);

	ocrpt_string **content_type = ocrpt_mem_malloc(4 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: text/json; charset=utf-8\n");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = ocrpt_mem_string_new_printf("Content-Disposition: attachment; filename=%s", o->csv_filename ? o->csv_filename : "report.json");
	content_type[3] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_json_init(opencreport *o) {
	ocrpt_common_init(o, sizeof(json_private_data), 4096, 65536);
	o->output_functions.start_part = ocrpt_json_start_part;
	o->output_functions.end_part = ocrpt_json_end_part;
	o->output_functions.start_part_row = ocrpt_json_start_part_row;
	o->output_functions.end_part_row = ocrpt_json_end_part_row;
	o->output_functions.start_part_column = ocrpt_json_start_part_column;
	o->output_functions.end_part_column = ocrpt_json_end_part_column;
	o->output_functions.start_report = ocrpt_json_start_report;
	o->output_functions.end_report = ocrpt_json_end_report;
	o->output_functions.start_output = ocrpt_json_start_output;
	o->output_functions.end_output = ocrpt_json_end_output;
	o->output_functions.start_data_row = ocrpt_json_start_data_row;
	o->output_functions.end_data_row = ocrpt_json_end_data_row;
	o->output_functions.draw_image = ocrpt_json_draw_image;
	o->output_functions.draw_barcode = ocrpt_json_draw_barcode;
	o->output_functions.draw_imageend = ocrpt_json_draw_imageend;
	o->output_functions.set_font_sizes = ocrpt_common_set_font_sizes;
	o->output_functions.get_text_sizes = ocrpt_common_get_text_sizes;
	o->output_functions.draw_text = ocrpt_json_draw_text;
	o->output_functions.draw_hline = ocrpt_json_draw_hline;
	o->output_functions.finalize = ocrpt_json_finalize;
	o->output_functions.support_any_font = true;
	o->output_functions.line_element_font = true;

	json_private_data *priv = (json_private_data *)o->output_private;

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

	priv->yajl_gen = yajl_gen_alloc(&ocrpt_yajl_alloc_funcs);
	yajl_gen_config(priv->yajl_gen, yajl_gen_beautify, 1);
	yajl_gen_config(priv->yajl_gen, yajl_gen_print_callback, ocrpt_json_write, o);

	yajl_gen_map_open(priv->yajl_gen);
}
