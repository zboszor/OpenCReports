/*
 * Expression tree utilities
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <alloca.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpfr.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "environment.h"
#include "exprutil.h"
#include "functions.h"
#include "variables.h"
#include "datasource.h"
#include "parts.h"

void ocrpt_result_print_internal(ocrpt_result *r, ocrpt_printf_func func);

ocrpt_expr *newblankexpr(opencreport *o, ocrpt_report *r, enum ocrpt_expr_type type, uint32_t n_ops) {
	ocrpt_expr *e;

	if (!o) {
		if (r)
			o = r->o;
		else
			return NULL;
	} else if (r && o != r->o)
		return NULL;

	e = ocrpt_mem_malloc(sizeof(ocrpt_expr));
	if (!e)
		return NULL;

	memset(e, 0, sizeof(ocrpt_expr));
	e->o = o;
	e->r = r;
	e->ops = (n_ops > 0 ? ocrpt_mem_malloc(n_ops * sizeof(ocrpt_expr *)) : NULL);

	if ((n_ops > 0) && !e->ops) {
		ocrpt_expr_free(e);
		return NULL;
	}

	e->type = type;
	e->n_ops = n_ops;

	return e;
}

ocrpt_expr *ocrpt_newstring(opencreport *o, ocrpt_report *r, const char *string) {
	ocrpt_expr *e = newblankexpr(o, r, OCRPT_EXPR_STRING, 0);

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);
	ocrpt_mem_string_append_printf(EXPR_STRING(e), "%s", string ? string : "");
	EXPR_STRING_OWNED(e) = true;
	EXPR_NEXT_RESULT(e) = EXPR_RESULT(e);
	EXPR_PREV_RESULT(e) = EXPR_RESULT(e);

	if (r && !r->executing && !r->dont_add_exprs) {
		r->exprs = ocrpt_list_end_append(r->exprs, &r->exprs_last, e);
		e->result_index = r->num_expressions++;
		e->result_index_set = true;
	}

	e->expr_string = ocrpt_mem_strdup(string ? string : "");

	return e;
}

ocrpt_expr *ocrpt_newstring_add_to_list(opencreport *o, ocrpt_report *r, const char *string) {
	ocrpt_expr *e = ocrpt_newstring(o, r, string);

	o->exprs = ocrpt_list_end_append(o->exprs, &o->exprs_last, e);

	return e;
}

static void ocrpt_expr_print_worker(ocrpt_expr *e, int depth, const char *delimiter, ocrpt_string *str) {
	ocrpt_result *result;
	int i;

	if (!e)
		return;

	result = EXPR_RESULT(e);

	switch (e->type) {
	case OCRPT_EXPR_ERROR:
		ocrpt_mem_string_append_printf(str, "(ERROR)%s", result ? result->string->str : "NULL");
		break;
	case OCRPT_EXPR_STRING:
		ocrpt_mem_string_append_printf(str, "(string)%s", result ? (result->isnull ? "NULL" : (result->string ? result->string->str : "NULL")) : "NULL");
		break;
	case OCRPT_EXPR_DATETIME: {
		ocrpt_string *tmp = ocrpt_mem_string_new_with_len(NULL, 128);
		if (!result->isnull) {
			if (result->interval) {
				if (result->datetime.tm_year)
					ocrpt_mem_string_append_printf(tmp, "%d years", result->datetime.tm_year);
				if (result->datetime.tm_mon)
					ocrpt_mem_string_append_printf(tmp, "%s%d mons", tmp->len ? " " : "", result->datetime.tm_mon);
				if (result->datetime.tm_mday)
					ocrpt_mem_string_append_printf(tmp, "%s%d days", tmp->len ? " " : "", result->datetime.tm_mday);
				if (result->datetime.tm_hour)
					ocrpt_mem_string_append_printf(tmp, "%s%d hours", tmp->len ? " " : "", result->datetime.tm_hour);
				if (result->datetime.tm_min)
					ocrpt_mem_string_append_printf(tmp, "%s%d mins", tmp->len ? " " : "", result->datetime.tm_min);
				if (result->datetime.tm_sec)
					ocrpt_mem_string_append_printf(tmp, "%s%d seconds", tmp->len ? " " : "", result->datetime.tm_sec);
			} else if (result->date_valid && result->time_valid)
				strftime(tmp->str, 128, "%F %T", &result->datetime);
			else if (result->date_valid)
				strftime(tmp->str, 128, "%F", &result->datetime);
			else if (result->time_valid)
				strftime(tmp->str, 128, "%T", &result->datetime);
		}
		ocrpt_mem_string_append_printf(str, "(datetime)%s", result ? (result->isnull ? "NULL" : tmp->str) : NULL);
		ocrpt_mem_string_free(tmp, true);
		break;
	}
	case OCRPT_EXPR_NUMBER:
		if (!result || result->isnull)
			ocrpt_mem_string_append_printf(str, "NULL");
		else {
			size_t len = mpfr_snprintf(NULL, 0, "%RF", result->number);
			ocrpt_string *newstr = ocrpt_mem_string_new_with_len(NULL, len);
			mpfr_snprintf(newstr->str, newstr->allocated_len, "%RF", result->number);
			newstr->len = len;
			ocrpt_mem_string_append_len(str, newstr->str, newstr->len);
			ocrpt_mem_string_free(newstr, true);
		}
		break;
	case OCRPT_EXPR_MVAR:
		ocrpt_mem_string_append_printf(str, "m.'%s'", e->name->str);
		break;
	case OCRPT_EXPR_RVAR:
		ocrpt_mem_string_append_printf(str, "r.'%s'", e->name->str);
		break;
	case OCRPT_EXPR_VVAR:
		ocrpt_mem_string_append_printf(str, "v.'%s'", e->name->str);
		break;
	case OCRPT_EXPR_IDENT:
		if (e->query)
			ocrpt_mem_string_append_printf(str, "'%s'.'%s'", e->query->str, e->name->str);
		else
			ocrpt_mem_string_append_printf(str, ".'%s'", e->name->str);
		break;
	case OCRPT_EXPR:
		ocrpt_mem_string_append_printf(str, "%s(", e->func->fname);
		for (i = 0; i < e->n_ops; i++) {
			if (i > 0)
				ocrpt_mem_string_append_printf(str, ",");
			ocrpt_expr_print_worker(e->ops[i], depth + 1, delimiter, str);
		}
		ocrpt_mem_string_append_printf(str, ")");
		break;
	}

	if (depth == 0)
		ocrpt_mem_string_append_printf(str, "%s", delimiter);
}

void ocrpt_expr_print_internal(ocrpt_expr *e, ocrpt_printf_func func) {
	if (!e || !func)
		return;

	ocrpt_string *str = ocrpt_mem_string_new_with_len(NULL, 256);
	ocrpt_expr_print_worker(e, 0, "\n", str);
	func("%s", str->str);
	ocrpt_mem_string_free(str, true);
}

DLL_EXPORT_SYM void ocrpt_expr_print(ocrpt_expr *e) {
	ocrpt_expr_print_internal(e, ocrpt_std_printf);
}

void ocrpt_expr_result_deep_print_worker(ocrpt_expr *e, ocrpt_printf_func func) {
	if (!e || !func)
		return;

	ocrpt_string *str = ocrpt_mem_string_new_with_len(NULL, 256);
	int i;

	switch (e->type) {
	case OCRPT_EXPR:
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_result_deep_print_worker(e->ops[i], func);
		break;
	case OCRPT_EXPR_VVAR:
		assert(e->var);
		if (e->var->baseexpr)
			ocrpt_expr_result_deep_print_worker(e->var->baseexpr, func);
		if (e->var->ignoreexpr)
			ocrpt_expr_result_deep_print_worker(e->var->ignoreexpr, func);
		if (e->var->intermedexpr)
			ocrpt_expr_result_deep_print_worker(e->var->intermedexpr, func);
		if (e->var->intermed2expr)
			ocrpt_expr_result_deep_print_worker(e->var->intermed2expr, func);
		ocrpt_expr_result_deep_print_worker(e->var->resultexpr, func);
		break;
	default:
		break;
	}

	ocrpt_expr_print_worker(e, 0, " - ", str);
	func("%s\n", str->str);
	ocrpt_mem_string_free(str, true);
	ocrpt_result_print_internal(EXPR_RESULT(e), func);
}

DLL_EXPORT_SYM void ocrpt_expr_result_deep_print(ocrpt_expr *e) {
	ocrpt_expr_result_deep_print_worker(e, ocrpt_std_printf);
}

DLL_EXPORT_SYM void ocrpt_result_copy(ocrpt_result *dst, ocrpt_result *src) {
	if (!src || !dst || dst->o != src->o)
		return;

	dst->type = src->type;
	dst->isnull = src->isnull;

	if (!src->isnull) {
		switch (src->type) {
		case OCRPT_RESULT_ERROR:
		case OCRPT_RESULT_STRING:
			if (!src->string) {
				dst->isnull = true;
				break;
			}
			if (!dst->string) {
				dst->string = ocrpt_mem_string_new_with_len(src->string->str, src->string->len);
				dst->string_owned = true;
			} else {
				/* Probably a new ocrpt_mem_string_copy() is needed */
				ocrpt_mem_string_resize(dst->string, src->string->len);
				memcpy(dst->string->str, src->string->str, src->string->len);
				dst->string->len = src->string->len;
				dst->string->str[dst->string->len] = 0;
			}
			break;
		case OCRPT_RESULT_NUMBER:
			if (!dst->number_initialized) {
				mpfr_init2(dst->number, src->o->prec);
				dst->number_initialized = true;
			}
			mpfr_set(dst->number, src->number, src->o->rndmode);
			break;
		case OCRPT_RESULT_DATETIME:
			dst->datetime = src->datetime;
			dst->date_valid = src->date_valid;
			dst->time_valid = src->time_valid;
			dst->interval = src->interval;
			dst->day_carry = src->day_carry;
			break;
		}
	}
}

