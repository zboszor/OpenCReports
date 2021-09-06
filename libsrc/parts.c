/*
 * Report part utilities
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
	ocrpt_list *lr;

	if (!p)
		p = ocrpt_part_new(o);

	if (!p->lastrow)
		ocrpt_part_new_row(o, p);
	lr = p->lastrow;
	lr->data = ocrpt_list_end_append((ocrpt_list *)lr->data, &p->lastcol_in_lastrow, r);
	r->part = p;
}

DLL_EXPORT_SYM void ocrpt_part_free(opencreport *o, ocrpt_part *p) {
	ocrpt_list *row;

	for (row = p->rows; row; row = row->next) {
		ocrpt_list *col;

		for (col = (ocrpt_list *)row->data; col; col = col->next)
			ocrpt_report_free(o, (ocrpt_report *)col->data);

		ocrpt_list_free((ocrpt_list *)row->data);
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

DLL_EXPORT_SYM ocrpt_report *ocrpt_report_new(void) {
	ocrpt_report *r = ocrpt_mem_malloc(sizeof(ocrpt_report));
	memset(r, 0, sizeof(ocrpt_report));
	return r;
}

DLL_EXPORT_SYM void ocrpt_report_free(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->variables; ptr; ptr = ptr->next)
		ocrpt_variable_free(o, r, (ocrpt_var *)ptr->data);
	ocrpt_list_free(r->variables);

	ocrpt_mem_free(r->query);
	ocrpt_mem_free(r);
}

DLL_EXPORT_SYM void ocrpt_report_set_main_query(ocrpt_report *r, const char *query) {
	r->query = ocrpt_mem_strdup(query);
}

DLL_EXPORT_SYM void ocrpt_report_resolve_variables(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->variables; ptr; ptr = ptr->next) {
		ocrpt_var *v = (ocrpt_var *)ptr->data;
		ocrpt_expr_resolve(o, r, v->expr);
	}
}

DLL_EXPORT_SYM void ocrpt_report_evaluate_variables(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->variables; ptr; ptr = ptr->next) {
		ocrpt_var *v = (ocrpt_var *)ptr->data;
		ocrpt_expr_eval(o, r, v->expr);
	}
}
