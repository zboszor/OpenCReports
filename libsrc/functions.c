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

bool ocrpt_init_func_result(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type) {
	ocrpt_result *result = e->result[o->residx];

	if (!result) {
		result = ocrpt_mem_malloc(sizeof(ocrpt_result));
		if (result) {
			memset(result, 0, sizeof(ocrpt_result));
			e->result[o->residx] = result;
			ocrpt_expr_set_result_owned(o, e, true);
		}
	}
	if (result) {
		if (type == OCRPT_RESULT_NUMBER && !result->number_initialized) {
			mpfr_init2(result->number, o->prec);
			result->number_initialized = true;
		}
		result->type = type;
	}

	return !!result;
}

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
	} else if (nstr == e->n_ops) {
		ocrpt_string *string;
		int32_t len, i;

		ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

		for (len = 0, i = 0; i < e->n_ops; i++)
			len += e->ops[i]->result[o->residx]->string->len;

		string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
		if (string) {
			if (!e->result[o->residx]->string) {
				e->result[o->residx]->string = string;
				e->result[o->residx]->string_owned = true;
			}
			for (i = 0; i < e->n_ops; i++) {
				ocrpt_string *sstring = e->ops[i]->result[o->residx]->string;
				ocrpt_mem_string_append_len(e->result[o->residx]->string, sstring->str, sstring->len);
			}
		} else
			ocrpt_expr_make_error_result(o, e, "out of memory");
	} else
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

static void ocrpt_val(opencreport *o, ocrpt_expr *e) {
	char *str;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_init_func_result(o, e, OCRPT_EXPR_NUMBER);
		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		break;
	case OCRPT_RESULT_STRING:
		str = e->ops[0]->result[o->residx]->string->str;
		if (!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "t"))
			str = "1";
		else if (!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "f"))
			str = "0";
		ocrpt_init_func_result(o, e, OCRPT_EXPR_NUMBER);
		mpfr_set_str(e->result[o->residx]->number, str, 10, o->rndmode);
		break;
	case OCRPT_RESULT_DATETIME:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	case OCRPT_RESULT_ERROR:
		break;
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

static void ocrpt_isnull(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_EXPR_NUMBER);
	mpfr_set_ui(e->result[o->residx]->number, e->ops[0]->result[o->residx]->isnull, o->rndmode);
}

/*
 * Keep this sorted by function name because it is
 * used via bsearch()
 */
static ocrpt_function ocrpt_functions[] = {
	{ "abs",		1,	false,	false,	false,	ocrpt_abs },
	{ "add",		-1,	true,	true,	false,	ocrpt_add },
	{ "and",		-1,	true,	true,	false,	NULL },
	{ "concat",		-1,	false,	false,	false,	NULL },
	{ "dec",		1,	false,	false,	false,	NULL },
	{ "div",		-1,	false,	false,	true,	ocrpt_div },
	{ "eq",			2,	true,	false,	false,	ocrpt_eq },
	{ "factorial",	1,	false,	false,	false,	NULL },
	{ "ge",			2,	false,	false,	false,	ocrpt_ge },
	{ "gt",			2,	false,	false,	false,	ocrpt_gt },
	{ "iif",		3,	false,	false,	false,	NULL },
	{ "inc",		1,	false,	false,	false,	NULL },
	{ "isnull",		1,	false,	false,	false,	ocrpt_isnull },
	{ "land",		-1,	true,	true,	false,	NULL },
	{ "le",			2,	false,	false,	false,	ocrpt_le },
	{ "lnot",		1,	false,	false,	false,	NULL },
	{ "lor",		-1,	true,	true,	false,	NULL },
	{ "lt",			2,	false,	false,	false,	ocrpt_lt },
	{ "mod",		2,	false,	false,	false,	NULL },
	{ "mul",		-1,	true,	true,	false,	ocrpt_mul },
	{ "ne",			2,	true,	false,	false,	ocrpt_ne },
	{ "not",		1,	false,	false,	false,	NULL },
	{ "or",			-1,	true,	true,	false,	NULL },
	{ "shl",		2,	false,	false,	false,	NULL },
	{ "shr",		2,	false,	false,	false,	NULL },
	{ "sub",		-1,	false,	false,	false,	ocrpt_sub },
	{ "uminus",		1,	false,	false,	false,	ocrpt_uminus },
	{ "xor",		-1,	true,	true,	true,	NULL },
	{ "val",		1,	false,	false,	false,	ocrpt_val },
};

static int n_ocrpt_functions = sizeof(ocrpt_functions) / sizeof(ocrpt_function);

static int funccmp(const void *key, const void *f) {
	return strcasecmp((const char *)key, ((const ocrpt_function *)f)->fname);
}

ocrpt_function *ocrpt_find_function(const char *fname) {
	return bsearch(fname, ocrpt_functions, n_ocrpt_functions, sizeof(ocrpt_function), funccmp);			
}
