/*
 * Break utilities
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "exprutil.h"
#include "datasource.h"
#include "variables.h"
#include "breaks.h"
#include "parts.h"
#include "layout.h"

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_new(ocrpt_report *r, const char *name) {
	if (!r || !name)
		return NULL;

	ocrpt_break *br = ocrpt_mem_malloc(sizeof(ocrpt_break));

	if (!br)
		return NULL;

	memset(br, 0, sizeof(ocrpt_break));
	br->r = r;
	br->name = ocrpt_mem_strdup(name);
	br->header.o = r->o;
	br->header.r = r;
	br->footer.o = r->o;
	br->footer.r = r;

	/* Initialize rownum to 1, start with initial value */
	r->dont_add_exprs = true;
	br->rownum = ocrpt_report_expr_parse(r, "r.self + 1", NULL);
	r->dont_add_exprs = false;
	ocrpt_expr_init_results(br->rownum, OCRPT_RESULT_NUMBER);
	for (int i = 0; i < OCRPT_EXPR_RESULTS; i++)
		mpfr_set_ui(br->rownum->result[i]->number, 1, r->o->rndmode);
	ocrpt_expr_set_iterative_start_value(br->rownum, true);
	ocrpt_expr_resolve(br->rownum);
	ocrpt_expr_optimize(br->rownum);

	r->breaks = ocrpt_list_append(r->breaks, br);
	r->breaks_reverse = ocrpt_list_prepend(r->breaks_reverse, br);

	br->index = ocrpt_list_length(r->breaks) - 1;

	return br;
}

DLL_EXPORT_SYM void ocrpt_break_set_headernewpage(ocrpt_break *br, const char *headernewpage) {
	if (!br)
		return;

	ocrpt_expr_free(br->headernewpage_expr);
	br->headernewpage_expr = NULL;
	if (headernewpage)
		br->headernewpage_expr = ocrpt_report_expr_parse(br->r, headernewpage, NULL);
}

DLL_EXPORT_SYM void ocrpt_break_set_suppressblank(ocrpt_break *br, const char *suppressblank) {
	if (!br)
		return;

	ocrpt_expr_free(br->suppressblank_expr);
	br->suppressblank_expr = NULL;
	if (suppressblank)
		br->suppressblank_expr = ocrpt_report_expr_parse(br->r, suppressblank, NULL);
}

void ocrpt_break_free(ocrpt_break *br) {
	if (!br)
		return;

	for (ocrpt_list *ptr = br->breakfields; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;
		ocrpt_expr_free(e);
	}

	ocrpt_expr_free(br->headernewpage_expr);
	ocrpt_expr_free(br->suppressblank_expr);

	ocrpt_list_free(br->breakfields);
	ocrpt_list_free_deep(br->callbacks, ocrpt_mem_free);
	ocrpt_expr_free(br->rownum);
	ocrpt_mem_free(br->name);
	ocrpt_output_free(br->r->o, &br->header, false);
	ocrpt_output_free(br->r->o, &br->footer, false);
	ocrpt_mem_free(br);
}

void ocrpt_breaks_free(ocrpt_report *r) {
	if (!r)
		return;

	for (ocrpt_list *ptr = r->breaks; ptr; ptr = ptr->next) {
		ocrpt_break *br = (ocrpt_break *)ptr->data;

		ocrpt_break_free(br);
	}

	ocrpt_list_free(r->breaks);
	r->breaks = NULL;
	ocrpt_list_free(r->breaks_reverse);
	r->breaks_reverse = NULL;
}

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_get(ocrpt_report *r, const char *name) {
	if (!r || !name)
		return NULL;

	for (ocrpt_list *ptr = r->breaks; ptr; ptr = ptr->next) {
		ocrpt_break *br = (ocrpt_break *)ptr->data;

		if (strcmp(br->name, name) == 0)
			return br;
	}

	return NULL;
}

DLL_EXPORT_SYM const char *ocrpt_break_get_name(ocrpt_break *br) {
	return br ? br->name : NULL;
}

