/*
 * Expression tree utilities
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
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

ocrpt_expr *newblankexpr(enum ocrpt_expr_type type, uint32_t n_ops);

#endif
