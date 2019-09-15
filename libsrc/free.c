/*
 * OpenCReports memory freeing utilities
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

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
	case OCRPT_NUMBER_VALUE:
		return "NUMBER VALUE";
	case OCRPT_FLAT_EXPR:
		return "FLATTENED EXPR";
	}
	return "UNKNOWN_TYPE";
}

void ocrpt_free_expr(ocrpt_expr *e) {
	int i;

	if (!e)
		return;

	switch (e->type) {
	case OCRPT_EXPR_ERROR:
	case OCRPT_EXPR_NUMBER:
	case OCRPT_EXPR_STRING:
	case OCRPT_EXPR_DATETIME:
		ocrpt_strfree(e->string);
		break;
	case OCRPT_EXPR_MVAR:
	case OCRPT_EXPR_RVAR:
	case OCRPT_EXPR_VVAR:
	case OCRPT_EXPR_IDENT:
		ocrpt_strfree(e->query);
		ocrpt_strfree(e->name);
		break;
	case OCRPT_EXPR:
		for (i = 0; i < e->n_ops; i++) {
			if (e->uops[i].type == OCRPT_EXPR)
				ocrpt_free_expr(e->uops[i].op);
			else {
				/* TODO - is there anything needed to free a flattened expr? */
			}
		}
		ocrpt_strfree(e->fname);
		ocrpt_mem_free(e->uops);
		break;
	case OCRPT_NUMBER_VALUE:
		mpfr_clear(e->number);
		break;
	case OCRPT_FLAT_EXPR:
		// TODO free the array of references flattened expr ops
		ocrpt_strfree(e->fname);
		switch (e->expr_type) {
		case OCRPT_NUMBER_VALUE:
			mpfr_clear(e->number);
			break;
		case OCRPT_EXPR_STRING:
		case OCRPT_EXPR_ERROR:
			ocrpt_strfree(e->string);
			break;
		default:
			abort();
		}
		break;
	}

	ocrpt_mem_free(e);
}