void ocrpt_result_print_internal(ocrpt_result *r, ocrpt_printf_func func) {
	if (!r || !func)
		return;

	switch (r->type) {
	case OCRPT_RESULT_ERROR:
		func("(ERROR)%s\n", (r->isnull || !r->string) ? "NULL" : r->string->str);
		break;
	case OCRPT_RESULT_STRING:
		func("(string)%s\n", (r->isnull || !r->string) ? "NULL" : r->string->str);
		break;
	case OCRPT_RESULT_DATETIME: {
		if (!r->isnull) {
			if (r->interval) {
				ocrpt_string *str = ocrpt_mem_string_new_with_len("", 0);
				if (r->datetime.tm_year)
					ocrpt_mem_string_append_printf(str, "%d years", r->datetime.tm_year);
				if (r->datetime.tm_mon)
					ocrpt_mem_string_append_printf(str, "%s%d mons", str->len ? " " : "", r->datetime.tm_mon);
				if (r->datetime.tm_mday)
					ocrpt_mem_string_append_printf(str, "%s%d days", str->len ? " " : "", r->datetime.tm_mday);
				if (r->datetime.tm_hour)
					ocrpt_mem_string_append_printf(str, "%s%d hours", str->len ? " " : "", r->datetime.tm_hour);
				if (r->datetime.tm_min)
					ocrpt_mem_string_append_printf(str, "%s%d mins", str->len ? " " : "", r->datetime.tm_min);
				if (r->datetime.tm_sec)
					ocrpt_mem_string_append_printf(str, "%s%d seconds", str->len ? " " : "", r->datetime.tm_sec);
				func("(datetime)%s\n", str->str);
				ocrpt_mem_string_free(str, true);
			} else {
				char tmp[128] = "";
				if (r->date_valid && r->time_valid)
					strftime(tmp, sizeof(tmp), "%F %T", &r->datetime);
				else if (r->date_valid)
					strftime(tmp, sizeof(tmp), "%F", &r->datetime);
				else if (r->time_valid)
					strftime(tmp, sizeof(tmp), "%T", &r->datetime);
				func("(datetime)%s\n", tmp);
			}
		} else
			func("(datetime)NULL\n");
		break;
	}
	case OCRPT_RESULT_NUMBER:
		if (r->isnull)
			func("(number)NULL\n");
		else {
			size_t len = mpfr_snprintf(NULL, 0, "(number)%RF\n", r->number);
			ocrpt_string *s = ocrpt_mem_string_new_with_len("", len + 16);
			mpfr_snprintf(s->str, len + 1, "(number)%RF\n", r->number);
			func("%s", s->str);
			ocrpt_mem_string_free(s, true);
		}
		break;
	}
}

DLL_EXPORT_SYM void ocrpt_result_print(ocrpt_result *r) {
	ocrpt_result_print_internal(r, ocrpt_std_printf);
}

static void ocrpt_expr_nodes_worker(ocrpt_expr *e, int32_t *nodes) {
	(*nodes)++;

	if (e->type == OCRPT_EXPR) {
		int32_t i;

		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_nodes_worker(e->ops[i], nodes);
	}
}

DLL_EXPORT_SYM int32_t ocrpt_expr_nodes(ocrpt_expr *e) {
	if (!e)
		return 0;

	int32_t nodes = 0;
	ocrpt_expr_nodes_worker(e, &nodes);

	return nodes;
}

/*
 * Optimize the expression as much as possible
 * Implememented ideas:
 * 1. If all operands of a function are constants,
 *    compute it and replace the function call with the constant.
 *    Free the operands to trim the nodes in the expression.
 * 2. If a function (operators are handled via functions) is
 *    commutative and associative, reorder the operands so
 *    constants are first, non-constants are second, separate them
 *    into two subexpressions and re-run the optimizer on the same
 *    level. The 1st optimization steps will compute the fully
 *    constant subexpression.
 * 3. If a function's operands only contain subexpressions with
 *    the same function as their parent and this function is
 *    commutative and associative, then the subexpressions'
 *    operands can be pulled up to the same level as the parent.
 *    Enumerating array elements is a little faster then following
 *    pointers of a tree.
 * 4. Do the same as (3) for left-associative functions
 *    for the first operand unconditionally and for subsequent
 *    operands only if they are not inside parentheses.
 *    Example: (a/b)/c is the same as a/b/c but a/(b/c) is not.
 */
