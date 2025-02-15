/*
 * Expression tree utilities
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _EXPRUTIL_H_
#define _EXPRUTIL_H_

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <opencreport.h>

/*
 * The first part of the definitions are the same as enum ocrpt_result_type
 */
enum ocrpt_expr_type {
	/*
	 * Constants
	 */
	OCRPT_EXPR_STRING,
	OCRPT_EXPR_NUMBER,
	OCRPT_EXPR_DATETIME,

	/*
	 * Error in parsing or evaluation
	 */
	OCRPT_EXPR_ERROR,

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
	OCRPT_EXPR_IDENT,
	OCRPT_EXPR_VVAR,
	OCRPT_EXPR,
};

struct ocrpt_expr {
	opencreport *o;
	ocrpt_report *r;
	struct ocrpt_result *result[OCRPT_EXPR_RESULTS];
	struct ocrpt_result *delayed_result;
	union {
		/*
		 * Identifiers: computed report variables,
		 * environment variables, query fields
		 */
		struct {
			ocrpt_string *query;
			ocrpt_string *name;
			ocrpt_var *var;
		};

		struct {
			const ocrpt_function *func;
			ocrpt_query *q; /* set if func is rownum() */
			ocrpt_expr **ops;
			uint32_t n_ops;
		};
	};
	/*
	 * Set to a valid pointer if:
	 * - func is brrownum("break"), or
	 * - the expression contains a reference to a VVAR
	 *   with resetonbreak="break"
	 */
	struct ocrpt_break *br;
	/*
	 * Pointer for the "r.value" internal variable reference.
	 * Valid for any expression in a <field> in the report XML except for value="..."
	 */
	ocrpt_expr *rvalue;
	ocrpt_expr *format;
	unsigned int result_index;
	enum ocrpt_expr_type type:4;
	bool result_index_set:1;
	bool result_owned0:1;
	bool result_owned1:1;
	bool result_owned2:1;
	bool result_evaluated0:1;
	bool result_evaluated1:1;
	bool result_evaluated2:1;
	bool parenthesized:1;
	bool dotprefixed:1;
	bool delayed:1;
	bool iterative:1;
	bool iterative_init:1;
	bool iterative_start_with_init:1;
};

#define ocrpt_expr_is_const(x) ((x)->type == OCRPT_EXPR_STRING || (x)->type == OCRPT_EXPR_NUMBER || (x)->type == OCRPT_EXPR_DATETIME)
#define ocrpt_expr_is_dconst(x) ((x)->type == OCRPT_EXPR_NUMBER)
#define ocrpt_expr_is_sconst(x) ((x)->type == OCRPT_EXPR_STRING)
#define ocrpt_expr_is_dtconst(x) ((x)->type == OCRPT_EXPR_DATETIME)

ocrpt_expr *newblankexpr(opencreport *o, ocrpt_report *r, enum ocrpt_expr_type type, uint32_t n_ops);
ocrpt_expr *ocrpt_newstring(opencreport *o, ocrpt_report *r, const char *string);
ocrpt_expr *ocrpt_newstring_add_to_list(opencreport *o, ocrpt_report *r, const char *string);
void ocrpt_expr_free_internal(ocrpt_expr *e, bool free_from_list);

void ocrpt_expr_print_internal(ocrpt_expr *e, ocrpt_printf_func func);
void ocrpt_expr_result_deep_print_worker(ocrpt_expr *e, ocrpt_printf_func func);
void ocrpt_expr_resolve_worker(ocrpt_expr *e, ocrpt_expr *orig_e, ocrpt_var *var, int32_t varref_exclude_mask, bool warn);
static inline void ocrpt_expr_resolve_nowarn(ocrpt_expr *e) {
	if (!e)
		return;
	ocrpt_expr_resolve_worker(e, e, NULL, 0, false);
}

#define get_string(o, expr) { \
					expr##_e = ocrpt_expr_parse(o, (char *)expr, NULL); \
					ocrpt_expr_resolve_nowarn(expr##_e); \
					const ocrpt_string *expr##_ss = ocrpt_expr_get_string(expr##_e); \
					expr##_s = expr##_ss ? expr##_ss->str : (char *)expr; \
				}

#define get_int(o, expr) { \
					expr##_e = ocrpt_expr_parse(o, (char *)expr, NULL); \
					ocrpt_expr_resolve_nowarn(expr##_e); \
					expr##_i = ocrpt_expr_get_long(expr##_e); \
				}

void ocrpt_expr_resolve_exclude(ocrpt_expr *e, int32_t varref_exclude_mask);
bool ocrpt_expr_references(ocrpt_expr *e, int32_t varref_include_mask, uint32_t *varref_vartype_mask);
void ocrpt_expr_eval_worker(ocrpt_expr *e, ocrpt_expr *orig_e, ocrpt_var *var);
bool ocrpt_expr_get_precalculate(ocrpt_expr *e);
void ocrpt_report_expressions_add_delayed_results(ocrpt_report *r);
void ocrpt_expr_init_iterative_results(ocrpt_expr *e, enum ocrpt_result_type type);
void ocrpt_expr_set_plain_iterative_to_null(ocrpt_report *r);

