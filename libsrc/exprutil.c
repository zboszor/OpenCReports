/*
 * Expression tree utilities
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <alloca.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport.h"
#include "exprutil.h"
#include "datasource.h"

ocrpt_expr *newblankexpr(enum ocrpt_expr_type type, uint32_t n_ops) {
	ocrpt_expr *e;

	e = ocrpt_mem_malloc(sizeof(ocrpt_expr));
	if (!e)
		return NULL;

	memset(e, 0, sizeof(ocrpt_expr));

	e->ops = (n_ops > 0 ? ocrpt_mem_malloc(n_ops * sizeof(ocrpt_expr *)) : NULL);

	if ((n_ops > 0) && !e->ops) {
		ocrpt_expr_free(e);
		return NULL;
	}

	e->type = type;
	e->n_ops = n_ops;

	return e;
}

static void ocrpt_expr_print_worker(opencreport *o, ocrpt_expr *e, int depth, const char *delimiter, ocrpt_string *str) {
	ocrpt_result *result = e->result[o->residx];
	int i;

	switch (e->type) {
	case OCRPT_EXPR_ERROR:
		ocrpt_mem_string_append_printf(str, "(ERROR)%s", result->string->str);
		break;
	case OCRPT_EXPR_STRING:
		ocrpt_mem_string_append_printf(str, "(string)%s", result->isnull ? "NULL" : result->string->str);
		break;
	case OCRPT_EXPR_DATETIME:
		ocrpt_mem_string_append_printf(str, "(datetime)%s", result->isnull ? "NULL" : result->string->str);
		break;
	case OCRPT_EXPR_NUMBER:
		if (result->isnull)
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
			ocrpt_expr_print_worker(o, e->ops[i], depth + 1, delimiter, str);
		}
		ocrpt_mem_string_append_printf(str, ")");
		break;
	}

	if (depth == 0)
		ocrpt_mem_string_append_printf(str, "%s", delimiter);
}

DLL_EXPORT_SYM void ocrpt_expr_print(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *str = ocrpt_mem_string_new_with_len(NULL, 256);
	ocrpt_expr_print_worker(o, e, 0, "\n", str);
	printf("%s", str->str);
	ocrpt_mem_string_free(str, true);
}

static void ocrpt_expr_result_deep_print_worker(opencreport *o, ocrpt_expr *e) {
	ocrpt_string *str = ocrpt_mem_string_new_with_len(NULL, 256);
	int i;

	switch (e->type) {
	case OCRPT_EXPR:
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_result_deep_print(o, e->ops[i]);
		break;
	case OCRPT_EXPR_VVAR:
		assert(e->var);
		if (e->var->baseexpr)
			ocrpt_expr_result_deep_print(o, e->var->baseexpr);
		if (e->var->intermedexpr)
			ocrpt_expr_result_deep_print(o, e->var->intermedexpr);
		if (e->var->intermed2expr)
			ocrpt_expr_result_deep_print(o, e->var->intermed2expr);
		ocrpt_expr_result_deep_print(o, e->var->resultexpr);
		break;
	default:
		break;
	}

	ocrpt_expr_print_worker(o, e, 0, " - ", str);
	printf("%s", str->str);
	ocrpt_mem_string_free(str, true);
	ocrpt_result_print(e->result[o->residx]);
}

DLL_EXPORT_SYM void ocrpt_expr_result_deep_print(opencreport *o, ocrpt_expr *e) {
	ocrpt_expr_result_deep_print_worker(o, e);
}

DLL_EXPORT_SYM void ocrpt_result_print(ocrpt_result *r) {
	if (!r) {
		printf("ERROR: ocrpt_result is NULL\n");
		return;
	}

	switch (r->type) {
	case OCRPT_RESULT_ERROR:
		printf("(ERROR)%s\n", r->isnull ? "NULL" : r->string->str);
		break;
	case OCRPT_RESULT_STRING:
		printf("(string)%s\n", (r->isnull || !r->string) ? "NULL" : r->string->str);
		break;
	case OCRPT_RESULT_DATETIME:
		printf("(datetime)%s\n", r->isnull ? "NULL" : r->string->str);
		break;
	case OCRPT_RESULT_NUMBER:
		if (r->isnull)
			printf("(number)NULL\n");
		else
			mpfr_printf("(number)%RF\n", r->number);
		break;
	}
}

static void ocrpt_expr_nodes_worker(ocrpt_expr *e, int *nodes) {
	(*nodes)++;

	if (e->type == OCRPT_EXPR) {
		int i;

		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_nodes_worker(e->ops[i], nodes);
	}
}

DLL_EXPORT_SYM int ocrpt_expr_nodes(ocrpt_expr *e) {
	int nodes = 0;

	if (e)
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
static void ocrpt_expr_optimize_worker(opencreport *o, ocrpt_report *r, ocrpt_expr *e) {
	int32_t i, nconst, dconst, sconst, dtconst;
	bool try_pullup = true;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	if (!o || !e)
		return;

	//fprintf(stderr, "%s:%d: type is %d\n", __func__, __LINE__, e->type);
	if (e->type != OCRPT_EXPR)
		return;

	again:

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	for (i = 0, nconst = 0, dconst = 0, sconst = 0, dtconst = 0; i < e->n_ops; i++) {
		if (!ocrpt_expr_is_const(e->ops[i])) {
			//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
			ocrpt_expr_optimize_worker(o, r, e->ops[i]);
		}
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
				//fprintf(stderr, "%s:%d: calling func() ptr\n", __func__, __LINE__);
				e->func->func(o, r, e);
				//fprintf(stderr, "%s:%d: returned from func()\n", __func__, __LINE__);
				//fprintf(stderr, "subexpr: \n");
				//ocrpt_expr_print(e);
				for (i = 0; i < e->n_ops; i++)
					ocrpt_expr_free(e->ops[i]);
				ocrpt_mem_free(e->ops);
				e->n_ops = 0;
				e->ops = NULL;
				e->type = e->result[o->residx]->type;
			} else {
				//fprintf(stderr, "%s: funccall is unset\n", __func__);
			}
		} else {
			//fprintf(stderr, "%s: function is unknown (impossible, it is caught by the parser)\n", __func__);
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
		ocrpt_expr *split1 = newblankexpr(OCRPT_EXPR, dconst);
		ocrpt_expr *split2 = newblankexpr(OCRPT_EXPR, e->n_ops - dconst);
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

DLL_EXPORT_SYM void ocrpt_expr_optimize(opencreport *o, ocrpt_report *r, ocrpt_expr *e) {
	if (!o || !e)
		return;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	ocrpt_expr_optimize_worker(o, r, e);
}

void ocrpt_expr_resolve_worker(opencreport *o, ocrpt_report *r, ocrpt_expr *e, ocrpt_expr *orig_e, ocrpt_var *var, int32_t varref_exclude_mask) {
	int32_t i;

	switch (e->type) {
	case OCRPT_EXPR_MVAR:
		if ((varref_exclude_mask & OCRPT_VARREF_MVAR) == 0) {
			if (!e->result[0] && !e->result[1]) {
				ocrpt_result *result = ocrpt_environment_get_c(e->name->str);

				ocrpt_mem_string_free(e->query, true);
				e->query = NULL;
				ocrpt_mem_string_free(e->name, true);
				e->name = NULL;
				e->dotprefixed = false;

				e->type = OCRPT_EXPR_STRING;
				e->result[0] = result;
				e->result_owned0 = true;
				e->result[1] = result;
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
				 * ocrpt_expr_init_both_results() must be called on the
				 * original expression before ocrpt_expr_resolve() because
				 * only the caller may know the intended type.
				 *
				 * The intended use cases are breakrownumber("breakname")
				 * and iterative variables like sum and others.
				 */
				orig_e->iterative = true;
				if (!e->result[0]) {
					assert(orig_e->result[1]);
					e->result[0] = orig_e->result[1];
					e->result_owned0 = false;
				}
				if (!e->result[1]) {
					assert(orig_e->result[0]);
					e->result[1] = orig_e->result[0];
					e->result_owned1 = false;
				}
			} else if (strcmp(e->name->str, "baseexpr") == 0) {
				assert(var);
				e->var = var;
				if (var->baseexpr) {
					ocrpt_expr_resolve_worker(o, r, e->var->baseexpr, e->var->baseexpr, var, varref_exclude_mask);
					if (!e->result[0]) {
						assert(var->baseexpr->result[0]);
						e->result[0] = var->baseexpr->result[0];
						e->result_owned0 = false;
					}
					if (!e->result[1]) {
						assert(var->baseexpr->result[1]);
						e->result[1] = var->baseexpr->result[1];
						e->result_owned1 = false;
					}
				}
			} else if (strcmp(e->name->str, "intermedexpr") == 0) {
				assert(var);
				e->var = var;
				if (var->intermedexpr) {
					ocrpt_expr_resolve_worker(o, r, e->var->intermedexpr, e->var->intermedexpr, var, varref_exclude_mask);
					if (!e->result[0]) {
						assert(var->intermedexpr->result[0]);
						e->result[0] = var->intermedexpr->result[0];
						e->result_owned0 = false;
					}
					if (!e->result[1]) {
						assert(var->intermedexpr->result[1]);
						e->result[1] = var->intermedexpr->result[1];
						e->result_owned1 = false;
					}
				}
			} else if (strcmp(e->name->str, "intermed2expr") == 0) {
				assert(var);
				e->var = var;
				if (var->intermed2expr) {
					ocrpt_expr_resolve_worker(o, r, e->var->intermed2expr, e->var->intermed2expr, var, varref_exclude_mask);
					if (!e->result[0]) {
						assert(var->intermed2expr->result[0]);
						e->result[0] = var->intermed2expr->result[0];
						e->result_owned0 = false;
					}
					if (!e->result[1]) {
						assert(var->intermed2expr->result[1]);
						e->result[1] = var->intermed2expr->result[1];
						e->result_owned1 = false;
					}
				}
			}
			/* TODO: implement generally usable global report variables */
		}

		break;
	case OCRPT_EXPR_VVAR:
		if ((varref_exclude_mask & OCRPT_VARREF_VVAR) == 0) {
			if (!e->var) {
				ocrpt_list *ptr;

				for (ptr = r ? r->variables : NULL; ptr; ptr = ptr->next) {
					ocrpt_var *v = (ocrpt_var *)ptr->data;
					if (strcasecmp(e->name->str, v->name) == 0) {
						e->var = v;
						if (v->precalculate)
							orig_e->delayed = true;
						break;
					}
				}
				if (!ptr)
					ocrpt_expr_make_error_result(o, e, "unknown variable v.'%s'\n", e->name->str);
			}
		}
		break;
	case OCRPT_EXPR_IDENT:
		if ((varref_exclude_mask & OCRPT_VARREF_IDENT) == 0) {
			ocrpt_list *ptr;
			bool found = false;

			/* Resolve the identifier ocrpt_query_result from the queries */
			if (e->result[o->residx] && e->result[o->residx]->type != OCRPT_RESULT_ERROR)
				break;
			for (ptr = o->queries; ptr; ptr = ptr->next) {
				ocrpt_query *q = (ocrpt_query *)ptr->data;
				ocrpt_query_result *qr = NULL;
				int32_t cols;

				/* Identifier is domain-qualified and it doesn't match the query name */
				if (e->query && strcmp(e->query->str, q->name))
					continue;

				/* ocrpt_query_get_result() cannot be used here, we need the whole array */
				if (!q->result)
					q->source->input->describe(q, &q->result, &q->cols);
				qr = q->result;
				cols = (q->result ? q->cols : 0);

				for (i = 0; i < cols; i++) {
					if (!strcmp(e->name->str, qr[i].name)) {
						if (!e->result[0])
							e->result[0] = &qr[i].result;
						if (!e->result[1])
							e->result[1] = &qr[cols + i].result;
						found = true;
						break;
					}
				}

				if (!found)
					ocrpt_expr_make_error_result(o, e, "invalid identifier '%s%s%s'", (e->query ? e->query->str : ""), ((e->query || e->dotprefixed) ? "." : ""), e->name->str);
			}
		}
		break;
	case OCRPT_EXPR:
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_resolve_worker(o, r, e->ops[i], orig_e, var, varref_exclude_mask);
		break;
	default:
		break;
	}
}