static void ocrpt_expr_optimize_worker(ocrpt_expr *e) {
	if (!e)
		return;

	if (e->type != OCRPT_EXPR)
		return;

	int32_t i, nconst, dconst, sconst, dtconst;
	bool try_pullup = true;

	again:

	for (i = 0, nconst = 0, dconst = 0, sconst = 0, dtconst = 0; i < e->n_ops; i++) {
		if (!ocrpt_expr_is_const(e->ops[i]))
			ocrpt_expr_optimize_worker(e->ops[i]);
		if (ocrpt_expr_is_dconst(e->ops[i]))
			dconst++, nconst++;
		if (ocrpt_expr_is_sconst(e->ops[i]))
			sconst++, nconst++;
		if (ocrpt_expr_is_dtconst(e->ops[i]))
			dtconst++, nconst++;
	}
	if (nconst == e->n_ops) {
		/*
		 * Fully constant expression, precompute it.
		 */
		if (e->func) {
			if (e->func->func && !e->func->dont_optimize) {
				e->func->func(e, e->func->user_data);
				for (i = 0; i < e->n_ops; i++)
					ocrpt_expr_free(e->ops[i]);
				ocrpt_mem_free(e->ops);
				e->n_ops = 0;
				e->ops = NULL;
				e->type = (enum ocrpt_expr_type)EXPR_TYPE(e);

				int residx = e->o->residx;
				for (i = 0; i < OCRPT_EXPR_RESULTS - 1; i++) {
					residx = ocrpt_expr_next_residx(residx);

					ocrpt_result_free_data(e->result[residx]);
					e->result[residx] = EXPR_RESULT(e);
					ocrpt_expr_set_result_owned(e, residx, false);
				}
			}
		}

		/* Nothing can be optimized further on this level. */
		return;
	}

	if (dconst > 1 && !sconst && !dtconst && e->func->n_ops == -1 && e->func->commutative && e->func->associative) {
		/*
		 * The constants in this expressions are all numeric and
		 * the function is both commutative and associative, and also
		 * accepts any number of operands.
		 *
		 * Reorder the operands so all constants are at the beginnings,
		 * split into two subexpressions, one with all constants and
		 * the other with the non-constant expressions and re-run
		 * optimization on this node.
		 */
		ocrpt_expr *split1 = newblankexpr(e->o, e->r, OCRPT_EXPR, dconst);
		ocrpt_expr *split2 = newblankexpr(e->o, e->r, OCRPT_EXPR, e->n_ops - dconst);
		int32_t nsplit1, nsplit2;

		if (!split1 || !split2) {
			ocrpt_expr_free(split1);
			ocrpt_expr_free(split2);
			return;
		}

		split1->func = e->func;
		split2->func = e->func;

		for (i = 0, nsplit1 = 0, nsplit2 = 0; i < e->n_ops; i++) {
			if (ocrpt_expr_is_dconst(e->ops[i]))
				split1->ops[nsplit1++] = e->ops[i];
			else
				split2->ops[nsplit2++] = e->ops[i];
		}

		e->ops[0] = split1;
		e->ops[1] = split2;
		e->n_ops = 2;

		goto again;
	}

	if (try_pullup && e->n_ops >= 2 && e->func->n_ops == -1 && ((e->func->commutative && e->func->associative) || e->func->left_associative)) {
		/*
		 * The expression has at least 2 operands. See if
		 * all the subexpressions has the same function.
		 * If yes, and the pull the subexpressions' operands
		 * to up to this level in two cases:
		 * 1) The function is both commutative and associative.
		 *    In this case, the operands can be in any order.
		 * 2) The function is left-associative and only the left
		 *    operand is inside parentheses. In this case,
		 *    the operands pulled up must be in the same order
		 *    as in the subexpression.
		 */
		bool sameexprs = true;
		uint32_t nchildren = 0;

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->type == OCRPT_EXPR) {
				if (e->ops[i]->func != e->func) {
					sameexprs = false;
					break;
				}
				nchildren += e->ops[i]->n_ops - 1;
			}
		}

		if ((nchildren > 0) && sameexprs) {
			ocrpt_expr **newops = ocrpt_mem_realloc(e->ops, (e->n_ops + nchildren) * sizeof(ocrpt_expr *));

			if (newops) {
				bool expr_pulled_up = false;

				e->ops = newops;

				for (i = 0; i < e->n_ops; i++) {
					if ((newops[i]->type == OCRPT_EXPR) &&
							((e->func->commutative && e->func->associative) ||
								(e->func->left_associative && (i == 0 || (i > 0 && !newops[i]->parenthesized))))) {

						ocrpt_expr *x = newops[i];
						uint32_t j;

						for (j = e->n_ops + x->n_ops - 2; j >= x->n_ops; j--)
							newops[j] = newops[j - x->n_ops + 1];

						for (j = 0; j < x->n_ops; j++) {
							newops[i + j] = x->ops[j];
							x->ops[j] = NULL;
						}

						e->n_ops += x->n_ops - 1;

						x->n_ops = 0;
						ocrpt_expr_free(x);

						expr_pulled_up = true;
					}
				}

				try_pullup = expr_pulled_up;

				goto again;
			}
		}
	}
}

DLL_EXPORT_SYM void ocrpt_expr_optimize(ocrpt_expr *e) {
	if (!e)
		return;

	ocrpt_expr_optimize_worker(e);
}

static bool ocrpt_resolve_ident(ocrpt_expr *e, ocrpt_query *q) {
	ocrpt_query_result *qr = NULL;
	ocrpt_list *ql;
	int32_t cols, i;
	bool found = false;

	/*
	 * Identifier is either non-domain qualified,
	 * or domain-qualified and it matches the query name:
	 * try to resolve the identifier name in this query.
	 */
	if (!e->query || strcmp(e->query->str, q->name) == 0) {
		/* ocrpt_query_get_result() cannot be used here, we need the whole array */
		if (!q->result)
			q->source->input->describe(q, &q->result, &q->cols);
		qr = q->result;
		cols = (q->result ? q->cols : 0);

		for (i = 0; i < cols; i++) {
			if (!strcmp(e->name->str, qr[i].name)) {
				for (int j = 0; j < OCRPT_EXPR_RESULTS; j++)
					if (!e->result[j])
						e->result[j] = &qr[j * cols + i].result;
				found = true;
				break;
			}
		}
	}

	for (ql = q->followers; !found && ql; ql = ql->next) {
		found = ocrpt_resolve_ident(e, (ocrpt_query *)ql->data);
		if (found)
			break;
	}

	for (ql = q->followers_n_to_1; !found && ql; ql = ql->next) {
		found = ocrpt_resolve_ident(e, (ocrpt_query *)ql->data);
		if (found)
			break;
	}

	return found;
}

static void ocrpt_expr_convert_to_string_const(ocrpt_expr *e, char *domain, char *sep, char *name, bool warn) {
	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);
	ocrpt_mem_string_append_printf(EXPR_STRING(e), "%s%s%s", domain, sep, name);
	EXPR_STRING_OWNED(e) = true;
	EXPR_NEXT_RESULT(e) = EXPR_RESULT(e);
	EXPR_PREV_RESULT(e) = EXPR_RESULT(e);

	ocrpt_mem_string_free(e->query, true);
	e->query = NULL;
	ocrpt_mem_string_free(e->name, true);
	e->name = NULL;
	e->dotprefixed = false;

	e->type = OCRPT_EXPR_STRING;

	if (warn)
		ocrpt_err_printf("invalid field reference: '%s', converted to string literal\n", EXPR_STRING_VAL(e));
}

