/*
 * OpenCReports main module
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opencreport-private.h"
#include "datasource.h"
#include "memutil.h"
#include "exprutil.h"
#include "functions.h"

static inline bool ocrpt_init_func_result(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type) {
	if (!e->result) {
		e->result = ocrpt_mem_malloc(sizeof(ocrpt_result));
		if (e->result) {
			memset(e->result, 0, sizeof(ocrpt_result));
			e->result_owned = true;
		}
	}
	if (e->result) {
		if (!e->result->number_initialized) {
			mpfr_init2(e->result->number, o->prec);
			e->result->number_initialized = true;
		}
		e->result->type = type;
	}

	return !!e->result;
}

static void ocrpt_abs(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops == 1 && (e->ops[0]->type == OCRPT_EXPR_NUMBER || (e->ops[0]->result && e->ops[0]->result->type == OCRPT_RESULT_NUMBER))) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_abs(e->result->number, e->ops[0]->result->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

static void ocrpt_uminus(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops == 1 && (e->ops[0]->type == OCRPT_EXPR_NUMBER || (e->ops[0]->result && e->ops[0]->result->type == OCRPT_RESULT_NUMBER))) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		e->result->type = OCRPT_RESULT_NUMBER;
		mpfr_neg(e->result->number, e->ops[0]->result->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

static void ocrpt_add(opencreport *o, ocrpt_expr *e) {
	int i, nnum, nstr;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	nstr = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[0]->type == OCRPT_EXPR_NUMBER || e->ops[0]->result->type == OCRPT_RESULT_NUMBER)
			nnum++;
		if (e->ops[0]->type == OCRPT_EXPR_STRING || e->ops[0]->result->type == OCRPT_RESULT_STRING)
			nstr++;
	}

	if (nnum == e->n_ops) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_set(e->result->number, e->ops[0]->result->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_add(e->result->number, e->result->number, e->ops[i]->result->number, o->rndmode);
	} else if (nstr == e->n_ops)
		ocrpt_expr_make_error_result(e, "string concat is not implemented yet");
	else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

static void ocrpt_sub(opencreport *o, ocrpt_expr *e) {
	int i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[0]->type == OCRPT_EXPR_NUMBER || (e->ops[0]->result && e->ops[0]->result->type == OCRPT_RESULT_NUMBER))
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_set(e->result->number, e->ops[0]->result->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_sub(e->result->number, e->result->number, e->ops[i]->result->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

static void ocrpt_mul(opencreport *o, ocrpt_expr *e) {
	int i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[0]->type == OCRPT_EXPR_NUMBER || (e->ops[0]->result && e->ops[0]->result->type == OCRPT_RESULT_NUMBER))
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_set(e->result->number, e->ops[0]->result->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_mul(e->result->number, e->result->number, e->ops[i]->result->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

static void ocrpt_div(opencreport *o, ocrpt_expr *e) {
	int i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[0]->type == OCRPT_EXPR_NUMBER || (e->ops[0]->result && e->ops[0]->result->type == OCRPT_RESULT_NUMBER))
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_init_func_result(o, e, OCRPT_EXPR_NUMBER);
		mpfr_set(e->result->number, e->ops[0]->result->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_div(e->result->number, e->result->number, e->ops[i]->result->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

static void ocrpt_eq(opencreport *o, ocrpt_expr *e) {
	int32_t ret;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result) {
		ocrpt_expr_make_error_result(e, "first operand has no result");
		return;
	}

	if (!e->ops[1]->result) {
		ocrpt_expr_make_error_result(e, "second operand has no result");
		return;
	}

	if (e->ops[0]->result->type != e->ops[1]->result->type) {
		if (e->ops[0]->result->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[0]->result->string);
		else if (e->ops[1]->result->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[1]->result->string);
		else
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result->number, e->ops[1]->result->number) == 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result->string, e->ops[1]->result->string) == 0);
		break;
	case OCRPT_RESULT_DATETIME:
		/* TODO */
		ret = false;
		break;
	default:
		ret = false;
		break;
	}

	ocrpt_init_func_result(o, e, OCRPT_EXPR_NUMBER);
	mpfr_set_ui(e->result->number, ret, o->rndmode);
}

/*
 * Keep this sorted by function name because it is
 * used via bsearch()
 */
static ocrpt_function ocrpt_functions[] = {
	{ "abs",		1,	false,	false,	ocrpt_abs },
	{ "add",		-1,	true,	true,	ocrpt_add },
	{ "and",		-1,	true,	true,	NULL },
	{ "concat",		-1,	false,	false,	NULL },
	{ "dec",		1,	false,	false,	NULL },
	{ "div",		-1,	false,	false,	ocrpt_div },
	{ "eq",			2,	true,	false,	ocrpt_eq },
	{ "factorial",	1,	false,	false,	NULL },
	{ "ge",			2,	false,	false,	NULL },
	{ "gt",			2,	false,	false,	NULL },
	{ "iif",		3,	false,	false,	NULL },
	{ "inc",		1,	false,	false,	NULL },
	{ "land",		-1,	true,	true,	NULL },
	{ "le",			2,	false,	false,	NULL },
	{ "lnot",		1,	false,	false,	NULL },
	{ "lor",		-1,	true,	true,	NULL },
	{ "lt",			2,	false,	false,	NULL },
	{ "mod",		2,	false,	false,	NULL },
	{ "mul",		-1,	true,	true,	ocrpt_mul },
	{ "ne",			2,	true,	false,	NULL },
	{ "not",		1,	false,	false,	NULL },
	{ "or",			-1,	true,	true,	NULL },
	{ "shl",		2,	false,	false,	NULL },
	{ "shr",		2,	false,	false,	NULL },
	{ "sub",		-1,	false,	false,	ocrpt_sub },
	{ "uminus",		1,	false,	false,	ocrpt_uminus },
	{ "xor",		-1,	true,	true,	NULL },
};

static int n_ocrpt_functions = sizeof(ocrpt_functions) / sizeof(ocrpt_function);

static int funccmp(const void *key, const void *f) {
	return strcasecmp((const char *)key, ((const ocrpt_function *)f)->fname);
}

ocrpt_function *ocrpt_find_function(const char *fname) {
	return bsearch(fname, ocrpt_functions, n_ocrpt_functions, sizeof(ocrpt_function), funccmp);			
}
