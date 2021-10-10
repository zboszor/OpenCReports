/*
 * OpenCReports main module
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utf8proc.h>

#include "opencreport.h"
#include "datasource.h"
#include "exprutil.h"

static bool ocrpt_expr_init_result_internal(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type, bool which) {
	ocrpt_result *result = e->result[which];

	if (!result) {
		result = ocrpt_mem_malloc(sizeof(ocrpt_result));
		if (result) {
			memset(result, 0, sizeof(ocrpt_result));
			assert(!e->result[which]);
			e->result[which] = result;
			ocrpt_expr_set_result_owned(o, e, which, true);
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

DLL_EXPORT_SYM bool ocrpt_expr_init_result(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type) {
	return ocrpt_expr_init_result_internal(o, e, type, o->residx);
}

DLL_EXPORT_SYM void ocrpt_expr_init_both_results(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type) {
	ocrpt_expr_init_result_internal(o, e, type, false);
	ocrpt_expr_init_result_internal(o, e, type, true);
}

OCRPT_STATIC_FUNCTION(ocrpt_abs) {
	if (e->n_ops == 1 && e->ops[0]->result[o->residx] && e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		mpfr_abs(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_uminus) {
	if (e->n_ops == 1 && e->ops[0]->result[o->residx] && e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		e->result[o->residx]->type = OCRPT_RESULT_NUMBER;
		mpfr_neg(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_add) {
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
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

		ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

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

OCRPT_STATIC_FUNCTION(ocrpt_sub) {
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
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_mul) {
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
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_div) {
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
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_eq) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_ne) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_lt) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_le) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_gt) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_ge) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_val) {
	char *str;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx] || e->ops[0]->result[o->residx]->type == OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_isnull) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
	mpfr_set_ui(e->result[o->residx]->number, e->ops[0]->result[o->residx]->isnull, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_null) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(o, e, e->ops[0]->result[o->residx]->type);
	e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulldt) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);
	e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulln) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
	e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulls) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);
	e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_iif) {
	int32_t opidx;
	long cond;
	ocrpt_string *string, *sstring;

	if (e->n_ops != 3) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER || e->ops[0]->result[o->residx]->isnull) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	cond = mpfr_get_si(e->ops[0]->result[o->residx]->number, o->rndmode);
	opidx = (cond ? 1 : 2);

	if (!e->ops[opidx]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}
	if (e->ops[opidx]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[opidx]->result[o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(o, e, e->ops[opidx]->result[o->residx]->type);

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

OCRPT_STATIC_FUNCTION(ocrpt_inc) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_dec) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_error) {
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

OCRPT_STATIC_FUNCTION(ocrpt_concat) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

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

OCRPT_STATIC_FUNCTION(ocrpt_left) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t l, len, i;

	if (e->n_ops != 2 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING || e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

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

OCRPT_STATIC_FUNCTION(ocrpt_right) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t l, start, i;

	if (e->n_ops != 2 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING || e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

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

OCRPT_STATIC_FUNCTION(ocrpt_mid) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t ofs, l, start, len, i;

	if (e->n_ops != 3 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING ||
			e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER ||
			e->ops[2]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

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

OCRPT_STATIC_FUNCTION(ocrpt_random) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
	mpfr_urandomb(e->result[o->residx]->number, o->randstate);
}

OCRPT_STATIC_FUNCTION(ocrpt_factorial) {
	intmax_t n;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		if (e->ops[0]->result[o->residx]->isnull) {
			ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
			e->result[o->residx]->isnull = true;
			return;
		}

		n = mpfr_get_sj(e->ops[0]->result[o->residx]->number, o->rndmode);
		if (n < 0LL || n > LONG_MAX) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}

		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
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

OCRPT_STATIC_FUNCTION(ocrpt_land) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_lor) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_lnot) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[o->residx]->number, 0) == 0);
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_and) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_or) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_xor) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_not) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, 0);
	mpfr_set_ui(e->result[o->residx]->number, ~ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_shl) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_shr) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

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

OCRPT_STATIC_FUNCTION(ocrpt_fmod) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	mpfr_fmod(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_remainder) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	mpfr_fmod(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_rint) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_rint(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_ceil) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_ceil(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_floor) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_floor(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_round) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_round(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_trunc) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_trunc(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_lower) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

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

OCRPT_STATIC_FUNCTION(ocrpt_upper) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

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

OCRPT_STATIC_FUNCTION(ocrpt_proper) {
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

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

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

OCRPT_STATIC_FUNCTION(ocrpt_rownum) {
	if (e->n_ops > 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->n_ops == 1) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
			return;
		}

		if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}

		if (!e->q && !e->ops[0]->result[o->residx]->isnull) {
			char *qname = e->ops[0]->result[o->residx]->string->str;
			ocrpt_list *ptr;

			for (ptr = o->queries; ptr; ptr = ptr->next) {
				ocrpt_query *tmp = (ocrpt_query *)ptr->data;
				if (strcmp(tmp->name, qname) == 0) {
					e->q = tmp;
					break;
				}
			}
		}
	} else {
		if (r && r->query)
			e->q = r->query;
		if (!e->q && o->queries)
			e->q = (ocrpt_query *)o->queries->data;
	}

	if (!e->q) {
		ocrpt_expr_make_error_result(o, e, "rownum(): no such query");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
	/*
	 * Internal row numbering is 0-based but in SQL it's 1-based.
	 * It feels more natural to the user to use the SQL row numbers.
	 * Also handle the case if a follow has run out of rows via ->isdone()
	 */
	if (e->q->source && e->q->source->input && e->q->source->input->isdone) {
		if (e->q->source->input->isdone(e->q))
			e->result[o->residx]->isnull = true;
		else
			mpfr_set_si(e->result[o->residx]->number, e->q->current_row + 1, o->rndmode);
	} else
		mpfr_set_si(e->result[o->residx]->number, e->q->current_row + 1, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_brrownum) {
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

	if (!e->br && !e->ops[0]->result[o->residx]->isnull) {
		char *brname = e->ops[0]->result[o->residx]->string->str;
		ocrpt_list *ptr;

		for (ptr = r->breaks; ptr; ptr = ptr->next) {
			ocrpt_break *tmp = (ocrpt_break *)ptr->data;
			if (strcmp(tmp->name, brname) == 0) {
				e->br = tmp;
				break;
			}
		}
	}

	if (!e->br) {
		ocrpt_expr_make_error_result(o, e, "brrownum(): no such break");
		return;
	}

	if (!e->result[o->residx]) {
		assert(!e->result[o->residx]);
		e->result[o->residx] = e->br->rownum->result[o->residx];
		ocrpt_expr_set_result_owned(o, e, o->residx, false);
	}
}

