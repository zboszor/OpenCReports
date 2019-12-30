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
#include <utf8proc.h>

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
		result->isnull = false;
	}

	return !!result;
}

static void ocrpt_abs(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops == 1 && e->ops[0]->result[o->residx] && e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		mpfr_abs(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

static void ocrpt_uminus(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops == 1 && e->ops[0]->result[o->residx] && e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

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

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_add(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else if (nstr == e->n_ops) {
		ocrpt_string *string;
		int32_t len, i;

		ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

		for (len = 0, i = 0; i < e->n_ops; i++)
			len += e->ops[i]->result[o->residx]->string->len;

		string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
		if (string) {
			if (!e->result[o->residx]->string) {
				e->result[o->residx]->string = string;
				e->result[o->residx]->string_owned = true;
			}
			string->len = 0;
			for (i = 0; i < e->n_ops; i++) {
				ocrpt_string *sstring = e->ops[i]->result[o->residx]->string;
				ocrpt_mem_string_append_len(string, sstring->str, sstring->len);
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

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

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

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

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
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_div(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

static void ocrpt_eq(opencreport *o, ocrpt_expr *e) {
	unsigned long ret;
	int32_t i;

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

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) == 0);
		break;
	case OCRPT_RESULT_STRING:
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

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_ne(opencreport *o, ocrpt_expr *e) {
	unsigned long ret;
	int32_t i;

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

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
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

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_lt(opencreport *o, ocrpt_expr *e) {
	unsigned long ret;
	int32_t i;

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

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
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

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_le(opencreport *o, ocrpt_expr *e) {
	unsigned long ret;
	int32_t i;

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

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
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

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_gt(opencreport *o, ocrpt_expr *e) {
	unsigned long ret;
	int32_t i;

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

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
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

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_ge(opencreport *o, ocrpt_expr *e) {
	unsigned long ret;
	int i;

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

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
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

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_val(opencreport *o, ocrpt_expr *e) {
	char *str;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx] || e->ops[0]->result[o->residx]->type == OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		break;
	case OCRPT_RESULT_STRING:
		str = e->ops[0]->result[o->residx]->string->str;
		if (!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "t"))
			str = "1";
		else if (!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "f"))
			str = "0";
		mpfr_set_str(e->result[o->residx]->number, str, 10, o->rndmode);
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

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
	mpfr_set_ui(e->result[o->residx]->number, e->ops[0]->result[o->residx]->isnull, o->rndmode);
}

static void ocrpt_null(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_init_func_result(o, e, e->ops[0]->result[o->residx]->type);
	e->result[o->residx]->isnull = true;
}

static void ocrpt_nulldt(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_DATETIME);
	e->result[o->residx]->isnull = true;
}

static void ocrpt_nulln(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
	e->result[o->residx]->isnull = true;
}

static void ocrpt_nulls(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);
	e->result[o->residx]->isnull = true;
}

static void ocrpt_iif(opencreport *o, ocrpt_expr *e) {
	int32_t i, opidx;
	long cond;
	ocrpt_string *string, *sstring;

	if (e->n_ops != 3) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER || e->ops[0]->result[o->residx]->isnull) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	cond = mpfr_get_si(e->ops[0]->result[o->residx]->number, o->rndmode);
	opidx = (cond ? 1 : 2);

	ocrpt_init_func_result(o, e, e->ops[opidx]->result[o->residx]->type);

	if (e->ops[opidx]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	switch (e->ops[opidx]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		mpfr_set(e->result[o->residx]->number, e->ops[opidx]->result[o->residx]->number, o->rndmode);
		break;
	case OCRPT_RESULT_STRING:
		sstring = e->ops[opidx]->result[o->residx]->string;
		string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len);
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}
		e->result[o->residx]->string->len = 0;
		ocrpt_mem_string_append_len(e->result[o->residx]->string, sstring->str, sstring->len);
		break;
	case OCRPT_RESULT_DATETIME:
		/* TODO */
		break;
	default:
		break;
	}
}

static void ocrpt_inc(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		mpfr_add_ui(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, 1, o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

static void ocrpt_dec(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		mpfr_sub_ui(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, 1, o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

static void ocrpt_error(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx] || e->ops[0]->result[o->residx]->isnull) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_STRING:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_NUMBER:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

static void ocrpt_concat(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *string;
	int32_t len, i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx] || e->ops[i]->result[o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	for (len = 0, i = 0; i < e->n_ops; i++)
		len += e->ops[i]->result[o->residx]->string->len;

	string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		for (i = 0; i < e->n_ops; i++) {
			ocrpt_string *sstring = e->ops[i]->result[o->residx]->string;
			ocrpt_mem_string_append_len(string, sstring->str, sstring->len);
		}
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

static void utf8forward(const char *s, int l, int blen, int *blen2) {
	int i = 0, j = 0;

	while (j < l && i < blen) {
		if ((s[i] & 0xf8) == 0xf0)
			i+= 4, j++;
		else if ((s[i] & 0xf0) == 0xe0)
			i += 3, j++;
		else if ((s[i] & 0xe0) == 0xc0)
			i += 2, j++;
		else
			i++, j++;
	}

	if (i > blen)
		i = blen;

	*blen2 = i;
}

static void utf8backward(const char *s, int l, int blen, int *blen2) {
	int i = blen, j = 0;

	while (j < l && i > 0) {
		i--;
		if (((s[i] & 0xf8) == 0xf0) || ((s[i] & 0xf0) == 0xe0) || ((s[i] & 0xe0) == 0xc0) || ((s[i] & 0x80) == 0x00))
			j++;
	}

	*blen2 = i;
}

static void ocrpt_left(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t l, len, i;

	if (e->n_ops != 2 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING || e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	l = mpfr_get_si(e->ops[1]->result[o->residx]->number, o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[o->residx]->string;

	utf8forward(sstring->str, l, sstring->len, &len);

	string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str, len);
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

static void ocrpt_right(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t l, start, i;

	if (e->n_ops != 2 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING || e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	l = mpfr_get_si(e->ops[1]->result[o->residx]->number, o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[o->residx]->string;

	utf8backward(sstring->str, l, sstring->len, &start);

	string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len - start);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str + start, sstring->len - start);
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

static void ocrpt_mid(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t ofs, l, start, len, i;

	if (e->n_ops != 3 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING ||
			e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER ||
			e->ops[2]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ofs = mpfr_get_si(e->ops[1]->result[o->residx]->number, o->rndmode);
	l = mpfr_get_si(e->ops[2]->result[o->residx]->number, o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[o->residx]->string;

	if (ofs < 0)
		utf8backward(sstring->str, -ofs, sstring->len, &start);
	else if (ofs > 0)
		utf8forward(sstring->str, ofs - 1, sstring->len, &start);
	else
		start = 0;
	utf8forward(sstring->str + start, l, sstring->len - start, &len);

	string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str + start, len);
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

static void ocrpt_random(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
	mpfr_urandomb(e->result[o->residx]->number, o->randstate);
}

static void ocrpt_factorial(opencreport *o, ocrpt_expr *e) {
	intmax_t n;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		if (e->ops[0]->result[o->residx]->isnull) {
			ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
			e->result[o->residx]->isnull = true;
			return;
		}

		n = mpfr_get_sj(e->ops[0]->result[o->residx]->number, o->rndmode);
		if (n < 0LL || n > LONG_MAX) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}

		ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_fac_ui(e->result[o->residx]->number, (unsigned long)n, o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

static void ocrpt_land(opencreport *o, ocrpt_expr *e) {
	unsigned long ret;
	int32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[o->residx]->number, 0) != 0);
	for (i = 1; ret && i < e->n_ops; i++) {
		unsigned long ret1 = (mpfr_cmp_ui(e->ops[i]->result[o->residx]->number, 0) != 0);
		ret = ret && ret1;
	}
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_lor(opencreport *o, ocrpt_expr *e) {
	unsigned long ret;
	int32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[o->residx]->number, 0) != 0);
	for (i = 1; !ret && i < e->n_ops; i++) {
		unsigned long ret1 = (mpfr_cmp_ui(e->ops[i]->result[o->residx]->number, 0) != 0);
		ret = ret || ret1;
	}
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_lnot(opencreport *o, ocrpt_expr *e) {
	intmax_t ret;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[o->residx]->number, 0) == 0);
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_and(opencreport *o, ocrpt_expr *e) {
	uintmax_t ret;
	int32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	for (i = 1; ret && i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[o->residx]->number, o->rndmode);
		ret &= ret1;
	}
	mpfr_set_uj(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_or(opencreport *o, ocrpt_expr *e) {
	uintmax_t ret;
	int32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	for (i = 1; !ret && i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[o->residx]->number, o->rndmode);
		ret |= ret1;
	}
	mpfr_set_uj(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_xor(opencreport *o, ocrpt_expr *e) {
	uintmax_t ret;
	int32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	for (i = 1; !ret && i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[o->residx]->number, o->rndmode);
		ret ^= ret1;
	}
	mpfr_set_uj(e->result[o->residx]->number, ret, o->rndmode);
}

static void ocrpt_not(opencreport *o, ocrpt_expr *e) {
	intmax_t ret;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, 0);
	mpfr_set_ui(e->result[o->residx]->number, ~ret, o->rndmode);
}

static void ocrpt_shl(opencreport *o, ocrpt_expr *e) {
	uintmax_t op, shift;
	int32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	op = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	shift = mpfr_get_uj(e->ops[1]->result[o->residx]->number, o->rndmode);
	mpfr_set_uj(e->result[o->residx]->number, op << shift, o->rndmode);
}

static void ocrpt_shr(opencreport *o, ocrpt_expr *e) {
	uintmax_t op, shift;
	int32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	op = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	shift = mpfr_get_uj(e->ops[1]->result[o->residx]->number, o->rndmode);
	mpfr_set_uj(e->result[o->residx]->number, op >> shift, o->rndmode);
}

static void ocrpt_fmod(opencreport *o, ocrpt_expr *e) {
	int32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	mpfr_fmod(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number, o->rndmode);
}

static void ocrpt_remainder(opencreport *o, ocrpt_expr *e) {
	int32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	mpfr_fmod(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number, o->rndmode);
}

static void ocrpt_rint(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_rint(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

static void ocrpt_ceil(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_ceil(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

static void ocrpt_floor(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_floor(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

static void ocrpt_round(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_round(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

static void ocrpt_trunc(opencreport *o, ocrpt_expr *e) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_trunc(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

static void ocrpt_lower(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		char cc[8];

		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		bytes_total = 0;

		while (bytes_total < sstring->len) {
			bytes_read = utf8proc_iterate((utf8proc_uint8_t *)(sstring->str + bytes_total), sstring->len - bytes_total, &c);
			c = utf8proc_tolower(c);
			bytes_written = utf8proc_encode_char(c, (utf8proc_uint8_t *)cc);
			ocrpt_mem_string_append_len(string, cc, bytes_written);
			bytes_total += bytes_read;
		}
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

static void ocrpt_upper(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		char cc[8];

		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		bytes_total = 0;

		while (bytes_total < sstring->len) {
			bytes_read = utf8proc_iterate((utf8proc_uint8_t *)(sstring->str + bytes_total), sstring->len - bytes_total, &c);
			c = utf8proc_toupper(c);
			bytes_written = utf8proc_encode_char(c, (utf8proc_uint8_t *)cc);
			ocrpt_mem_string_append_len(string, cc, bytes_written);
			bytes_total += bytes_read;
		}
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

static void ocrpt_proper(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_init_func_result(o, e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		bool first = true;
		char cc[8];

		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		bytes_total = 0;

		while (bytes_total < sstring->len) {
			bytes_read = utf8proc_iterate((utf8proc_uint8_t *)(sstring->str + bytes_total), sstring->len - bytes_total, &c);
			c = (first ? utf8proc_toupper(c) : utf8proc_tolower(c));
			bytes_written = utf8proc_encode_char(c, (utf8proc_uint8_t *)cc);
			ocrpt_mem_string_append_len(string, cc, bytes_written);
			bytes_total += bytes_read;
			first = false;
		}
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

/*
 * Keep this sorted by function name because it is
 * used via bsearch()
 */
static ocrpt_function ocrpt_functions[] = {
	{ "abs",		ocrpt_abs,	1,	false,	false,	false,	false },
	{ "add",		ocrpt_add,	-1,	true,	true,	false,	false },
	{ "and",		ocrpt_and,	-1,	true,	true,	false,	false },
	{ "ceil",		ocrpt_ceil,	1,	false,	false,	false,	false },
	{ "concat",		ocrpt_concat,	-1,	false,	false,	false,	false },
	{ "dec",		ocrpt_dec,	1,	false,	false,	false,	false },
	{ "div",		ocrpt_div,	-1,	false,	false,	true,	false },
	{ "eq",			ocrpt_eq,	2,	true,	false,	false,	false },
	{ "error",		ocrpt_error,	1,	false,	false,	false,	false },
	{ "factorial",	ocrpt_factorial,	1,	false,	false,	false,	false },
	{ "floor",		ocrpt_floor,	1,	false,	false,	false,	false },
	{ "fmod",		ocrpt_fmod,	2,	false,	false,	false,	false },
	{ "ge",			ocrpt_ge,	2,	false,	false,	false,	false },
	{ "gt",			ocrpt_gt,	2,	false,	false,	false,	false },
	{ "iif",		ocrpt_iif,	3,	false,	false,	false,	false },
	{ "inc",		ocrpt_inc,	1,	false,	false,	false,	false },
	{ "isnull",		ocrpt_isnull,	1,	false,	false,	false,	false },
	{ "land",		ocrpt_land,	-1,	true,	true,	false,	false },
	{ "le",			ocrpt_le,	2,	false,	false,	false,	false },
	{ "left",		ocrpt_left,	2,	false,	false,	false,	false },
	{ "lnot",		ocrpt_lnot,	1,	false,	false,	false,	false },
	{ "lor",		ocrpt_lor,	-1,	true,	true,	false,	false },
	{ "lower",		ocrpt_lower,	1,	false,	false,	false,	false },
	{ "lt",			ocrpt_lt,	2,	false,	false,	false,	false },
	{ "mid",		ocrpt_mid,	3,	false,	false,	false,	false },
	{ "mod",		ocrpt_remainder,	2,	false,	false,	false,	false },
	{ "mul",		ocrpt_mul,	-1,	true,	true,	false,	false },
	{ "ne",			ocrpt_ne,	2,	true,	false,	false,	false },
	{ "not",		ocrpt_not,	1,	false,	false,	false,	false },
	{ "null",		ocrpt_null,	1,	false,	false,	false,	false },
	{ "nulldt",		ocrpt_nulldt,	0,	false,	false,	false,	false },
	{ "nulln",		ocrpt_nulln,	0,	false,	false,	false,	false },
	{ "nulls",		ocrpt_nulls,	0,	false,	false,	false,	false },
	{ "or",			ocrpt_or,	-1,	true,	true,	false,	false },
	{ "proper",		ocrpt_proper,	1,	false,	false,	false,	false },
	{ "random",		ocrpt_random,	0,	false,	false,	false,	true },
	{ "remainder",	ocrpt_remainder,	2,	false,	false,	false,	false },
	{ "right",		ocrpt_right,	2,	false,	false,	false,	false },
	{ "rint",		ocrpt_rint,	1,	false,	false,	false,	false },
	{ "round",		ocrpt_round,	1,	false,	false,	false,	false },
	{ "shl",		ocrpt_shl,	2,	false,	false,	false,	false },
	{ "shr",		ocrpt_shr,	2,	false,	false,	false,	false },
	{ "sub",		ocrpt_sub,	-1,	false,	false,	false,	false },
	{ "trunc",		ocrpt_trunc,	1,	false,	false,	false,	false },
	{ "uminus",		ocrpt_uminus,	1,	false,	false,	false,	false },
	{ "upper",		ocrpt_upper,	1,	false,	false,	false,	false },
	{ "val",		ocrpt_val,	1,	false,	false,	false,	false },
	{ "xor",		ocrpt_xor,	-1,	true,	true,	true,	false },
};

static int n_ocrpt_functions = sizeof(ocrpt_functions) / sizeof(ocrpt_function);

static int funccmp(const void *key, const void *f) {
	return strcasecmp((const char *)key, ((const ocrpt_function *)f)->fname);
}

ocrpt_function *ocrpt_find_function(const char *fname) {
	return bsearch(fname, ocrpt_functions, n_ocrpt_functions, sizeof(ocrpt_function), funccmp);
}