DLL_EXPORT_SYM bool ocrpt_break_add_breakfield(ocrpt_break *br, ocrpt_expr *bf) {
	if (!br || !bf || br->r != bf->r)
		return false;

	uint32_t vartypes;

	if (ocrpt_expr_references(bf, OCRPT_VARREF_VVAR, &vartypes)) {
		if ((vartypes & OCRPT_VARIABLE_UNKNOWN_BIT)) {
			ocrpt_err_printf("breakfield for '%s' references an unknown variable name\n", br->name);
			ocrpt_expr_free(bf);
			return false;
		}
		if ((vartypes & ~OCRPT_VARIABLE_EXPRESSION_BIT) != 0) {
			ocrpt_err_printf("breakfield for '%s' may only reference expression variables\n", br->name);
			ocrpt_expr_free(bf);
			return false;
		}
	}

	br->breakfields = ocrpt_list_append(br->breakfields, bf);
	return true;
}

DLL_EXPORT_SYM void ocrpt_break_resolve_fields(ocrpt_break *br) {
	if (!br)
		return;

	for (ocrpt_list *ptr = br->breakfields; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		ocrpt_expr_resolve(e);
		ocrpt_expr_optimize(e);
	}
}

DLL_EXPORT_SYM bool ocrpt_break_check_fields(ocrpt_break *br) {
	if (!br)
		return false;

	ocrpt_report *r = br->r;
	if (!r)
		return false;

	opencreport *o = r->o;
	if (!o)
		return false;

	bool match = true;

	for (ocrpt_list *ptr = br->breakfields; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		ocrpt_expr_eval(e);

		if (!ocrpt_expr_cmp_results(e)) {
			match = false;
			break;
		}
	}

	ocrpt_query *q = r->query;

	if (!q && br->r && br->r->o->queries)
		q = (ocrpt_query *)br->r->o->queries->data;

	/* Return true if any of the breakfield expressions don't match */
	bool retval = !match || (q && q->current_row == 0);

	if (retval) {
		mpfr_set_ui(EXPR_NUMERIC(br->rownum), 1, EXPR_RNDMODE(br->rownum));
		ocrpt_expr_set_iterative_start_value(br->rownum, true);
	}
	ocrpt_expr_eval(br->rownum);

	return retval;
}

DLL_EXPORT_SYM bool ocrpt_break_check_blank(ocrpt_break *br, bool evaluate) {
	if (!br || !br->suppressblank)
		return false;

	ocrpt_report *r = br->r;
	if (!r)
		return false;

	opencreport *o = r->o;
	if (!o)
		return false;

	for (ocrpt_list *ptr = br->breakfields; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		if (evaluate)
			ocrpt_expr_eval(e);

		if (!EXPR_RESULT(e))
			return true;
		if (EXPR_ISNULL(e))
			return true;
		if (EXPR_VALID_STRING(e) && EXPR_STRING_LEN(e) == 0)
			return true;
	}

	return false;
}

DLL_EXPORT_SYM void ocrpt_break_reset_vars(ocrpt_break *br) {
	if (!br)
		return;

	for (ocrpt_list *ptr = br->r->variables; ptr; ptr = ptr->next) {
		ocrpt_var *v = (ocrpt_var *)ptr->data;
		bool match = false;

		if (v->br) {
			if (v->br == br)
				match = true;
		} else if (v->br_name) {
			if (strcmp(v->br_name, br->name) == 0)
				match = true;
		}

		if (match)
			ocrpt_variable_reset(v);
	}

	for (int i = 0; i < OCRPT_EXPR_RESULTS; i++)
		mpfr_set_ui(br->rownum->result[i]->number, 1, br->r->o->rndmode);
	ocrpt_expr_set_iterative_start_value(br->rownum, true);
	ocrpt_expr_eval(br->rownum);
}

DLL_EXPORT_SYM bool ocrpt_break_add_trigger_cb(ocrpt_break *br, ocrpt_break_trigger_cb func, void *data) {
	if (!br || !func)
		return false;

	ocrpt_break_trigger_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_break_trigger_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	br->callbacks = ocrpt_list_append(br->callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_get_next(ocrpt_report *r, ocrpt_list **list) {
	if (!r || !list)
		return NULL;

	*list = *list ? (*list)->next : r->breaks;
	return (ocrpt_break *)(*list ? (*list)->data : NULL);
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_break_get_header(ocrpt_break *br) {
	if (!br)
		return NULL;

	return &br->header;
}

DLL_EXPORT_SYM ocrpt_output *ocrpt_break_get_footer(ocrpt_break *br) {
	if (!br)
		return NULL;

	return &br->footer;
}