void ocrpt_expr_resolve_worker(ocrpt_expr *e, ocrpt_query *query, ocrpt_expr *orig_e, ocrpt_var *var, int32_t varref_exclude_mask, bool warn, bool *unresolved) {
	int32_t i;

	if (!e)
		return;

	switch (e->type) {
	case OCRPT_EXPR_MVAR:
		if ((varref_exclude_mask & OCRPT_VARREF_MVAR) == 0) {
			if (!e->result[0]) {
				ocrpt_result *result = ocrpt_find_mvariable(e->o, e->name->str);

				if (!result)
					result = ocrpt_env_get(e->o, e->name->str);

				ocrpt_mem_string_free(e->query, true);
				e->query = NULL;
				ocrpt_mem_string_free(e->name, true);
				e->name = NULL;
				e->dotprefixed = false;

				e->type = OCRPT_EXPR_STRING;
				for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
					e->result[i] = result;
					ocrpt_expr_set_result_owned(e, i, !i);
				}
			}
		}
		break;
	case OCRPT_EXPR_RVAR:
		if ((varref_exclude_mask & OCRPT_VARREF_RVAR) == 0) {
			if (strcmp(e->name->str, "self") == 0) {
				/* Don't assert(var) here because unit tests use r.self to test basic behaviour. */
				e->var = var;
				/*
				 * Initialize the self-reference of result pointers crossed
				 * so a call to ocrpt_expr_eval() will use the previous
				 * result.
				 *
				 * ocrpt_expr_init_results() must be called on the
				 * original expression before ocrpt_expr_resolve() because
				 * only the caller may know the intended type.
				 *
				 * The intended use cases are brrownum("breakname")
				 * and iterative variables like sum and others.
				 */
				orig_e->iterative = true;
				for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
					if (!e->result[i] && orig_e->result[ocrpt_expr_prev_residx(i)]) {
						e->result[i] = orig_e->result[ocrpt_expr_prev_residx(i)];
						ocrpt_expr_set_result_owned(e, i, false);
					}
				}
			} else if (strcmp(e->name->str, "baseexpr") == 0) {
				if (var && (orig_e == var->ignoreexpr || orig_e == var->intermedexpr || orig_e == var->intermed2expr || orig_e == var->resultexpr)) {
					e->var = var;
					if (var->baseexpr) {
						ocrpt_expr_resolve_worker(e->var->baseexpr, query, e->var->baseexpr, var, varref_exclude_mask, warn, unresolved);
						for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
							if (!e->result[i]) {
								//assert(var->baseexpr->result[i]);
								e->result[i] = var->baseexpr->result[i];
								ocrpt_expr_set_result_owned(e, i, false);
							}
						}
					}
				} else
					assert(!"illegal reference to r.baseexpr");
			} else if (strcmp(e->name->str, "ignoreexpr") == 0) {
				if (var && (orig_e == var->intermedexpr || orig_e == var->intermed2expr || orig_e == var->resultexpr)) {
					e->var = var;
					if (var->ignoreexpr) {
						ocrpt_expr_resolve_worker(e->var->ignoreexpr, query, e->var->ignoreexpr, var, varref_exclude_mask, warn, unresolved);
						for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
							if (!e->result[i]) {
								//assert(var->ignoreexpr->result[i]);
								e->result[i] = var->ignoreexpr->result[i];
								ocrpt_expr_set_result_owned(e, i, false);
							}
						}
					}
				} else
					assert(!"illegal reference to r.ignoreexpr");
			} else if (strcmp(e->name->str, "intermedexpr") == 0) {
				if (var && (orig_e == var->intermed2expr || orig_e == var->resultexpr)) {
					e->var = var;
					if (var->intermedexpr) {
						ocrpt_expr_resolve_worker(e->var->intermedexpr, query, e->var->intermedexpr, var, varref_exclude_mask, warn, unresolved);
						for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
							if (!e->result[i]) {
								//assert(var->intermedexpr->result[i]);
								e->result[i] = var->intermedexpr->result[i];
								ocrpt_expr_set_result_owned(e, i, false);
							}
						}
					}
				} else
					assert(!"illegal reference to r.intermedexpr");
			} else if (strcmp(e->name->str, "intermed2expr") == 0) {
				if (var && orig_e == var->resultexpr) {
					e->var = var;
					if (var->intermed2expr) {
						ocrpt_expr_resolve_worker(e->var->intermed2expr, query, e->var->intermed2expr, var, varref_exclude_mask, warn, unresolved);
						for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
							if (!e->result[i]) {
								//assert(var->intermed2expr->result[i]);
								e->result[i] = var->intermed2expr->result[i];
								ocrpt_expr_set_result_owned(e, i, false);
							}
						}
					}
				} else
					assert(!"illegal reference to r.intermed2expr");
			} else if (strcmp(e->name->str, "value") == 0) {
				if (orig_e->rvalue) {
					for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
						if (!e->result[i] && orig_e->rvalue->result[i]) {
							e->result[i] = orig_e->rvalue->result[i];
							ocrpt_expr_set_result_owned(e, i, false);
						}
					}
				}
			} else if (strcmp(e->name->str, "totpages") == 0) {
				for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
					if (!e->result[i]) {
						e->result[i] = e->o->totpages;
						ocrpt_expr_set_result_owned(e, i, false);
					}
				}
				if (e->r)
					e->r->have_delayed_expr = true;
			} else if (strcmp(e->name->str, "pageno") == 0) {
				for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
					if (!e->result[i]) {
						e->result[i] = e->o->pageno;
						ocrpt_expr_set_result_owned(e, i, false);
					}
				}
			} else if (strcmp(e->name->str, "lineno") == 0 && e->r) {
				if (e->r->query && e->r->query->rownum) {
					for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
						if (!e->result[i] && e->r->query->rownum->result[i]) {
							e->result[i] = e->r->query->rownum->result[i];
							ocrpt_expr_set_result_owned(e, i, false);
						}
					}
				}
			} else if (strcmp(e->name->str, "detailcnt") == 0) {
				if (e->r) {
					ocrpt_list *l;

					for (l = e->r->detailcnt_dependees; l; l = l->next) {
						if (l->data == orig_e)
							break;
					}
					if (!l)
						e->r->detailcnt_dependees = ocrpt_list_append(e->r->detailcnt_dependees, orig_e);

					if (!EXPR_RESULT(e)) {
						for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
							e->result[i] = e->r->detailcnt->result[i];
							ocrpt_expr_set_result_owned(e, i, false);
						}
					}
				} else
					ocrpt_expr_make_error_result(e, "invalid usage of r.detailcnt");
			} else if (strcmp(e->name->str, "format") == 0) {
				for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
					if (!e->result[i]) {
						e->result[i] = orig_e->o->rptformat;
						ocrpt_expr_set_result_owned(e, i, false);
					}
				}
			} else {
				if (unresolved)
					*unresolved = true;
				/* No such identifier, turn it into a string. */
				ocrpt_expr_convert_to_string_const(e, "r", ".", e->name->str, warn);
			}
		}

		break;
	case OCRPT_EXPR_VVAR:
		if ((varref_exclude_mask & OCRPT_VARREF_VVAR) == 0) {
			if (!e->var) {
				ocrpt_list *ptr;

				for (ptr = e->r ? e->r->variables : NULL; ptr; ptr = ptr->next) {
					ocrpt_var *v = (ocrpt_var *)ptr->data;

					/*
					 * A variable can only reference another variable
					 * if the referenced one is declared earlier.
					 * Therefore jump out of the loop if the referencing
					 * variable is found earlier.
					 */
					if (var && v == var)
						break;

					if (strcasecmp(e->name->str, v->name) == 0) {
						e->var = v;
						break;
					}
				}
				if (!e->var) {
					if (unresolved)
						*unresolved = true;
					/* No such identifier, turn it into a string. */
					ocrpt_expr_convert_to_string_const(e, "v", ".", e->name->str, warn);
				}
			}
		}
		break;
	case OCRPT_EXPR_IDENT:
		if ((varref_exclude_mask & OCRPT_VARREF_IDENT) == 0) {
			bool found = false;

			/* Resolve the identifier ocrpt_query_result from the queries */
			if (EXPR_RESULT(e) && EXPR_TYPE(e) != OCRPT_RESULT_ERROR)
				break;
			if (query)
				found = ocrpt_resolve_ident(e, query);
			else if (e->r && e->r->query)
				found = ocrpt_resolve_ident(e, e->r->query);
			else if (e->o->queries) {
				for (ocrpt_list *ql = e->o->queries; !found && ql; ql = ql->next) {
					found = ocrpt_resolve_ident(e, (ocrpt_query *)ql->data);
					if (found)
						break;
				}
			}

			if (!found) {
				if (unresolved)
					*unresolved = true;
				/* No such identifier, turn it into a string. */
				ocrpt_expr_convert_to_string_const(e, e->query ? e->query->str : "", (e->query || e->dotprefixed) ? "." : "", e->name->str, warn);
			}
		}
		break;
	case OCRPT_EXPR:
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_resolve_worker(e->ops[i], query, orig_e, var, varref_exclude_mask, warn, unresolved);
		break;
	default:
		break;
	}
}

