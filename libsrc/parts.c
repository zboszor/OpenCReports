/*
 * Report part utilities
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
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

DLL_EXPORT_SYM ocrpt_part *ocrpt_part_new(opencreport *o) {
	ocrpt_part *p;

	p = ocrpt_mem_malloc(sizeof(ocrpt_part));
	memset(p, 0, sizeof(ocrpt_part));

	o->parts = ocrpt_list_end_append(o->parts, &o->last_part, p);
	return p;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_part_new_row(opencreport *o, ocrpt_part *p) {
	p->rows = ocrpt_list_end_append(p->rows, &p->lastrow, NULL);
	p->lastcol_in_lastrow = NULL;
	return p->lastrow;
}

DLL_EXPORT_SYM void ocrpt_part_append_report(opencreport *o, ocrpt_part *p, ocrpt_report *r) {
	ocrpt_list *lr, *cbl;

	if (!p)
		p = ocrpt_part_new(o);

	if (!p->lastrow)
		ocrpt_part_new_row(o, p);
	lr = p->lastrow;
	lr->data = ocrpt_list_end_append((ocrpt_list *)lr->data, &p->lastcol_in_lastrow, r);
	r->part = p;

	for (cbl = o->report_added_callbacks; cbl; cbl = cbl->next) {
		ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

		cbd->func(o, r, cbd->data);
	}
}

DLL_EXPORT_SYM void ocrpt_part_free(opencreport *o, ocrpt_part *p) {
	ocrpt_list *row;

	for (row = p->rows; row; row = row->next) {
		ocrpt_list *col;

		for (col = (ocrpt_list *)row->data; col; col = col->next)
			ocrpt_report_free(o, (ocrpt_report *)col->data);

		ocrpt_list_free((ocrpt_list *)row->data);
		row->data = NULL;
	}
	ocrpt_list_free(p->rows);
	p->rows = NULL;
	p->lastrow = NULL;
	p->lastcol_in_lastrow = NULL;

	ocrpt_mem_free(p->path);
#if 0
	if (p->allocated)
		ocrpt_mem_free(p->xmlbuf);
#endif
	ocrpt_mem_free(p);
}

DLL_EXPORT_SYM void ocrpt_parts_free(opencreport *o) {
	while (o->parts) {
		ocrpt_part *p = (ocrpt_part *)o->parts->data;
		ocrpt_part_free(o, p);
		o->parts = ocrpt_list_remove(o->parts, p);
	}
}

DLL_EXPORT_SYM ocrpt_report *ocrpt_report_new(opencreport *o) {
	ocrpt_report *r = ocrpt_mem_malloc(sizeof(ocrpt_report));
	memset(r, 0, sizeof(ocrpt_report));
	r->query_rownum = ocrpt_expr_parse(o, r, "rownum()", NULL);
	ocrpt_expr_resolve(o, r, r->query_rownum);
	return r;
}

DLL_EXPORT_SYM void ocrpt_report_free(opencreport *o, ocrpt_report *r) {
	ocrpt_list *l;

	ocrpt_variables_free(o, r);
	ocrpt_breaks_free(o, r);
	ocrpt_list_free(r->exprs);
	r->exprs = r->exprs_last = NULL;
	ocrpt_list_free_deep(r->start_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->done_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(r->newrow_callbacks, ocrpt_mem_free);

	for (l = r->delayed_results; l; l = l->next) {
		ocrpt_result *rs_array = (ocrpt_result *)l->data;
		int i;

		for (i = 0; i < r->num_expressions; i++)
			ocrpt_result_free_data(&rs_array[i]);

		ocrpt_mem_free(rs_array);
	}
	ocrpt_list_free(r->delayed_results);

	ocrpt_expr_free(o, r, r->query_rownum);
	ocrpt_mem_free(r->query);
	ocrpt_mem_free(r);
}

DLL_EXPORT_SYM bool ocrpt_report_validate(opencreport *o, ocrpt_report *r) {
	ocrpt_list *parts_ptr;

	if (!o || !r)
		return false;

	for (parts_ptr = o->parts; parts_ptr; parts_ptr = parts_ptr->next) {
		ocrpt_part *p = (ocrpt_part *)parts_ptr->data;
		ocrpt_list *row;

		for (row = p->rows; row; row = row->next) {
			ocrpt_list *col;

			for (col = (ocrpt_list *)row->data; col; col = col->next) {
				if (r == col->data)
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

unsigned int ocrpt_report_push_delayed_results(opencreport *o, ocrpt_report *r, ocrpt_list **last) {
	ocrpt_list *drl, *el;
	ocrpt_result *rs_array;
	unsigned int row;

	fprintf(stderr, "%s: r %p num_exprs %d delayed_result_rows %d (%" PRIdFAST32 ")\n", __func__, r, r->num_expressions, r->delayed_result_rows, ocrpt_list_length(r->delayed_results));

	for (row = 0, drl = r->delayed_results; drl && row < r->delayed_result_rows; row++, drl = drl->next)
		;

	if (!drl) {
		rs_array = ocrpt_mem_malloc(r->num_expressions * sizeof(ocrpt_result));
		memset(rs_array, 0, r->num_expressions * sizeof(ocrpt_result));
		drl = r->delayed_results = ocrpt_list_end_append(r->delayed_results, last, rs_array);
	} else {
		*last = drl;
	}

	rs_array = (ocrpt_result *)drl->data;
	r->delayed_result_rows++;

	for (el = r->exprs; el; el = el->next) {
		ocrpt_expr *e = (ocrpt_expr *)el->data;

		if (e->result_index <= r->num_expressions) {
			ocrpt_result_copy(o, &rs_array[e->result_index], e->result[o->residx]);
			ocrpt_result_print(&rs_array[e->result_index]);
		}
	}

	return row;
}
