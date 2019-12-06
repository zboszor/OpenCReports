/*
 * Expression tree utilities
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _EXPRUTIL_H_
#define _EXPRUTIL_H_

#include <stdbool.h>
#include <stdint.h>

#include <opencreport.h>
#include "functions.h"

/*
 * Used as both main type of an expression and subtype
 * The first part of the definitions are the same as
 * enum ocrpt_result_type in the public opencreport.h
 */
enum ocrpt_expr_type {
	/*
	 * Error in parsing or evaluation
	 */
	OCRPT_EXPR_ERROR,

	/*
	 * Constants
	 */
	OCRPT_EXPR_STRING,
	OCRPT_EXPR_NUMBER,
	OCRPT_EXPR_DATETIME,

	/*
	 * Pre-set variable that can be evaluated early
	 */
	OCRPT_EXPR_MVAR,

	/*
	 * Internal variables, identifiers resolved from queries and
	 * general expressions. These are evaluated on a row-by-row
	 * basis from the recordset.
	 */
	OCRPT_EXPR_RVAR,
	OCRPT_EXPR_VVAR,
	OCRPT_EXPR_IDENT,
	OCRPT_EXPR,
};

#define ocrpt_expr_is_const(x) ((x)->type == OCRPT_EXPR_STRING || (x)->type == OCRPT_EXPR_NUMBER || (x)->type == OCRPT_EXPR_DATETIME)

struct ocrpt_expr {
	enum ocrpt_expr_type type;
	bool result_owned[2];
	struct ocrpt_result *result[2];
	union {
		/*
		 * Identifiers: computed report variables,
		 * environment variables, query fields
		 */
		struct {
			const char *query;
			const char *name;
			bool dotprefixed;
		};

		struct {
			const ocrpt_function *func;
			ocrpt_expr **ops;
			uint32_t n_ops;
		};
	};
};

ocrpt_result *ocrpt_expr_make_error_result(opencreport *o, ocrpt_expr *e, const char *error);

#endif
