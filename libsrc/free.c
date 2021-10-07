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
			ocrpt_result_free_data(e->result[0]);
		if (e->result_owned1)
			ocrpt_result_free_data(e->result[1]);
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
			ocrpt_result_free_data(e->result[0]);
		if (e->result_owned1)
			ocrpt_result_free_data(e->result[1]);
		break;
	case OCRPT_EXPR:
		if (e->result_owned0)
			ocrpt_result_free_data(e->result[0]);
		if (e->result_owned1)
			ocrpt_result_free_data(e->result[1]);
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