DLL_EXPORT_SYM void ocrpt_expr_resolve(opencreport *o, ocrpt_report *r, ocrpt_expr *e) {
	ocrpt_expr_resolve_worker(o, r, e, e, NULL, 0);
}

DLL_EXPORT_SYM void ocrpt_expr_resolve_exclude(opencreport *o, ocrpt_report *r, ocrpt_expr *e, int32_t varref_exclude_mask) {
	ocrpt_expr_resolve_worker(o, r, e, e, NULL, varref_exclude_mask);
}

static bool ocrpt_expr_reference_worker(opencreport *o, ocrpt_report *r, ocrpt_expr *e, uint32_t varref_include_mask, uint32_t *varref_vartype_mask) {
	int32_t i;
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
			if (r && !e->var) {
				ocrpt_list *l;

				for (l = r->variables; l; l = l->next) {
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

			for (ptr = o->queries; ptr; ptr = ptr->next) {
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

				for (i = 0; i < cols; i++) {
					if (!strcmp(e->name->str, qr[i].name)) {
						if (!e->result[0])
							e->result[0] = &qr[i].result;
						if (!e->result[1])
							e->result[1] = &qr[cols + i].result;
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
		for (i = 0; i < e->n_ops; i++) {
			bool found1 = ocrpt_expr_reference_worker(o, r, e->ops[i], varref_include_mask, varref_vartype_mask);
			found = found || found1;
		}
		break;
	default:
		break;
	}

	return found;
}

DLL_EXPORT_SYM bool ocrpt_expr_references(opencreport *o, ocrpt_report *r, ocrpt_expr *e, int32_t varref_include_mask, uint32_t *varref_vartype_mask) {
	if (varref_vartype_mask)
		*varref_vartype_mask = 0;
	return ocrpt_expr_reference_worker(o, r, e, varref_include_mask, varref_vartype_mask);
}

void ocrpt_expr_eval_worker(opencreport *o, ocrpt_report *r, ocrpt_expr *e, ocrpt_expr *orig_e, ocrpt_var *var) {
	int i;

	if (!o || !e)
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

	switch (e->type) {
	case OCRPT_EXPR:
		//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_eval_worker(o, r, e->ops[i], orig_e, var);

		if (e->func && e->func->func) {
			if (e->func->func) {
				//fprintf(stderr, "%s:%d: calling func() ptr\n", __func__, __LINE__);
				e->func->func(o, r, e);
			} else {
				fprintf(stderr, "%s: funccall is unset\n", __func__);
			}
		} else {
			fprintf(stderr, "%s: function is unknown (impossible, it is caught by the parser)\n", __func__);
		}
		break;

	case OCRPT_EXPR_VVAR:
		assert(e->var);

		if (!ocrpt_expr_get_result_evaluated(o, e->var->resultexpr))
			ocrpt_expr_eval_worker(o, r, e->var->resultexpr, e->var->resultexpr, e->var);

		if (!e->result[o->residx]) {
			assert(e->var->resultexpr->result[o->residx]);
			e->result[o->residx] = e->var->resultexpr->result[o->residx];
		}
		break;

	case OCRPT_EXPR_RVAR:
		if (strcmp(e->name->str, "self") == 0) {
			/* Do nothing - r.self references the VVAR's previous value */
		} else if (strcmp(e->name->str, "baseexpr") == 0) {
			assert(e->var);
			if (e->var->baseexpr) {
				ocrpt_expr_eval_worker(o, r, e->var->baseexpr, e->var->baseexpr, e->var);

				if (!e->result[o->residx]) {
					assert(e->var->baseexpr->result[o->residx]);
					e->result[o->residx] = e->var->baseexpr->result[o->residx];
				}
			}
		} else if (strcmp(e->name->str, "intermedexpr") == 0) {
			assert(e->var);
			if (e->var->intermedexpr) {
				ocrpt_expr_eval_worker(o, r, e->var->intermedexpr, e->var->intermedexpr, e->var);

				if (!e->result[o->residx]) {
					assert(e->var->intermedexpr->result[o->residx]);
					e->result[o->residx] = e->var->intermedexpr->result[o->residx];
				}
			}
		} else if (strcmp(e->name->str, "intermed2expr") == 0) {
			assert(e->var);
			if (e->var->intermed2expr) {
				ocrpt_expr_eval_worker(o, r, e->var->intermed2expr, e->var->intermed2expr, e->var);

				if (!e->result[o->residx]) {
					assert(e->var->intermed2expr->result[o->residx]);
					e->result[o->residx] = e->var->intermed2expr->result[o->residx];
				}
			}
		}
		/* TODO: implement generally usable global report variables */
		break;

	default:
		break;
	}

	if (e == orig_e) {
		ocrpt_expr_set_result_evaluated(o, e, o->residx, true);
		ocrpt_expr_set_result_evaluated(o, e, !o->residx, false);
	}
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_expr_eval(opencreport *o, ocrpt_report *r, ocrpt_expr *e) {
	ocrpt_expr_eval_worker(o, r, e, e, NULL);
	return e->result[o->residx];
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_expr_make_error_result(opencreport *o, ocrpt_expr *e, const char *format, ...) {
	ocrpt_result *result = e->result[o->residx];
	va_list va;
	size_t len;

	if (!result) {
		result = ocrpt_mem_malloc(sizeof(ocrpt_result));
		if (!result)
			return NULL;

		memset(result, 0, sizeof(ocrpt_result));
		assert(!e->result[o->residx]);
		e->result[o->residx] = result;
		ocrpt_expr_set_result_owned(o, e, o->residx, true);
	}

	result->type = OCRPT_RESULT_ERROR;

	va_start(va, format);
	len = ocrpt_mem_vnprintf_size_from_string(format, va);
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
DLL_EXPORT_SYM bool ocrpt_expr_cmp_results(opencreport *o, ocrpt_expr *e) {
	if (!e->result[o->residx] || !e->result[!o->residx])
		return false;

	/* (SQL) NULLs compare as non-equal */
	if (e->result[o->residx]->isnull || e->result[!o->residx]->isnull)
		return false;

	/* Different types are obviously non-equal */
	if (e->result[o->residx]->type != e->result[!o->residx]->type)
		return false;

	/* Two subsequent errors compare as non-equal */
	switch (e->result[o->residx]->type) {
	case OCRPT_RESULT_ERROR:
		return false;
	case OCRPT_RESULT_STRING:
		return !strcmp(e->result[o->residx]->string->str, e->result[!o->residx]->string->str);
	case OCRPT_RESULT_NUMBER:
		return !mpfr_cmp(e->result[o->residx]->number, e->result[!o->residx]->number);
	case OCRPT_RESULT_DATETIME:
		return false; // TODO: implement
	default:
		fprintf(stderr, "%s:%d: unknown expression type %d\n", __func__, __LINE__, e->result[o->residx]->type);
		return false;
	}
}

DLL_EXPORT_SYM void ocrpt_expr_set_iterative_start_value(ocrpt_expr *e, bool start_with_init) {
	e->iterative_init = true;
	e->iterative_start_with_init = start_with_init;
}

DLL_EXPORT_SYM void ocrpt_expr_get_value(opencreport *o, ocrpt_expr *e, char **s, int32_t *i) {
	ocrpt_result *r;

	if (s)
		*s = NULL;
	if (i)
		*i = 0;
	if (!e)
		return;

	r = ocrpt_expr_eval(o, NULL, e);
	if (r) {
		if (s && r->type == OCRPT_RESULT_STRING)
			*s = r->string->str;
		if (i && OCRPT_RESULT_NUMBER)
			*i = mpfr_get_si(r->number, o->rndmode);
	}
}
