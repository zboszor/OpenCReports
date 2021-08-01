/*
 * OpenCReports memory freeing utilities
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
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

static void ocrpt_expr_result_free(ocrpt_result *r, enum ocrpt_expr_type type) {
	if (!r)
		return;

	switch (type) {
	case OCRPT_EXPR_NUMBER:
		if (r && r->number_initialized)
			mpfr_clear(r->number);
		r->number_initialized = false;
		/* fall through */
	case OCRPT_EXPR_ERROR:
	case OCRPT_EXPR_STRING:
	case OCRPT_EXPR_DATETIME:
		if (r) {
			ocrpt_mem_string_free(r->string, r->string_owned);
			r->string = NULL;
		}
		break;
	default:
		break;
	}
}

DLL_EXPORT_SYM void ocrpt_result_free(ocrpt_result *r) {
	ocrpt_expr_result_free(r, r->type);
	ocrpt_mem_free(r);
}

DLL_EXPORT_SYM void ocrpt_expr_free(ocrpt_expr *e) {
	int i;

	if (!e)
		return;

	switch (e->type) {
	case OCRPT_EXPR_NUMBER:
	case OCRPT_EXPR_ERROR:
	case OCRPT_EXPR_STRING:
	case OCRPT_EXPR_DATETIME:
		if (e->result_owned0)
			ocrpt_expr_result_free(e->result[0], e->type);
		if (e->result_owned1)
			ocrpt_expr_result_free(e->result[1], e->type);
		break;
	case OCRPT_EXPR_MVAR:
	case OCRPT_EXPR_RVAR:
	case OCRPT_EXPR_VVAR:
	case OCRPT_EXPR_IDENT:
		ocrpt_mem_string_free(e->query, true);
		ocrpt_mem_string_free(e->name, true);
		e->query = NULL;
		e->name = NULL;
		if (e->result_owned0)
			ocrpt_expr_result_free(e->result[0], e->result[0]->type);
		if (e->result_owned1)
			ocrpt_expr_result_free(e->result[1], e->result[1]->type);
		break;
	case OCRPT_EXPR:
		if (e->result_owned0)
			ocrpt_expr_result_free(e->result[0], e->result[0]->type);
		if (e->result_owned1)
			ocrpt_expr_result_free(e->result[1], e->result[1]->type);
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_free(e->ops[i]);
		ocrpt_mem_free(e->ops);
		break;
	}

	if (e->result_owned0)
		ocrpt_mem_free(e->result[0]);
	if (e->result_owned1)
		ocrpt_mem_free(e->result[1]);
	ocrpt_mem_free(e);
}