DLL_EXPORT_SYM void ocrpt_expr_resolve(ocrpt_expr *e) {
	if (!e)
		return;

	ocrpt_expr_resolve_worker(e, NULL, e, NULL, 0, true, NULL);
}

DLL_EXPORT_SYM void ocrpt_expr_resolve_exclude(ocrpt_expr *e, int32_t varref_exclude_mask) {
	if (!e)
		return;

	ocrpt_expr_resolve_worker(e, NULL, e, NULL, varref_exclude_mask, true, NULL);
}

bool ocrpt_expr_reference_worker(ocrpt_expr *e, uint32_t varref_include_mask, uint32_t *varref_vartype_mask, ocrpt_list **var_list) {
	if (!e)
		return false;

	bool found = false;

	switch (e->type) {
	case OCRPT_EXPR_MVAR:
		if ((varref_include_mask & OCRPT_VARREF_MVAR))
			found = true;
		break;
	case OCRPT_EXPR_RVAR:
		if ((varref_include_mask & OCRPT_VARREF_RVAR))
			found = true;
		break;
	case OCRPT_EXPR_VVAR:
		if ((varref_include_mask & OCRPT_VARREF_VVAR)) {
			found = true;
			if (e->r && !e->var) {
				ocrpt_list *l;

				for (l = e->r ? e->r->variables : NULL; l; l = l->next) {
					ocrpt_var *v = (ocrpt_var *)l->data;

					if (strcmp(v->name, e->name->str) == 0) {
						e->var = v;
						break;
					}
				}
			}
			if (e->var) {
				if (varref_vartype_mask)
					*varref_vartype_mask |= (1 << e->var->type);
				if (var_list) {
					*var_list = ocrpt_list_append(*var_list, e->var);
				}
			} else {
				if (varref_vartype_mask)
					*varref_vartype_mask |= OCRPT_VARIABLE_UNKNOWN_BIT;
			}
		}
		break;
	case OCRPT_EXPR_IDENT:
		if ((varref_include_mask & OCRPT_VARREF_IDENT)) {
			ocrpt_list *ptr;
			bool domain_found = false;
			bool ident_found = false;

			found = true;

			for (ptr = e->o->queries; ptr; ptr = ptr->next) {
				ocrpt_query *q = (ocrpt_query *)ptr->data;
				ocrpt_query_result *qr = NULL;
				int32_t cols;

				/* Identifier is domain-qualified and it doesn't match the query name */
				if (e->query) {
					if (strcmp(e->query->str, q->name))
						continue;
					domain_found = true;
				}

				/* ocrpt_query_get_result() cannot be used here, we need the whole array */
				if (!q->result)
					q->source->input->describe(q, &q->result, &q->cols);
				qr = q->result;
				cols = (q->result ? q->cols : 0);

				for (int32_t i = 0; i < cols; i++) {
					if (!strcmp(e->name->str, qr[i].name)) {
						for (int j = 0; j < OCRPT_EXPR_RESULTS; j++)
							if (!e->result[j])
								e->result[j] = &qr[j * cols + i].result;
						ident_found = true;
						break;
					}
				}
			}
			if (!ident_found || (e->query && e->query->len > 0 && !domain_found)) {
				if (varref_vartype_mask)
					*varref_vartype_mask |= OCRPT_IDENT_UNKNOWN_BIT;
			}
		}
		break;
	case OCRPT_EXPR:
		for (int32_t i = 0; i < e->n_ops; i++) {
			bool found1 = ocrpt_expr_reference_worker(e->ops[i], varref_include_mask, varref_vartype_mask, var_list);
			found = found || found1;
		}
		break;
	default:
		break;
	}

	return found;
}

bool ocrpt_expr_references(ocrpt_expr *e, int32_t varref_include_mask, uint32_t *varref_vartype_mask) {
	if (varref_vartype_mask)
		*varref_vartype_mask = 0;
	return ocrpt_expr_reference_worker(e, varref_include_mask, varref_vartype_mask, NULL);
}

