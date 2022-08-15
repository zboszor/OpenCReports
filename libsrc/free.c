/*
 * OpenCReports memory freeing utilities
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdio.h>
#include <cairo.h>
#include <glib-object.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "exprutil.h"
#include "scanner.h"
#include "parts.h"

static const char *ocrpt_type_name(enum ocrpt_expr_type type) __attribute__((unused));
static const char *ocrpt_type_name(enum ocrpt_expr_type type) {
	switch (type) {
	case OCRPT_EXPR_ERROR:
		return "ERROR";
	case OCRPT_EXPR_NUMBER:
		return "NUMBER";
	case OCRPT_EXPR_STRING:
		return "STRING";
	case OCRPT_EXPR_DATETIME:
		return "DATETIME";
	case OCRPT_EXPR_MVAR:
		return "MVAR";
	case OCRPT_EXPR_RVAR:
		return "RVAR";
	case OCRPT_EXPR_VVAR:
		return "VVAR";
	case OCRPT_EXPR_IDENT:
		return "IDENT";
	case OCRPT_EXPR:
		return "EXPR";
	}
	return "UNKNOWN_TYPE";
}

void ocrpt_result_free_data(ocrpt_result *r) {
	if (!r)
		return;

	if (r->number_initialized)
		mpfr_clear(r->number);
	r->number_initialized = false;

	if (r->string) {
		ocrpt_mem_string_free(r->string, r->string_owned);
		r->string = NULL;
	}
}

DLL_EXPORT_SYM void ocrpt_result_free(ocrpt_result *r) {
	ocrpt_result_free_data(r);
	ocrpt_mem_free(r);
}

DLL_EXPORT_SYM void ocrpt_expr_free(opencreport *o, ocrpt_report *r, ocrpt_expr *e) {
	int i;

	if (!e)
		return;

	if (r && !r->executing) {
		r->exprs = ocrpt_list_remove(r->exprs, e);
		r->exprs_last = NULL;
	}

	switch (e->type) {
	case OCRPT_EXPR_NUMBER:
	case OCRPT_EXPR_ERROR:
	case OCRPT_EXPR_STRING:
	case OCRPT_EXPR_DATETIME:
		for (i = 0; i < OCRPT_EXPR_RESULTS; i++)
			if (ocrpt_expr_get_result_owned(o, e, i))
				ocrpt_result_free_data(e->result[i]);
		break;
	case OCRPT_EXPR_MVAR:
	case OCRPT_EXPR_RVAR:
	case OCRPT_EXPR_VVAR:
	case OCRPT_EXPR_IDENT:
		ocrpt_mem_string_free(e->query, true);
		ocrpt_mem_string_free(e->name, true);
		e->query = NULL;
		e->name = NULL;
		for (i = 0; i < OCRPT_EXPR_RESULTS; i++)
			if (ocrpt_expr_get_result_owned(o, e, i))
				ocrpt_result_free_data(e->result[i]);
		break;
	case OCRPT_EXPR:
		for (i = 0; i < OCRPT_EXPR_RESULTS; i++)
			if (ocrpt_expr_get_result_owned(o, e, i))
				ocrpt_result_free_data(e->result[i]);
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_free(o, r, e->ops[i]);
		ocrpt_mem_free(e->ops);
		break;
	}

	for (i = 0; i < OCRPT_EXPR_RESULTS; i++)
		if (ocrpt_expr_get_result_owned(o, e, i))
			ocrpt_mem_free(e->result[i]);
	ocrpt_result_free(e->delayed_result);
	ocrpt_mem_free(e);
}

void ocrpt_image_free(const void *ptr) {
	const ocrpt_image_file *image = (const ocrpt_image_file *)ptr;

	ocrpt_mem_free(image->name);
	cairo_surface_destroy((cairo_surface_t *)image->surface);
	if (image->rsvg)
		g_object_unref(image->rsvg);
	if (image->pixbuf)
		g_object_unref(image->pixbuf);
	ocrpt_mem_free(ptr);
}

void ocrpt_output_free(opencreport *o, ocrpt_report *r, ocrpt_output *output) {
	ocrpt_line *line;

	for (ocrpt_list *l = output->output_list; l; l = l->next) {
		ocrpt_output_element *output = (ocrpt_output_element *)l->data;
		switch (output->type) {
		case OCRPT_OUTPUT_LINE:
			line = (ocrpt_line *)output;
			for (ocrpt_list *l = line->elements; l; l = l->next) {
				ocrpt_line_element *le = (ocrpt_line_element *)l->data;
				ocrpt_mem_string_free(le->value_str, true);
				ocrpt_mem_free(l->data);
			}
			ocrpt_list_free(line->elements);
			break;
		case OCRPT_OUTPUT_HLINE:
		case OCRPT_OUTPUT_IMAGE:
			break;
		}

		ocrpt_mem_free(output);
	}

	ocrpt_list_free(output->output_list);
	output->output_list = NULL;
}
