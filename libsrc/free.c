/*
 * OpenCReports memory freeing utilities
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdio.h>

#include "exprutil.h"
#include "memutil.h"
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

static void ocrpt_free_expr_result(ocrpt_result *r, enum ocrpt_expr_type type) {
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
			if (r->string_owned)
				ocrpt_strfree(r->string);
			r->string = NULL;
		}
		break;
	default:
		break;
	}
}

DLL_EXPORT_SYM void ocrpt_free_expr(ocrpt_expr *e) {
	int i;

	if (!e)
		return;

	switch (e->type) {
	case OCRPT_EXPR_NUMBER:
	case OCRPT_EXPR_ERROR:
	case OCRPT_EXPR_STRING:
	case OCRPT_EXPR_DATETIME:
		ocrpt_free_expr_result(e->result, e->type);
		break;
	case OCRPT_EXPR_MVAR:
	case OCRPT_EXPR_RVAR:
	case OCRPT_EXPR_VVAR:
	case OCRPT_EXPR_IDENT:
		ocrpt_strfree(e->query);
		ocrpt_strfree(e->name);
		e->query = NULL;
		e->name = NULL;
		if (e->result_owned && e->result)
			ocrpt_free_expr_result(e->result, e->result->type);
		break;
	case OCRPT_EXPR:
		if (e->result_owned && e->result)
			ocrpt_free_expr_result(e->result, e->result->type);
		for (i = 0; i < e->n_ops; i++)
			ocrpt_free_expr(e->ops[i]);
		ocrpt_mem_free(e->ops);
		break;
	}

	if (e->result_owned)
		ocrpt_mem_free(e->result);
	ocrpt_mem_free(e);
}