void ocrpt_expr_eval_worker(ocrpt_expr *e, ocrpt_expr *orig_e, ocrpt_var *var) {
	if (!e || !orig_e)
		return;

	/* If:
	 * - the expression has a self-reference,
	 * - it's the first time it's evaluated, and
	 * - the expression should return the initial value the first time,
	 * then indicate that the next time it should be evaluated.
	 */
	if (e->iterative && e->iterative_init && e->iterative_start_with_init) {
		e->iterative_init = false;
		return;
	}

	int32_t i;

	switch (e->type) {
	case OCRPT_EXPR:
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_eval_worker(e->ops[i], orig_e, var);

		if (e->func && e->func->func)
			e->func->func(e, e->func->user_data);
		else
			ocrpt_err_printf("function is unknown (impossible, it is caught by the parser)\n");
		break;

	case OCRPT_EXPR_VVAR:
		assert(e->var);

		if (e->o->precalculate || !e->var->precalculate) {
			if (!ocrpt_expr_get_result_evaluated(e->var->resultexpr, e->o->residx))
				ocrpt_expr_eval_worker(e->var->resultexpr, e->var->resultexpr, e->var);

			if (!EXPR_RESULT(e)) {
				assert(EXPR_RESULT(e->var->resultexpr));
				EXPR_RESULT(e) = EXPR_RESULT(e->var->resultexpr);
			}
		} else {
			assert(e->var->precalc_rptr);
			EXPR_RESULT(e) = (ocrpt_result *)e->var->precalc_rptr->data;
		}
		break;

	case OCRPT_EXPR_RVAR:
		if (strcmp(e->name->str, "lineno") == 0) {
			if (e->r->query && e->r->query->rownum) {
				for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
					if (!e->result[i] && e->r->query->rownum->result[i]) {
						e->result[i] = e->r->query->rownum->result[i];
						ocrpt_expr_set_result_owned(e, i, false);
					}
				}
			}
		} else if (strcmp(e->name->str, "value") == 0) {
			if (orig_e->rvalue) {
				for (i = 0; i < OCRPT_EXPR_RESULTS; i++) {
					if (!e->result[i] && orig_e->rvalue->result[i]) {
						e->result[i] = orig_e->rvalue->result[i];
						ocrpt_expr_set_result_owned(e, i, false);
					}
				}
			} else {
				ocrpt_expr_make_error_result(e, "invalid usage of r.value");
			}
		} else if (strcmp(e->name->str, "self") == 0) {
			orig_e->iterative = true;
			if (!EXPR_RESULT(e) && EXPR_PREV_RESULT(orig_e)) {
				EXPR_RESULT(e) = EXPR_PREV_RESULT(orig_e);
				ocrpt_expr_set_result_owned(e, e->o->residx, false);
			}
		} else if (strcmp(e->name->str, "baseexpr") == 0) {
			assert(e->var);
			if (e->var->baseexpr) {
				ocrpt_expr_eval_worker(e->var->baseexpr, e->var->baseexpr, e->var);

				if (!EXPR_RESULT(e)) {
					assert(EXPR_RESULT(e->var->baseexpr));
					EXPR_RESULT(e) = EXPR_RESULT(e->var->baseexpr);
				}
			}
		} else if (strcmp(e->name->str, "intermedexpr") == 0) {
			assert(e->var);
			if (e->var->intermedexpr) {
				ocrpt_expr_eval_worker(e->var->intermedexpr, e->var->intermedexpr, e->var);

				if (!EXPR_RESULT(e)) {
					assert(EXPR_RESULT(e->var->intermedexpr));
					EXPR_RESULT(e) = EXPR_RESULT(e->var->intermedexpr);
				}
			}
		} else if (strcmp(e->name->str, "intermed2expr") == 0) {
			assert(e->var);
			if (e->var->intermed2expr) {
				ocrpt_expr_eval_worker(e->var->intermed2expr, e->var->intermed2expr, e->var);

				if (!EXPR_RESULT(e)) {
					assert(EXPR_RESULT(e->var->intermed2expr));
					EXPR_RESULT(e) = EXPR_RESULT(e->var->intermed2expr);
				}
			}
		}
		break;

	default:
		break;
	}

	if (e == orig_e) {
		ocrpt_expr_set_result_evaluated(e, e->o->residx, true);
		ocrpt_expr_set_result_evaluated(e, ocrpt_expr_next_residx(e->o->residx), false);
	}
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_expr_eval(ocrpt_expr *e) {
	if (!e)
		return NULL;

	ocrpt_expr_eval_worker(e, e, NULL);

	if (e->o->precalculate || !e->delayed)
		return EXPR_RESULT(e);
	else
		return e->delayed_result;
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_expr_get_result(ocrpt_expr *e) {
	if (!e)
		return NULL;

	if (e->o->precalculate || !e->delayed) {
		assert(EXPR_RESULT(e));
		return EXPR_RESULT(e);
	} else {
		assert(e->delayed_result);
		return e->delayed_result;
	}
}

DLL_EXPORT_SYM int32_t ocrpt_expr_get_num_operands(ocrpt_expr *e) {
	if (!e)
		return 0;

	return e->n_ops;
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_expr_operand_get_result(ocrpt_expr *e, int32_t opnum) {
	if (!e || opnum < 0 || opnum >= e->n_ops)
		return NULL;

	assert(EXPR_VALID(e->ops[opnum]));
	return EXPR_RESULT(e->ops[opnum]);
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_expr_make_error_result(ocrpt_expr *e, const char *format, ...) {
	if (!e)
		return NULL;

	ocrpt_result *result = EXPR_RESULT(e);
	va_list va;
	size_t len;

	if (!result) {
		result = ocrpt_result_new(e->o);
		if (!result)
			return NULL;

		assert(!EXPR_RESULT(e));
		EXPR_RESULT(e) = result;
		ocrpt_expr_set_result_owned(e, e->o->residx, true);
	}

	result->type = OCRPT_RESULT_ERROR;

	va_start(va, format);
	len = vsnprintf(NULL, 0, format, va);
	va_end(va);

	ocrpt_mem_string_free(result->string, result->string_owned);
	va_start(va, format);
	result->string = ocrpt_mem_string_new_vnprintf(len, format, va);
	va_end(va);
	result->string_owned = true;

	result->isnull = false;

	return result;
}

/*
 * Returns true if the two subsequent row data
 * in the expression are the same
 */
DLL_EXPORT_SYM bool ocrpt_expr_cmp_results(ocrpt_expr *e) {
	if (!e)
		return NULL;

	ocrpt_result *cur = EXPR_RESULT(e);
	ocrpt_result *prev = EXPR_PREV_RESULT(e);

	if (!cur || !prev)
		return false;

	/* (SQL) NULLs compare as non-equal */
	if (cur->isnull || prev->isnull)
		return false;

	/* Different types are obviously non-equal */
	if (cur->type != prev->type)
		return false;

	/* Two subsequent errors compare as non-equal */
	switch (cur->type) {
	case OCRPT_RESULT_ERROR:
		return false;
	case OCRPT_RESULT_STRING:
		return !strcmp(cur->string->str, prev->string->str);
	case OCRPT_RESULT_NUMBER:
		return !mpfr_cmp(cur->number, prev->number);
	case OCRPT_RESULT_DATETIME:
		if (cur->date_valid != prev->date_valid)
			return false;
		if (cur->time_valid != prev->time_valid)
			return false;
		if (cur->interval != prev->interval)
			return false;
		return !memcmp(&cur->datetime, &prev->datetime, sizeof(struct tm));
	default:
		ocrpt_err_printf("unknown expression type %d\n", cur->type);
		return false;
	}
}

DLL_EXPORT_SYM void ocrpt_expr_set_iterative_start_value(ocrpt_expr *e, bool start_with_init) {
	if (!e)
		return;

	e->iterative_init = true;
	e->iterative_start_with_init = start_with_init;
}

DLL_EXPORT_SYM const ocrpt_string *ocrpt_expr_get_string(ocrpt_expr *e) {
	if (!e)
		return NULL;

	ocrpt_result *r = ocrpt_expr_eval(e);

	if (r && r->type == OCRPT_RESULT_STRING)
		return r->string;

	return NULL;
}

DLL_EXPORT_SYM void ocrpt_expr_set_string(ocrpt_expr *e, const char *s) {
	if (!e)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);
	EXPR_STRING_LEN(e) = 0;
	ocrpt_mem_string_append_len(EXPR_STRING(e), s, strlen(s));
}

DLL_EXPORT_SYM long ocrpt_expr_get_long(ocrpt_expr *e) {
	if (!e)
		return 0;

	ocrpt_result *r = ocrpt_expr_eval(e);

	if (r && r->type == OCRPT_RESULT_NUMBER && r->number_initialized)
		return mpfr_get_si(r->number, e->o->rndmode);

	return 0L;
}

DLL_EXPORT_SYM void ocrpt_expr_set_long(ocrpt_expr *e, long l) {
	if (!e)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_set_si(EXPR_NUMERIC(e), l, EXPR_RNDMODE(e));
}

DLL_EXPORT_SYM void ocrpt_expr_set_nth_result_string(ocrpt_expr *e, int which, const char *s) {
	if (!e || which < 0 || which >= OCRPT_EXPR_RESULTS)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);
	e->result[which]->string->len = 0;
	ocrpt_mem_string_append_len(e->result[which]->string, s, strlen(s));
}

DLL_EXPORT_SYM void ocrpt_expr_set_nth_result_long(ocrpt_expr *e, int which, long l) {
	if (!e || which < 0 || which >= OCRPT_EXPR_RESULTS)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_set_si(e->result[which]->number, l, e->o->rndmode);
}

DLL_EXPORT_SYM double ocrpt_expr_get_double(ocrpt_expr *e) {
	if (!e)
		return 0.0;

	ocrpt_result *r = ocrpt_expr_eval(e);

	if (r && r->type == OCRPT_RESULT_NUMBER)
		return mpfr_get_d(r->number, e->o->rndmode);

	return 0.0;
}

DLL_EXPORT_SYM void ocrpt_expr_set_double(ocrpt_expr *e, double d) {
	if (!e)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_set_d(EXPR_NUMERIC(e), d, EXPR_RNDMODE(e));
}

DLL_EXPORT_SYM void ocrpt_expr_set_nth_result_double(ocrpt_expr *e, int which, double d) {
	if (!e || which < 0 || which >= OCRPT_EXPR_RESULTS)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_set_d(e->result[which]->number, d, e->o->rndmode);
}

DLL_EXPORT_SYM mpfr_ptr ocrpt_expr_get_number(ocrpt_expr *e) {
	if (!e)
		return NULL;

	ocrpt_result *r = ocrpt_expr_eval(e);

	if (r && r->type == OCRPT_RESULT_NUMBER)
		return r->number;

	return NULL;
}

DLL_EXPORT_SYM ocrpt_string *ocrpt_expr_get_number_as_string(ocrpt_expr *e, const char *format) {
	if (!e)
		return NULL;

	ocrpt_result *r = ocrpt_expr_eval(e);

	if (r && r->type == OCRPT_RESULT_NUMBER) {
		if (!format)
			format = "%RF";

		size_t len = mpfr_snprintf(NULL, 0, format, r->number);
		ocrpt_string *s = ocrpt_mem_string_new_with_len("", len + 1);
		mpfr_snprintf(s->str, s->len, "%RF", r->number);

		return s;
	}

	return NULL;
}

DLL_EXPORT_SYM void ocrpt_expr_set_number(ocrpt_expr *e, mpfr_ptr m) {
	if (!e || !m)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_set(EXPR_NUMERIC(e), m, EXPR_RNDMODE(e));
}

DLL_EXPORT_SYM void ocrpt_expr_set_number_from_string(ocrpt_expr *e, const char *s) {
	if (!e || !s)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_set_str(EXPR_NUMERIC(e), s, 10, EXPR_RNDMODE(e));
}

DLL_EXPORT_SYM void ocrpt_expr_set_nth_result_number_from_string(ocrpt_expr *e, int which, const char *n) {
	if (!e || which < 0 || which >= OCRPT_EXPR_RESULTS || !n)
		return;

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_set_str(e->result[which]->number, n, 10, e->o->rndmode);
}

static bool ocrpt_expr_get_precalculate_worker(ocrpt_expr *e, int32_t depth) {
	bool precalculate = false;

	if (!e)
		return precalculate;

	switch (e->type) {
	case OCRPT_EXPR_VVAR:
		if (e->var && e->var->precalculate)
			precalculate = true;
		break;
	case OCRPT_EXPR:
		for (int32_t i = 0; i < e->n_ops; i++) {
			precalculate |= ocrpt_expr_get_precalculate_worker(e->ops[i], depth + 1);
		}
		break;
	default:
		break;
	}

	return precalculate;
}

bool ocrpt_expr_get_precalculate(ocrpt_expr *e) {
	return ocrpt_expr_get_precalculate_worker(e, 0);
}

DLL_EXPORT_SYM void ocrpt_expr_set_delayed(ocrpt_expr *e, bool delayed) {
	if (!e)
		return;
	e->delayed = delayed;
}

void ocrpt_report_expressions_add_delayed_results(ocrpt_report *r) {
	ocrpt_list *ptr;

	for (ptr = r->exprs; ptr; ptr = ptr->next) {
		ocrpt_expr *e = (ocrpt_expr *)ptr->data;

		if (e->delayed && !e->delayed_result) {
			ocrpt_result *dst = ocrpt_result_new(e->o);
			ocrpt_result_copy(dst, EXPR_RESULT(e));
			e->delayed_result = dst;
		}
	}
}

DLL_EXPORT_SYM void ocrpt_expr_set_field_expr(ocrpt_expr *e, ocrpt_expr *rvalue) {
	if (!e || (rvalue && (e->o != rvalue->o || e->r != rvalue->r)))
		return;

	e->rvalue = rvalue;
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_result_new(opencreport *o) {
	if (!o)
		return NULL;

	ocrpt_result *result = ocrpt_mem_malloc(sizeof(ocrpt_result));

	memset(result, 0, sizeof(ocrpt_result));
	result->o = o;

	return result;
}

DLL_EXPORT_SYM enum ocrpt_result_type ocrpt_result_get_type(ocrpt_result *result) {
	if (!result)
		return OCRPT_RESULT_ERROR;

	return result->type;
}

DLL_EXPORT_SYM bool ocrpt_result_isnull(ocrpt_result *result) {
	if (!result)
		return true;

	return result->isnull;
}

DLL_EXPORT_SYM void ocrpt_result_set_isnull(ocrpt_result *result, bool isnull) {
	if (!result)
		return;

	result->isnull = isnull;
}

DLL_EXPORT_SYM bool ocrpt_result_isnumber(ocrpt_result *result) {
	if (!result)
		return false;

	return result->type == OCRPT_RESULT_NUMBER && result->number_initialized;
}

DLL_EXPORT_SYM mpfr_ptr ocrpt_result_get_number(ocrpt_result *result) {
	if (!result || result->isnull || result->type != OCRPT_RESULT_NUMBER || !result->number_initialized)
		return NULL;

	return result->number;
}

DLL_EXPORT_SYM ocrpt_string *ocrpt_result_get_number_as_string(ocrpt_result *result, const char *format) {
	if (!result || result->isnull || result->type != OCRPT_RESULT_NUMBER || !result->number_initialized)
		return NULL;

	if (!format)
		format = "%RF";

	size_t len = mpfr_snprintf(NULL, 0, format, result->number);
	ocrpt_string *s = ocrpt_mem_string_new_with_len("", len + 1);
	mpfr_snprintf(s->str, s->len, "%RF", result->number);

	return s;
}

DLL_EXPORT_SYM void ocrpt_result_set_long(ocrpt_result *result, long value) {
	if (!result)
		return;

	result->type = OCRPT_RESULT_NUMBER;
	result->isnull = false;
	if (!result->number_initialized) {
		result->number_initialized = true;
		mpfr_init2(result->number, result->o->prec);
	}

	mpfr_set_si(result->number, value, result->o->rndmode);
}

DLL_EXPORT_SYM void ocrpt_result_set_double(ocrpt_result *result, double value) {
	if (!result)
		return;

	result->type = OCRPT_RESULT_NUMBER;
	result->isnull = false;
	if (!result->number_initialized) {
		result->number_initialized = true;
		mpfr_init2(result->number, result->o->prec);
	}

	mpfr_set_d(result->number, value, result->o->rndmode);
}

DLL_EXPORT_SYM void ocrpt_result_set_number(ocrpt_result *result, mpfr_ptr value) {
	if (!result)
		return;

	result->type = OCRPT_RESULT_NUMBER;
	result->isnull = false;
	if (!result->number_initialized) {
		result->number_initialized = true;
		mpfr_init2(result->number, result->o->prec);
	}

	mpfr_set(result->number, value, result->o->rndmode);
}

DLL_EXPORT_SYM void ocrpt_result_set_number_from_string(ocrpt_result *result, const char *value) {
	if (!result)
		return;

	result->type = OCRPT_RESULT_NUMBER;
	result->isnull = false;
	if (!result->number_initialized) {
		result->number_initialized = true;
		mpfr_init2(result->number, result->o->prec);
	}

	mpfr_set_str(result->number, value, 10, result->o->rndmode);
}

DLL_EXPORT_SYM bool ocrpt_result_isstring(ocrpt_result *result) {
	if (!result)
		return false;

	return result->type == OCRPT_RESULT_STRING && result->string;
}

DLL_EXPORT_SYM ocrpt_string *ocrpt_result_get_string(ocrpt_result *result) {
	if (!result || result->isnull || (result->type != OCRPT_RESULT_STRING && result->type != OCRPT_RESULT_ERROR))
		return NULL;

	return result->string;
}

DLL_EXPORT_SYM void ocrpt_result_set_string(ocrpt_result *result, const char *value) {
	if (!result)
		return;

	result->type = OCRPT_RESULT_STRING;
	if (value) {
		result->isnull = false;

		ocrpt_string *string = ocrpt_mem_string_resize(result->string, 16);
		if (string) {
			if (!result->string) {
				result->string = string;
				result->string_owned = true;
			}
			string->len = 0;
		}

		ocrpt_mem_string_append(string, value);
	} else
		result->isnull = true;
}

DLL_EXPORT_SYM bool ocrpt_result_isdatetime(ocrpt_result *result) {
	if (!result)
		return false;

	return result->type == OCRPT_RESULT_DATETIME;
}

DLL_EXPORT_SYM const struct tm *ocrpt_result_get_datetime(ocrpt_result *result) {
	if (!result || result->isnull || result->type != OCRPT_RESULT_DATETIME)
		return NULL;

	return &result->datetime;
}

DLL_EXPORT_SYM bool ocrpt_result_datetime_is_interval(ocrpt_result *result) {
	if (!result || result->isnull || result->type != OCRPT_RESULT_DATETIME)
		return false;

	return result->interval;
}

DLL_EXPORT_SYM bool ocrpt_result_datetime_is_date_valid(ocrpt_result *result) {
	if (!result || result->isnull || result->type != OCRPT_RESULT_DATETIME)
		return false;

	return result->date_valid;
}

DLL_EXPORT_SYM bool ocrpt_result_datetime_is_time_valid(ocrpt_result *result) {
	if (!result || result->isnull || result->type != OCRPT_RESULT_DATETIME)
		return false;

	return result->time_valid;
}

void ocrpt_expr_init_iterative_results(ocrpt_expr *e, enum ocrpt_result_type type) {
	if (!e)
		return;

	ocrpt_expr_set_iterative_start_value(e, false);
	ocrpt_expr_init_results(e, type);
	for (int i = 0; i < OCRPT_EXPR_RESULTS; i++) {
		if (ocrpt_expr_get_result_owned(e, i))
			switch (type) {
			case OCRPT_RESULT_NUMBER:
				mpfr_set_ui(e->result[i]->number, 0, e->o->rndmode);
				break;
			case OCRPT_RESULT_STRING:
				e->result[i]->string->len = 0;
				break;
			case OCRPT_RESULT_DATETIME:
				memset(&e->result[i]->datetime, 0, sizeof(struct tm));
				e->result[i]->date_valid = false;
				e->result[i]->time_valid = false;
				e->result[i]->interval = false;
				break;
			default:
				break;
			}
	}
}

static bool ocrpt_expr_is_plain_iterative_worker(ocrpt_expr *e, ocrpt_expr *orig_e) {
	if (!e || !orig_e || !orig_e->iterative)
		return false;

	bool ret = false;
	int32_t i;

	switch (e->type) {
	case OCRPT_EXPR:
		for (i = 0; i < e->n_ops; i++) {
			bool subexp_plain_iter = ocrpt_expr_is_plain_iterative_worker(e->ops[i], orig_e);
			ret = ret || subexp_plain_iter;
		}
		break;

	case OCRPT_EXPR_RVAR:
		if (strcmp(e->name->str, "self") == 0) {
			bool self_is_plain = (e->var == NULL);
			ret = ret || self_is_plain;
		}
		break;

	default:
		break;
	}

	return ret;
}

static bool ocrpt_expr_is_plain_iterative(ocrpt_report *r, ocrpt_expr *e) {
	if (!e->iterative)
		return false;

	if (e == r->detailcnt || e == r->query_rownum)
		return false;

	return ocrpt_expr_is_plain_iterative_worker(e, e);
}

void ocrpt_expr_set_plain_iterative_to_null(ocrpt_report *r) {
	for (ocrpt_list *el = r->exprs; el; el = el->next) {
		ocrpt_expr *e = (ocrpt_expr *)el->data;

		if (ocrpt_expr_is_plain_iterative(r, e)) {
			for (int32_t i = 0; i < OCRPT_EXPR_RESULTS; i++)
				if (e->result[i])
					e->result[i]->isnull = true;
		}
	}
}

DLL_EXPORT_SYM const char *ocrpt_expr_get_expr_string(ocrpt_expr *e) {
	return e ? e->expr_string : NULL;
}
