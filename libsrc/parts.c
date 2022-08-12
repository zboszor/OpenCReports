/*
 * Report part utilities
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport.h"
#include "exprutil.h"
#include "datasource.h"
#include "layout.h"

DLL_EXPORT_SYM ocrpt_part *ocrpt_part_new(opencreport *o) {
	ocrpt_part *p;
	ocrpt_list *cbl;

	p = ocrpt_mem_malloc(sizeof(ocrpt_part));
	memset(p, 0, sizeof(ocrpt_part));
	p->iterations = 1;

	o->parts = ocrpt_list_end_append(o->parts, &o->last_part, p);

	for (cbl = o->part_added_callbacks; cbl; cbl = cbl->next) {
		ocrpt_part_cb_data *cbd = (ocrpt_part_cb_data *)cbl->data;

		cbd->func(o, p, cbd->data);
	}

	return p;
}

DLL_EXPORT_SYM ocrpt_part_row *ocrpt_part_new_row(opencreport *o, ocrpt_part *p) {
	ocrpt_part_row *pr = ocrpt_mem_malloc(sizeof(ocrpt_part_row));
	memset(pr, 0, sizeof(ocrpt_part_row));
	p->rows = ocrpt_list_end_append(p->rows, &p->row_last, pr);
	return pr;
}

DLL_EXPORT_SYM ocrpt_part_row_data *ocrpt_part_row_new_data(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr) {
	ocrpt_part_row_data *pd = ocrpt_mem_malloc(sizeof(ocrpt_part_row_data));
	memset(pd, 0, sizeof(ocrpt_part_row_data));
	pr->pd_list = ocrpt_list_end_append(pr->pd_list, &pr->pd_last, pd);
	return pd;
}

DLL_EXPORT_SYM ocrpt_part *ocrpt_part_append_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r) {
	if (!p)
		p = ocrpt_part_new(o);

	if (!pr)
		pr = ocrpt_part_new_row(o, p);

	if (!pd)
		pd = ocrpt_part_row_new_data(o, p, pr);

	pd->reports = ocrpt_list_end_append(pd->reports, &pd->last_report, r);
	r->part = p;

	for (ocrpt_list *cbl = o->report_added_callbacks; cbl; cbl = cbl->next) {
		ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

		cbd->func(o, r, cbd->data);
	}

	return p;
}

DLL_EXPORT_SYM void ocrpt_part_free(opencreport *o, ocrpt_part *p) {
	ocrpt_list *row;

	for (row = p->rows; row; row = row->next) {
		ocrpt_part_row *pr = (ocrpt_part_row *)row->data;

		for (ocrpt_list *pdl = pr->pd_list; pdl; pdl = pdl->next) {
			ocrpt_part_row_data *pd = (ocrpt_part_row_data *)pdl->data;

			for (ocrpt_list *rl = pd->reports; rl; rl = rl->next)
				ocrpt_report_free(o, (ocrpt_report *)rl->data);

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
#if 0
	if (p->allocated)
		ocrpt_mem_free(p->xmlbuf);
#endif
	ocrpt_output_free(o, NULL, &p->pageheader);
	ocrpt_output_free(o, NULL, &p->pagefooter);
	ocrpt_mem_free(p);
}

DLL_EXPORT_SYM void ocrpt_parts_free(opencreport *o) {
	ocrpt_list *parts = o->parts;

	o->parts = NULL;

	for (ocrpt_list *pl = parts; pl; pl = pl->next)
		ocrpt_part_free(o, (ocrpt_part *)pl->data);

	ocrpt_list_free(parts);
}

DLL_EXPORT_SYM ocrpt_report *ocrpt_report_new(opencreport *o) {
	ocrpt_report *r = ocrpt_mem_malloc(sizeof(ocrpt_report));
	memset(r, 0, sizeof(ocrpt_report));
	r->iterations = 1;
	r->query_rownum = ocrpt_expr_parse(o, r, "rownum()", NULL);
	ocrpt_expr_resolve(o, r, r->query_rownum);
	return r;
}

DLL_EXPORT_SYM void ocrpt_report_free(opencreport *o, ocrpt_report *r) {
	ocrpt_variables_free(o, r);
	ocrpt_breaks_free(o, r);
	r->executing = true;
	for (ocrpt_list *l = r->exprs; l; l = l->next)
		ocrpt_expr_free(o, r, (ocrpt_expr *)l->data);
	ocrpt_list_free(r->exprs);
	r->executing = false;
	r->exprs = r->exprs_last = NULL;
	ocrpt_list_free_deep(r->start_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->done_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->newrow_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->precalc_done_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->iteration_callbacks, ocrpt_mem_free);
	ocrpt_output_free(o, r, &r->nodata);
	ocrpt_output_free(o, r, &r->reportheader);
	ocrpt_output_free(o, r, &r->reportfooter);
	ocrpt_output_free(o, r, &r->fieldheader);
	ocrpt_output_free(o, r, &r->fielddetails);
	ocrpt_mem_free(r->query);
	ocrpt_mem_free(r);
}

DLL_EXPORT_SYM bool ocrpt_report_validate(opencreport *o, ocrpt_report *r) {
	if (!o || !r)
		return false;

	for (ocrpt_list *part = o->parts; part; part = part->next) {
		ocrpt_part *p = (ocrpt_part *)part->data;

		for (ocrpt_list *row = p->rows; row; row = row->next) {
			ocrpt_part_row *pr = (ocrpt_part_row *)row->data;

			for (ocrpt_list *pdl = pr->pd_list; pdl; pdl = pdl->next) {
				ocrpt_part_row_data *pd = (ocrpt_part_row_data *)pdl->data;

				for (ocrpt_list *rl = pd->reports; rl; rl = rl->next)
					if (r == rl->data)
						return true;
			}
		}
	}

	return false;
}

DLL_EXPORT_SYM void ocrpt_report_set_main_query(ocrpt_report *r, const char *query) {
	r->query = ocrpt_mem_strdup(query);
}

DLL_EXPORT_SYM void ocrpt_report_resolve_variables(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->variables; ptr; ptr = ptr->next) {
		ocrpt_var *v = (ocrpt_var *)ptr->data;

		ocrpt_variable_resolve(o, r, v);
	}
}

DLL_EXPORT_SYM void ocrpt_report_evaluate_variables(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->variables; ptr; ptr = ptr->next) {
		ocrpt_var *v = (ocrpt_var *)ptr->data;

		ocrpt_variable_evaluate(o, r, v);
	}
}

DLL_EXPORT_SYM void ocrpt_report_resolve_breaks(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->breaks; ptr; ptr = ptr->next) {
		ocrpt_break *br = (ocrpt_break *)ptr->data;
		ocrpt_break_resolve_fields(o, r, br);
	}
}

DLL_EXPORT_SYM void ocrpt_report_resolve_expressions(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->exprs; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		ocrpt_expr_resolve(o, r, e);
		ocrpt_expr_optimize(o, r, e);

		if (e->delayed || ocrpt_expr_get_precalculate(o, e))
			r->have_delayed_expr = true;
	}
}

DLL_EXPORT_SYM void ocrpt_report_evaluate_expressions(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->exprs; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		ocrpt_expr_eval(o, r, e);
	}
}

DLL_EXPORT_SYM bool ocrpt_report_add_start_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data) {
	ocrpt_report_cb_data *ptr;

	if (!ocrpt_report_validate(o, r))
		return false;

	if (!func)
		return false;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->start_callbacks = ocrpt_list_append(r->start_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_report_add_done_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data) {
	ocrpt_report_cb_data *ptr;

	if (!ocrpt_report_validate(o, r))
		return false;

	if (!func)
		return false;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->done_callbacks = ocrpt_list_append(r->done_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_report_add_new_row_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data) {
	ocrpt_report_cb_data *ptr;

	if (!ocrpt_report_validate(o, r))
		return false;

	if (!func)
		return false;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->newrow_callbacks = ocrpt_list_append(r->newrow_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_report_add_precalculation_done_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data) {
	ocrpt_report_cb_data *ptr;

	if (!ocrpt_report_validate(o, r))
		return false;

	if (!func)
		return false;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->precalc_done_callbacks = ocrpt_list_append(r->precalc_done_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_report_add_iteration_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data) {
	ocrpt_report_cb_data *ptr;

	if (!ocrpt_report_validate(o, r))
		return false;

	if (!func)
		return false;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	r->iteration_callbacks = ocrpt_list_append(r->iteration_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_add_part_iteration_cb(opencreport *o, ocrpt_part_cb func, void *data) {
	ocrpt_part_cb_data *ptr;

	if (!o)
		return false;

	if (!func)
		return false;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_part_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	o->part_iteration_callbacks = ocrpt_list_append(o->part_iteration_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_add_precalculation_done_cb(opencreport *o, ocrpt_cb func, void *data) {
	ocrpt_cb_data *ptr;

	if (!o)
		return false;

	if (!func)
		return false;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_cb_data));
	if (!ptr)
		return false;

	ptr->func = func;
	ptr->data = data;

	o->precalc_done_callbacks = ocrpt_list_append(o->precalc_done_callbacks, ptr);

	return true;
}
