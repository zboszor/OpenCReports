/*
 * Break utilities
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport.h"
#include "exprutil.h"
#include "datasource.h"

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_new(opencreport *o, ocrpt_report *r, const char *name) {
	ocrpt_break *br;

	if (!o || !r)
		return NULL;

#if 0
	if (!ocrpt_part_validate(o, r))
		return NULL;
#endif

	br = ocrpt_mem_malloc(sizeof(ocrpt_break));

	if (!br)
		return NULL;

	memset(br, 0, sizeof(ocrpt_break));
	br->name = ocrpt_mem_strdup(name);

	//TODO: Append the new break to the list of breaks
	//o->breaks = ocrpt_list_append()

	return br;
}

DLL_EXPORT_SYM bool ocrpt_break_set_attribute(ocrpt_break *brk, const ocrpt_break_attr_type attr_type, bool value) {
	brk->attrs[attr_type] = value;
	return true;
}

DLL_EXPORT_SYM bool ocrpt_break_set_attribute_from_expr(opencreport *o, ocrpt_report *r, ocrpt_break *brk, const ocrpt_break_attr_type attr_type, ocrpt_expr *expr) {
	long tmp;

	if (!expr) {
		fprintf(stderr, "%s: invalid expression\n", __func__);
		return false;
	}

	ocrpt_expr_optimize(o, r, expr);
	if (!ocrpt_expr_is_const(expr) || !ocrpt_expr_is_dconst(expr)) {
		fprintf(stderr, "%s: invalid (non-constant) expression\n", __func__);
		return false;
	}

	tmp = mpfr_get_si(expr->ops[0]->result[o->residx]->number, o->rndmode);
	brk->attrs[attr_type] = !!tmp;
	return true;
}

DLL_EXPORT_SYM void ocrpt_break_free(opencreport *o, ocrpt_report *r, ocrpt_break *br) {
	/* TODO: remove r->breaks */
	ocrpt_list_free_deep(br->breakfields, (ocrpt_mem_free_t)ocrpt_expr_free);
	ocrpt_mem_free(br);
}

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_get(opencreport *o, ocrpt_report *r, const char *name) {
	return NULL;
}

DLL_EXPORT_SYM bool ocrpt_break_add_breakfield(opencreport *o, ocrpt_report *r, ocrpt_break *b, ocrpt_expr *bf) {
	return false;
}

DLL_EXPORT_SYM bool ocrpt_break_check_fields(opencreport *o, ocrpt_break *br) {
	return false;
}