static inline void ocrpt_expr_set_result_owned(ocrpt_expr *e, unsigned int which, bool owned) {
	switch (which) {
	case 0: e->result_owned0 = owned; break;
	case 1: e->result_owned1 = owned; break;
	case 2: e->result_owned2 = owned; break;
	default: assert(!"unreachable"); abort(); break;
	}
}

static inline bool ocrpt_expr_get_result_owned(ocrpt_expr *e, unsigned int which) {
	switch (which) {
	case 0: return e->result_owned0;
	case 1: return e->result_owned1;
	case 2: return e->result_owned2;
	default: assert(!"unreachable"); abort(); break;
	}
}

static inline void ocrpt_expr_set_result_evaluated(ocrpt_expr *e, unsigned int which, bool evaluated) {
	switch (which) {
	case 0: e->result_evaluated0 = evaluated; break;
	case 1: e->result_evaluated1 = evaluated; break;
	case 2: e->result_evaluated2 = evaluated; break;
	default: assert(!"unreachable"); abort(); break;
	}
}

static inline bool ocrpt_expr_get_result_evaluated(ocrpt_expr *e, unsigned int which) {
	switch (which) {
	case 0: return e->result_evaluated0;
	case 1: return e->result_evaluated1;
	case 2: return e->result_evaluated2;
	default: assert(!"unreachable"); abort(); break;
	}
}

static inline int ocrpt_expr_next_residx(int residx) {
	int val = residx + 1;
	if (val >= OCRPT_EXPR_RESULTS)
		val = 0;
	return val;
}

static inline int ocrpt_expr_prev_residx(int residx) {
	int val = residx - 1;
	if (val < 0)
		val = OCRPT_EXPR_RESULTS - 1;
	return val;
}

#define EXPR_VALID(e) ((e) && (e)->result[(e)->o->residx])
#define EXPR_RESULT(e) ((e)->result[(e)->o->residx])
#define EXPR_NEXT_RESULT(e) ((e)->result[ocrpt_expr_next_residx((e)->o->residx)])
#define EXPR_PREV_RESULT(e) ((e)->result[ocrpt_expr_prev_residx((e)->o->residx)])
#define EXPR_TYPE(e) (EXPR_RESULT(e)->type)
#define EXPR_ISNULL(e) (EXPR_RESULT(e)->isnull)
#define EXPR_VALID_NOT_NULL(e) ((e) && EXPR_RESULT(e) && !EXPR_ISNULL(e))
#define EXPR_VALID_NUMERIC(e) ((e) && EXPR_RESULT(e) && EXPR_TYPE(e) == OCRPT_RESULT_NUMBER && EXPR_RESULT(e)->number_initialized)
#define EXPR_VALID_NUMERIC_MAYBE_UNINITIALIZED(e) ((e) && EXPR_RESULT(e) && EXPR_TYPE(e) == OCRPT_RESULT_NUMBER)
#define EXPR_NUMERIC(e) (EXPR_RESULT(e)->number)
#define EXPR_RNDMODE(e) ((e)->o->rndmode)
#define EXPR_VALID_STRING(e) ((e) && EXPR_RESULT(e) && EXPR_TYPE(e) == OCRPT_RESULT_STRING && EXPR_RESULT(e)->string)
#define EXPR_VALID_STRING_MAYBE_UNINITIALIZED(e) ((e) && EXPR_RESULT(e) && EXPR_TYPE(e) == OCRPT_RESULT_STRING)
#define EXPR_STRING(e) (EXPR_RESULT(e)->string)
#define EXPR_STRING_OWNED(e) (EXPR_RESULT(e)->string_owned)
#define EXPR_STRING_VAL(e) (EXPR_STRING(e)->str)
#define EXPR_STRING_LEN(e) (EXPR_STRING(e)->len)
#define EXPR_STRING_ALEN(e) (EXPR_STRING(e)->allocated_len)
#define EXPR_VALID_DATETIME(e) ((e) && EXPR_RESULT(e) && EXPR_TYPE(e) == OCRPT_RESULT_DATETIME && (EXPR_RESULT(e)->date_valid || EXPR_RESULT(e)->time_valid))
#define EXPR_VALID_DATETIME_MAYBE_UNINITIALIZED(e) ((e) && EXPR_RESULT(e) && EXPR_TYPE(e) == OCRPT_RESULT_DATETIME)
#define EXPR_DATETIME(e) (EXPR_RESULT(e)->datetime)
#define EXPR_DATE_VALID(e) (EXPR_RESULT(e)->date_valid)
#define EXPR_TIME_VALID(e) (EXPR_RESULT(e)->time_valid)
#define EXPR_INTERVAL(e) (EXPR_RESULT(e)->interval)
#define EXPR_DAY_CARRY(e) (EXPR_RESULT(e)->day_carry)
#define EXPR_VALID_ERROR(e) ((e) && EXPR_RESULT(e) && EXPR_TYPE(e) == OCRPT_RESULT_ERROR && EXPR_STRING(e))
#define EXPR_VALID_ERROR_MAYBE_UNINITIALIZED(e) ((e) && EXPR_RESULT(e) && EXPR_TYPE(e) == OCRPT_RESULT_ERROR)

void ocrpt_result_free_data(ocrpt_result *r);

#endif
