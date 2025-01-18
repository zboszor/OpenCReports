/*
 * Variable utilities
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _VARIABLES_H_
#define _VARIABLES_H_

#include <opencreport.h>

struct ocrpt_var {
	ocrpt_report *r;
	const char *name;
	char *br_name;
	ocrpt_break *br;
	ocrpt_expr *precalculate_expr;
	ocrpt_expr *baseexpr;
	ocrpt_expr *intermedexpr;
	ocrpt_expr *intermed2expr;
	ocrpt_expr *resultexpr;
	ocrpt_list *precalc_results;
	ocrpt_list *precalc_rptr;
	unsigned int break_index:16;
	enum ocrpt_var_type type:4;
	enum ocrpt_result_type basetype:2;
	bool precalculate:1;
};

void ocrpt_variable_reset(ocrpt_var *v);
void ocrpt_variables_add_precalculated_results(ocrpt_report *r, ocrpt_list *brl_start, bool last_row);
void ocrpt_variables_advance_precalculated_results(ocrpt_report *r, ocrpt_list *brl_start);
void ocrpt_variable_free(ocrpt_var *var);
void ocrpt_variables_free(ocrpt_report *r);

#endif
