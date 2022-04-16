/*
 * OpenCReports memory freeing utilities
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdio.h>

#include "exprutil.h"
#include "scanner.h"

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

DLL_EXPORT_SYM void ocrpt_result_free_data(ocrpt_result *r) {
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
