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
#include "exprutil.h"
#include "functions.h"

static void ocrpt_abs(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops == 1 && e->ops[0]->result[o->residx] && e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_abs(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

static void ocrpt_uminus(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops == 1 && e->ops[0]->result[o->residx] && e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		e->result[o->residx]->type = OCRPT_RESULT_NUMBER;
		mpfr_neg(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

static void ocrpt_add(opencreport *o, ocrpt_expr *e) {
	int i, nnum, nstr;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	nstr = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]) {
			if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
				nnum++;
			if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_STRING)
				nstr++;
		}
	}

	if (nnum == e->n_ops) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_add(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else if (nstr == e->n_ops)
		ocrpt_expr_make_error_result(o, e, "string concat is not implemented yet");
	else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

static void ocrpt_sub(opencreport *o, ocrpt_expr *e) {
	int i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx] && e->ops[i]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_sub(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

static void ocrpt_mul(opencreport *o, ocrpt_expr *e) {
	int i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx] && e->ops[i]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_mul(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

static void ocrpt_div(opencreport *o, ocrpt_expr *e) {
	int i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx] && e->ops[i]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_init_func_result(o, e, OCRPT_EXPR_NUMBER);
		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_div(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

static void ocrpt_eq(opencreport *o, ocrpt_expr *e) {
	int32_t ret;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) == 0);
		break;
	case OCRPT_RESULT_STRING:
		printf("%s:%d: called with '%s' and '%s'\n", __func__, __LINE__, e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str);
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) == 0);
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
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_ne(opencreport *o, ocrpt_expr *e) {
	int32_t ret;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) != 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) != 0);
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
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_lt(opencreport *o, ocrpt_expr *e) {
	int32_t ret;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) < 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) < 0);
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
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_le(opencreport *o, ocrpt_expr *e) {
	int32_t ret;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) <= 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) <= 0);
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
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_gt(opencreport *o, ocrpt_expr *e) {
	int32_t ret;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) > 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) > 0);
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
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_ge(opencreport *o, ocrpt_expr *e) {
	int32_t ret;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) >= 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) >= 0);
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
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
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
	{ "ge",			2,	false,	false,	ocrpt_ge },
	{ "gt",			2,	false,	false,	ocrpt_gt },
	{ "iif",		3,	false,	false,	NULL },
	{ "inc",		1,	false,	false,	NULL },
	{ "land",		-1,	true,	true,	NULL },
	{ "le",			2,	false,	false,	ocrpt_le },
	{ "lnot",		1,	false,	false,	NULL },
	{ "lor",		-1,	true,	true,	NULL },
	{ "lt",			2,	false,	false,	ocrpt_lt },
	{ "mod",		2,	false,	false,	NULL },
	{ "mul",		-1,	true,	true,	ocrpt_mul },
	{ "ne",			2,	true,	false,	ocrpt_ne },
	{ "not",		1,	false,	false,	NULL },
	{ "or",			-1,	true,	true,	NULL },
	{ "shl",		2,	false,	false,	NULL },
	{ "shr",		2,	false,	false,	NULL },
	{ "sub",		-1,	false,	false,	ocrpt_sub },
	{ "uminus",		1,	false,	false,	ocrpt_uminus },
	{ "xor",		-1,	true,	true,	NULL },
//	{ "val",		1,	false,	false,	ocrpt_val },
};

static int n_ocrpt_functions = sizeof(ocrpt_functions) / sizeof(ocrpt_function);

static int funccmp(const void *key, const void *f) {
	return strcasecmp((const char *)key, ((const ocrpt_function *)f)->fname);
}

ocrpt_function *ocrpt_find_function(const char *fname) {
	return bsearch(fname, ocrpt_functions, n_ocrpt_functions, sizeof(ocrpt_function), funccmp);			
}