/*
 * Keep this sorted by function name because it is
 * used via bsearch()
 */
static const ocrpt_function ocrpt_functions[] = {
	{ "abs",		ocrpt_abs,	1,	false,	false,	false,	false },
	{ "add",		ocrpt_add,	-1,	true,	true,	false,	false },
	{ "and",		ocrpt_and,	-1,	true,	true,	false,	false },
	{ "brrownum",	ocrpt_brrownum,	1,	false,	false,	false,	true },
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
	{ "rownum",		ocrpt_rownum,	-1,	false,	false,	false,	true },
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

static int funccmpind(const void *key, const void *f) {
	return strcasecmp((const char *)key, (*(const ocrpt_function **)f)->fname);
}

static int funcsortind(const void *key1, const void *key2) {
	return strcasecmp((*(const ocrpt_function **)key1)->fname, (*(const ocrpt_function **)key2)->fname);
}

DLL_EXPORT_SYM bool ocrpt_function_add(opencreport *o, const char *fname, ocrpt_function_call func,
										int32_t n_ops, bool commutative, bool associative,
										bool left_associative, bool dont_optimize) {
	ocrpt_function *new_func;
	ocrpt_function **f_array;

	if (!fname || !*fname || !func)
		return false;

	new_func = ocrpt_mem_malloc(sizeof(ocrpt_function));
	if (!new_func)
		return false;

	f_array = ocrpt_mem_realloc(o->functions, (o->n_functions + 1) * sizeof(ocrpt_function *));
	if (!f_array) {
		ocrpt_mem_free(new_func);
		return false;
	}

	new_func->fname = ocrpt_mem_strdup(fname);
	if (!new_func->fname) {
		ocrpt_mem_free(new_func);
		return false;
	}

	new_func->func = func;
	new_func->n_ops = n_ops;
	new_func->commutative = commutative;
	new_func->associative = associative;
	new_func->left_associative = left_associative;
	new_func->dont_optimize = dont_optimize;

	o->functions = f_array;
	o->functions[o->n_functions++] = new_func;

	qsort(o->functions, o->n_functions, sizeof(ocrpt_function *), funcsortind);

	return true;
}

DLL_EXPORT_SYM ocrpt_function const * const ocrpt_function_get(opencreport *o, const char *fname) {
	ocrpt_function **ret;

	if (o->functions) {
		ret = bsearch(fname, o->functions, o->n_functions, sizeof(ocrpt_function *), funccmpind);
		if (ret)
			return *ret;
	}

	return bsearch(fname, ocrpt_functions, n_ocrpt_functions, sizeof(ocrpt_function), funccmp);
}
