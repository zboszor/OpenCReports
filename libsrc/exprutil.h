/*
 * Expression tree utilities
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _EXPRUTIL_H_
#define _EXPRUTIL_H_

#include <stdbool.h>
#include <stdint.h>

#include <opencreport.h>

#define ocrpt_expr_is_const(x) ((x)->type == OCRPT_EXPR_STRING || (x)->type == OCRPT_EXPR_NUMBER || (x)->type == OCRPT_EXPR_DATETIME)
#define ocrpt_expr_is_dconst(x) ((x)->type == OCRPT_EXPR_NUMBER)
#define ocrpt_expr_is_sconst(x) ((x)->type == OCRPT_EXPR_STRING)
#define ocrpt_expr_is_dtconst(x) ((x)->type == OCRPT_EXPR_DATETIME)

ocrpt_expr *newblankexpr(opencreport *o, ocrpt_report *r, enum ocrpt_expr_type type, uint32_t n_ops);

void ocrpt_expr_resolve_worker(opencreport *o, ocrpt_report *r, ocrpt_expr *e, ocrpt_expr *orig_e, ocrpt_var *var, int32_t varref_exclude_mask);
void ocrpt_expr_eval_worker(opencreport *o, ocrpt_report *r, ocrpt_expr *e, ocrpt_expr *orig_e, ocrpt_var *var);
bool ocrpt_expr_get_precalculate(opencreport *o, ocrpt_expr *e);
void ocrpt_report_expressions_add_delayed_results(opencreport *o, ocrpt_report *r);

#endif
