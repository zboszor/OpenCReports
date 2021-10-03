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

	if (!o || !r || !name)
		return NULL;

	if (!ocrpt_report_validate(o, r))
		return NULL;

	br = ocrpt_mem_malloc(sizeof(ocrpt_break));

	if (!br)
		return NULL;

	memset(br, 0, sizeof(ocrpt_break));
	br->name = ocrpt_mem_strdup(name);

	/* Initialize rownum to 1, start with initial value */
	br->rownum = ocrpt_expr_parse(o, r, "r.self + 1", NULL);
	ocrpt_expr_init_both_results(o, br->rownum, OCRPT_RESULT_NUMBER);
	mpfr_set_ui(br->rownum->result[0]->number, 1, o->rndmode);
	mpfr_set_ui(br->rownum->result[1]->number, 1, o->rndmode);
	ocrpt_expr_set_iterative_start_value(br->rownum, true);
	ocrpt_expr_resolve(o, r, br->rownum);
	ocrpt_expr_optimize(o, r, br->rownum);

	r->breaks = ocrpt_list_append(r->breaks, br);

	return br;
}

DLL_EXPORT_SYM bool ocrpt_break_set_attribute(ocrpt_break *br, const ocrpt_break_attr_type attr_type, bool value) {
	br->attrs[attr_type] = value;
	return true;
}

DLL_EXPORT_SYM bool ocrpt_break_set_attribute_from_expr(opencreport *o, ocrpt_report *r, ocrpt_break *br, const ocrpt_break_attr_type attr_type, ocrpt_expr *expr) {
	long tmp;

	if (!ocrpt_break_validate(o, r, br))
		return false;

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
	br->attrs[attr_type] = !!tmp;
	return true;
}

DLL_EXPORT_SYM void ocrpt_break_free(opencreport *o, ocrpt_report *r, ocrpt_break *br) {
	ocrpt_list_free_deep(br->breakfields, (ocrpt_mem_free_t)ocrpt_expr_free);
	ocrpt_expr_free(br->rownum);
	ocrpt_mem_free(br->name);
	ocrpt_mem_free(br);
}

DLL_EXPORT_SYM void ocrpt_breaks_free(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;
	bool good = ocrpt_report_validate(o, r);

	for (ptr = good ? r->breaks : NULL; ptr; ptr = ptr->next) {
		ocrpt_break *br = (ocrpt_break *)ptr->data;

		ocrpt_break_free(o, r, br);
	}

	if (good) {
		ocrpt_list_free(r->breaks);
		r->breaks = NULL;
	}
}

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_get(opencreport *o, ocrpt_report *r, const char *name) {
	ocrpt_list *ptr;

	for (ptr = ocrpt_report_validate(o, r) ? r->breaks : NULL; ptr; ptr = ptr->next) {
		ocrpt_break *br = (ocrpt_break *)ptr->data;

		if (strcmp(br->name, name) == 0)
			return br;
	}

	return NULL;
}

DLL_EXPORT_SYM bool ocrpt_break_validate(opencreport *o, ocrpt_report *r, ocrpt_break *br) {
	ocrpt_list *ptr;

	if (!ocrpt_report_validate(o, r))
		return false;

	if (!br)
		return false;

	for (ptr = r->breaks; ptr; ptr = ptr->next) {
		if (br == ptr->data)
			break;
	}
	return !!ptr;
}

DLL_EXPORT_SYM bool ocrpt_break_add_breakfield(opencreport *o, ocrpt_report *r, ocrpt_break *br, ocrpt_expr *bf) {
	uint32_t vartypes;

	if (!bf)
		return false;

	if (!ocrpt_break_validate(o, r, br)) {
		ocrpt_expr_free(bf);
		return false;
	}

	if (ocrpt_expr_references(o, r, bf, OCRPT_VARREF_VVAR, &vartypes)) {
		if ((vartypes & OCRPT_VARIABLE_UNKNOWN_BIT)) {
			fprintf(stderr, "breakfield for '%s' references an unknown variable name\n", br->name);
			ocrpt_expr_free(bf);
			return false;
		}
		if ((vartypes & ~OCRPT_VARIABLE_EXPRESSION_BIT) != 0) {
			fprintf(stderr, "breakfield for '%s' may only reference expression variables\n", br->name);
			ocrpt_expr_free(bf);
			return false;
		}
	}

	br->breakfields = ocrpt_list_append(br->breakfields, bf);
	return true;
}

DLL_EXPORT_SYM void ocrpt_break_resolve_fields(opencreport *o, ocrpt_report *r, ocrpt_break *br) {
	ocrpt_list *ptr;

	for (ptr = br->breakfields; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		ocrpt_expr_resolve(o, r, e);
		ocrpt_expr_optimize(o, r, e);
	}
}

DLL_EXPORT_SYM bool ocrpt_break_check_fields(opencreport *o, ocrpt_report *r, ocrpt_break *br) {
	ocrpt_query *q = NULL;
	ocrpt_list *ptr;
	bool match = true;
	bool retval;

	for (ptr = br->breakfields; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		ocrpt_expr_eval(o, r, e);
		match = ocrpt_expr_cmp_results(o, e) && match;
	}

	if (r && r->query)
		q = r->query;
	if (!q && o->queries)
		q = (ocrpt_query *)o->queries->data;

	/* Return true if any of the breakfield expressions don't match */
	retval = !match || (q && q->current_row == 0);

	if (retval) {
		mpfr_set_ui(br->rownum->result[o->residx]->number, 1, o->rndmode);
		ocrpt_expr_set_iterative_start_value(br->rownum, true);
	}
	ocrpt_expr_eval(o, r, br->rownum);

	return retval;
}
