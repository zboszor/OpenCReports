/*
 * Report part utilities
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "breaks.h"
#include "listutil.h"
#include "exprutil.h"
#include "variables.h"
#include "datasource.h"
#include "layout.h"
#include "parts.h"

DLL_EXPORT_SYM ocrpt_part *ocrpt_part_new(opencreport *o) {
	if (!o)
		return NULL;

	ocrpt_part *p = ocrpt_mem_malloc(sizeof(ocrpt_part));
	memset(p, 0, sizeof(ocrpt_part));
	p->o = o;
	p->iterations = 1;
	p->top_margin = OCRPT_DEFAULT_TOP_MARGIN;
	p->bottom_margin = OCRPT_DEFAULT_BOTTOM_MARGIN;
	p->left_margin = OCRPT_DEFAULT_LEFT_MARGIN;
	p->right_margin = OCRPT_DEFAULT_LEFT_MARGIN;
	p->pageheader.o = o;
	p->pagefooter.o = o;

	o->parts = ocrpt_list_end_append(o->parts, &o->last_part, p);

	for (ocrpt_list *cbl = o->part_added_callbacks; cbl; cbl = cbl->next) {
		ocrpt_part_cb_data *cbd = (ocrpt_part_cb_data *)cbl->data;

		cbd->func(o, p, cbd->data);
	}

	return p;
}

DLL_EXPORT_SYM ocrpt_part_row *ocrpt_part_new_row(ocrpt_part *p) {
	if (!p)
		return NULL;

	ocrpt_part_row *pr = ocrpt_mem_malloc(sizeof(ocrpt_part_row));
	memset(pr, 0, sizeof(ocrpt_part_row));
	pr->o = p->o;
	p->rows = ocrpt_list_end_append(p->rows, &p->row_last, pr);
	return pr;
}

DLL_EXPORT_SYM ocrpt_part_column *ocrpt_part_row_new_column(ocrpt_part_row *pr) {
	if (!pr)
		return NULL;

	ocrpt_part_column *pd = ocrpt_mem_malloc(sizeof(ocrpt_part_column));
	memset(pd, 0, sizeof(ocrpt_part_column));
	pd->o = pr->o;
	pd->detail_columns = 1;

	pr->pd_list = ocrpt_list_end_append(pr->pd_list, &pr->pd_last, pd);

	return pd;
}

DLL_EXPORT_SYM ocrpt_report *ocrpt_part_column_new_report(ocrpt_part_column *pd) {
	if (!pd)
		return NULL;

	ocrpt_report *r = ocrpt_mem_malloc(sizeof(ocrpt_report));
	memset(r, 0, sizeof(ocrpt_report));


	r->o = pd->o;
	r->iterations = 1;
	r->fieldheader_high_priority = true;
	r->nodata.o = pd->o;
	r->nodata.r = r;
	r->reportheader.o = pd->o;
	r->reportheader.r = r;
	r->reportfooter.o = pd->o;
	r->reportfooter.r = r;
	r->fieldheader.o = pd->o;
	r->fieldheader.r = r;
	r->fielddetails.o = pd->o;
	r->fielddetails.r = r;

	r->query_rownum = ocrpt_report_expr_parse(r, "rownum()", NULL);

	r->detailcnt = ocrpt_report_expr_parse(r, "r.self + 1", NULL);
	ocrpt_expr_init_iterative_results(r->detailcnt, OCRPT_RESULT_NUMBER);

	pd->reports = ocrpt_list_end_append(pd->reports, &pd->last_report, r);

	for (ocrpt_list *cbl = r->o->report_added_callbacks; cbl; cbl = cbl->next) {
		ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

		cbd->func(r->o, r, cbd->data);
	}

	return r;
}

void ocrpt_part_free(ocrpt_part *p) {
	ocrpt_list *row;

	ocrpt_expr_free(p->paper_type_expr);
	ocrpt_expr_free(p->font_name_expr);
	ocrpt_expr_free(p->font_size_expr);
	ocrpt_expr_free(p->top_margin_expr);
	ocrpt_expr_free(p->bottom_margin_expr);
	ocrpt_expr_free(p->left_margin_expr);
	ocrpt_expr_free(p->right_margin_expr);

	ocrpt_output_free(p->o, &p->pageheader, true);
	ocrpt_output_free(p->o, &p->pagefooter, true);

	for (row = p->rows; row; row = row->next) {
		ocrpt_part_row *pr = (ocrpt_part_row *)row->data;

		for (ocrpt_list *pdl = pr->pd_list; pdl; pdl = pdl->next) {
			ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

			for (ocrpt_list *rl = pd->reports; rl; rl = rl->next)
				ocrpt_report_free((ocrpt_report *)rl->data);

			ocrpt_list_free(pd->reports);
			ocrpt_mem_free(pd);
		}

		ocrpt_list_free(pr->pd_list);
		ocrpt_mem_free(pr);
	}
	ocrpt_list_free(p->rows);
	p->rows = NULL;
	p->row_last = NULL;

	ocrpt_mem_free(p->path);
	ocrpt_mem_free(p->font_name);
#if 0
	if (p->allocated)
		ocrpt_mem_free(p->xmlbuf);
#endif

	ocrpt_list_free_deep(p->iteration_callbacks, ocrpt_mem_free);

	ocrpt_mem_free(p);
}

void ocrpt_parts_free(opencreport *o) {
	ocrpt_list *parts = o->parts;

	o->parts = NULL;

	for (ocrpt_list *pl = parts; pl; pl = pl->next)
		ocrpt_part_free((ocrpt_part *)pl->data);

	ocrpt_list_free(parts);
}

void ocrpt_report_free(ocrpt_report *r) {
	ocrpt_variables_free(r);
	ocrpt_breaks_free(r);
	r->executing = true;
	for (ocrpt_list *l = r->exprs; l; l = l->next) {
		ocrpt_expr *e = (ocrpt_expr *)l->data;
		e->o->exprs = ocrpt_list_end_remove(e->o->exprs, &e->o->exprs_last, e);
		ocrpt_expr_free_internal(e, false);
	}
	ocrpt_list_free(r->exprs);
	r->executing = false;
	r->exprs = r->exprs_last = NULL;
	ocrpt_list_free(r->detailcnt_dependees);
	ocrpt_list_free_deep(r->start_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->done_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->newrow_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->precalc_done_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->iteration_callbacks, ocrpt_mem_free);
	ocrpt_output_free(r->o, &r->nodata, false);
	ocrpt_output_free(r->o, &r->reportheader, false);
	ocrpt_output_free(r->o, &r->reportfooter, false);
	ocrpt_output_free(r->o, &r->fieldheader, false);
	ocrpt_output_free(r->o, &r->fielddetails, false);
	ocrpt_mem_free(r->font_name);
	ocrpt_mem_free(r);
}

DLL_EXPORT_SYM void ocrpt_report_set_main_query(ocrpt_report *r, const ocrpt_query *query) {
	if (!r)
		return;

	r->query = (ocrpt_query *)query;
}

DLL_EXPORT_SYM void ocrpt_report_set_main_query_by_name(ocrpt_report *r, const char *query) {
	if (!r || !query || !*query)
		return;

	r->query = ocrpt_query_get(r->o, query);
}

DLL_EXPORT_SYM void ocrpt_report_resolve_variables(ocrpt_report *r) {
	if (!r)
		return;

	for (ocrpt_list *ptr = r->variables; ptr; ptr = ptr->next) {
		ocrpt_var *v = (ocrpt_var *)ptr->data;

		ocrpt_variable_resolve(v);
	}
}

DLL_EXPORT_SYM void ocrpt_report_evaluate_variables(ocrpt_report *r) {
	if (!r)
		return;

	for (ocrpt_list *ptr = r->variables; ptr; ptr = ptr->next) {
		ocrpt_var *v = (ocrpt_var *)ptr->data;

		ocrpt_variable_evaluate(v);
	}
}

DLL_EXPORT_SYM void ocrpt_report_resolve_breaks(ocrpt_report *r) {
	if (!r)
		return;

	opencreport *o = r->o;
	int residx = o->residx;

	for (ocrpt_list *ptr = r->breaks; ptr; ptr = ptr->next) {
		ocrpt_break *br = (ocrpt_break *)ptr->data;
		ocrpt_break_resolve_fields(br);

		br->headernewpage = false;
		br->suppressblank = false;
		br->blank = false;
		br->blank_prev = false;

		ocrpt_expr *e;

		e = br->headernewpage_expr;
		ocrpt_expr_resolve(e);
		ocrpt_expr_optimize(e);
		ocrpt_expr_eval(e);
		if (e && e->result[residx] && e->result[residx]->type == OCRPT_RESULT_NUMBER && e->result[residx]->number_initialized)
			br->headernewpage = !!mpfr_get_ui(e->result[residx]->number, r->o->rndmode);

		e = br->suppressblank_expr;
		ocrpt_expr_resolve(e);
		ocrpt_expr_optimize(e);
		ocrpt_expr_eval(e);
		if (e && e->result[residx] && e->result[residx]->type == OCRPT_RESULT_NUMBER && e->result[residx]->number_initialized)
			br->suppressblank = !!mpfr_get_ui(e->result[residx]->number, r->o->rndmode);
	}
}

DLL_EXPORT_SYM void ocrpt_report_resolve_expressions(ocrpt_report *r) {
	if (!r)
		return;

	for (ocrpt_list *ptr = r->exprs; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		ocrpt_expr_resolve(e);
		ocrpt_expr_optimize(e);

		if (e->delayed || ocrpt_expr_get_precalculate(e))
			r->have_delayed_expr = true;
	}
}

DLL_EXPORT_SYM void ocrpt_report_evaluate_expressions(ocrpt_report *r) {
	if (!r)
		return;

	for (ocrpt_list *ptr = r->exprs; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		ocrpt_expr_eval(e);
	}
}

void ocrpt_report_evaluate_detailcnt_dependees(ocrpt_report *r) {
	if (!r)
		return;

	for (ocrpt_list *ptr = r->detailcnt_dependees; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;
		ocrpt_expr_eval(e);
	}
}

DLL_EXPORT_SYM bool ocrpt_report_add_start_cb(ocrpt_report *r, ocrpt_report_cb func, void *data) {
	if (!r || !func)
		return false;

	ocrpt_report_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->start_callbacks = ocrpt_list_append(r->start_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_report_add_done_cb(ocrpt_report *r, ocrpt_report_cb func, void *data) {
	if (!r || !func)
		return false;

	ocrpt_report_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->done_callbacks = ocrpt_list_append(r->done_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_report_add_new_row_cb(ocrpt_report *r, ocrpt_report_cb func, void *data) {
	if (!r || !func)
		return false;

	ocrpt_report_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->newrow_callbacks = ocrpt_list_append(r->newrow_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_report_add_precalculation_done_cb(ocrpt_report *r, ocrpt_report_cb func, void *data) {
	if (!r || !func)
		return false;

	ocrpt_report_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->precalc_done_callbacks = ocrpt_list_append(r->precalc_done_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_report_add_iteration_cb(ocrpt_report *r, ocrpt_report_cb func, void *data) {
	if (!r || !func)
		return false;

	ocrpt_report_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->iteration_callbacks = ocrpt_list_append(r->iteration_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_part_add_iteration_cb(ocrpt_part *p, ocrpt_part_cb func, void *data) {
	if (!p || !func)
		return false;

	ocrpt_part_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_part_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	p->iteration_callbacks = ocrpt_list_append(p->iteration_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_add_precalculation_done_cb(opencreport *o, ocrpt_cb func, void *data) {
	if (!o || !func)
		return false;

	ocrpt_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	o->precalc_done_callbacks = ocrpt_list_append(o->precalc_done_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM ocrpt_part *ocrpt_part_get_next(opencreport *o, ocrpt_list **list) {
	if (!o || !list)
		return NULL;

	*list = *list ? (*list)->next : o->parts;
	return (ocrpt_part *)(*list ? (*list)->data : NULL);
}

DLL_EXPORT_SYM ocrpt_part_row *ocrpt_part_row_get_next(ocrpt_part *p, ocrpt_list **list) {
	if (!p || !list)
		return NULL;

	*list = *list ? (*list)->next : p->rows;
	return (ocrpt_part_row *)(*list ? (*list)->data : NULL);
}

DLL_EXPORT_SYM ocrpt_part_column *ocrpt_part_column_get_next(ocrpt_part_row *pr, ocrpt_list **list) {
	if (!pr || !list)
		return NULL;

	*list = *list ? (*list)->next : pr->pd_list;
	return (ocrpt_part_column *)(*list ? (*list)->data : NULL);
}

DLL_EXPORT_SYM ocrpt_report *ocrpt_report_get_next(ocrpt_part_column *pd, ocrpt_list **list) {
	if (!pd || !list)
		return NULL;

	*list = *list ? (*list)->next : pd->reports;
	return (ocrpt_report *)(*list ? (*list)->data : NULL);
}

DLL_EXPORT_SYM long ocrpt_report_get_query_rownum(ocrpt_report *r) {
	if (!r || !r->query_rownum)
		return 0L;

	return ocrpt_expr_get_long_value(r->query_rownum);
}
