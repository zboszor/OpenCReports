/*
 * Expression tree utilities
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _EXPRUTIL_H_
#define _EXPRUTIL_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <mpfr.h>
#include "opencreport.h"

/*
 * Used as both main type of an expression and subtype
 */
enum ocrpt_expr_type {
	/*
	 * Error in parsing or evaluation
	 */
	OCRPT_EXPR_ERROR,

	/*
	 * Constants or can be evaluated early
	 */
	OCRPT_EXPR_STRING,
	OCRPT_EXPR_NUMBER,
	OCRPT_EXPR_DATETIME,
	OCRPT_EXPR_MVAR,

	/* Internal variables */

	/*
	 * Internal variables, identifiers resolved from queries and
	 * general expressions. These are evaluated on a row-by-row
	 * basis from the recordset.
	 */
	OCRPT_EXPR_RVAR,
	OCRPT_EXPR_VVAR,
	OCRPT_EXPR_IDENT,
	OCRPT_EXPR,

	/*
	 * Flattened values and expressions
	 *
	 * Used by the executor that reference constants and
	 * other flattened expressions.
	 */
	OCRPT_NUMBER_VALUE,
	OCRPT_FLAT_EXPR,
};

/*
 * Flattened ops for (flattened) expressions
 * "type" indicates the underlying array
 *    i.e.: constants, various identifiers and expressions
 * "idx" is the array index into the array indicated by "type"
 */
struct ocrpt_flat_expr {
	enum ocrpt_expr_type type;
	uint32_t idx;
};
typedef struct ocrpt_flat_expr ocrpt_flat_expr;

struct ocrpt_united_expr {
	enum ocrpt_expr_type type;
	union {
		ocrpt_expr *op;
		ocrpt_flat_expr fop;
	};
};
typedef struct ocrpt_united_expr ocrpt_united_expr;

struct ocrpt_expr {
	enum ocrpt_expr_type type;
	uint32_t n_ops;
	union {
		/* Original lexer token and computed string value for expression */
		const char *string;
		/* Converted numeric constant or computed numeric value for expression */
		mpfr_t number;
		/* Datetime value */
		struct tm datetime;
	};
	union {
		/*
		 * Identifiers: computed report variables,
		 * environment variables, query fields
		 */
		struct {
			const char *query;
			const char *name;
			enum ocrpt_expr_type ident_type;
			bool dotprefixed;
		};

		struct {
			const char *fname;
			enum ocrpt_expr_type expr_type;
			ocrpt_united_expr *uops;
		};
	};
};

#endif
