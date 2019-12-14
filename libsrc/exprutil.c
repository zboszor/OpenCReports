/*
 * Expression tree utilities
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport-private.h"
#include "exprutil.h"
#include "datasource.h"
#include "listutil.h"
#include "functions.h"

ocrpt_expr *newblankexpr(enum ocrpt_expr_type type, uint32_t n_ops) {
	ocrpt_expr *e;

	e = ocrpt_mem_malloc(sizeof(ocrpt_expr));
	if (!e)
		return NULL;

	memset(e, 0, sizeof(ocrpt_expr));

	e->ops = (n_ops > 0 ? ocrpt_mem_malloc(n_ops * sizeof(ocrpt_expr *)) : NULL);

	if ((n_ops > 0) && !e->ops) {
		ocrpt_free_expr(e);
		return NULL;
	}

	e->type = type;
	e->n_ops = n_ops;

	return e;
}

static void ocrpt_expr_print_worker(opencreport *o, ocrpt_expr *e, int depth) {
	ocrpt_result *result = e->result[o->residx];
	int i;

	switch (e->type) {
	case OCRPT_EXPR_ERROR:
		printf("(ERROR)%s", result->string->str);
		break;
	case OCRPT_EXPR_STRING:
		printf("(string)%s", result->isnull ? "NULL" : result->string->str);
		break;
	case OCRPT_EXPR_DATETIME:
		printf("(datetime)%s", result->isnull ? "NULL" : result->string->str);
		break;
	case OCRPT_EXPR_NUMBER:
		if (result->isnull)
			printf("NULL");
		else
			mpfr_printf("%RF", result->number);
		break;
	case OCRPT_EXPR_MVAR:
		printf("m.'%s'", e->name->str);
		break;
	case OCRPT_EXPR_RVAR:
		printf("r.'%s'", e->name->str);
		break;
	case OCRPT_EXPR_VVAR:
		printf("v.'%s'", e->name->str);
		break;
	case OCRPT_EXPR_IDENT:
		if (e->query)
			printf("'%s'.'%s'", e->query->str, e->name->str);
		else
			printf(".'%s'", e->name->str);
		break;
	case OCRPT_EXPR:
		printf("%s(", e->func->fname);
		for (i = 0; i < e->n_ops; i++) {
			if (i > 0)
				printf(",");
			ocrpt_expr_print_worker(o, e->ops[i], depth + 1);
		}
		printf(")");
		break;
	}

	if (depth == 0)
		printf("\n");
}

DLL_EXPORT_SYM void ocrpt_expr_print(opencreport *o, ocrpt_expr *e) {
	ocrpt_expr_print_worker(o, e, 0);
}

DLL_EXPORT_SYM void ocrpt_expr_result_print(ocrpt_result *r) {
	switch (r->type) {
	case OCRPT_RESULT_ERROR:
		printf("(ERROR)%s\n", r->string->str);
		break;
	case OCRPT_RESULT_STRING:
		printf("(string)%s\n", r->string->str);
		break;
	case OCRPT_RESULT_DATETIME:
		printf("(datetime)%s\n", r->string->str);
		break;
	case OCRPT_RESULT_NUMBER:
		mpfr_printf("%RF\n", r->number);
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
static void ocrpt_expr_optimize_worker(opencreport *o, ocrpt_expr *e) {
	int32_t i, nconst, dconst, sconst, dtconst;
	bool try_pullup = true;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	if (!o || !e)
		return;

	//fprintf(stderr, "%s:%d: type is %d\n", __func__, __LINE__, e->type);
	if (e->type != OCRPT_EXPR)
		return;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	if (!e->ops)
		return;

	again:

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	for (i = 0, nconst = 0, dconst = 0, sconst = 0, dtconst = 0; i < e->n_ops; i++) {
		if (!ocrpt_expr_is_const(e->ops[i])) {
			//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
			ocrpt_expr_optimize_worker(o, e->ops[i]);
		}
		if (ocrpt_expr_is_dconst(e->ops[i]))
			dconst++, nconst++;
		if (ocrpt_expr_is_sconst(e->ops[i]))
			sconst++, nconst++;
		if (ocrpt_expr_is_dtconst(e->ops[i]))
			dtconst++, nconst++;
	}
	if (nconst > 1 && nconst == e->n_ops) {
		/*
		 * Fully constant expression, precompute it.
		 */
		if (e->func) {
			if (e->func->func) {
				//fprintf(stderr, "%s:%d: calling func() ptr\n", __func__, __LINE__);
				e->func->func(o, e);
				//fprintf(stderr, "%s:%d: returned from func()\n", __func__, __LINE__);
				//fprintf(stderr, "subexpr: \n");
				//ocrpt_expr_print(e);
				for (i = 0; i < e->n_ops; i++)
					ocrpt_free_expr(e->ops[i]);
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
			ocrpt_free_expr(split1);
			ocrpt_free_expr(split2);
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
						ocrpt_free_expr(x);

						expr_pulled_up = true;
					}
				}

				try_pullup = expr_pulled_up;

				goto again;
			}
		}
	}
}

DLL_EXPORT_SYM void ocrpt_expr_optimize(opencreport *o, ocrpt_expr *e) {
	if (!o || !e)
		return;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	ocrpt_expr_optimize_worker(o, e);
}

static void ocrpt_expr_resolve_worker(opencreport *o, ocrpt_expr *e) {
	List *ptr;
	int32_t i;
	bool found = false;

	switch (e->type) {
	case OCRPT_EXPR_MVAR: /* TODO */
	case OCRPT_EXPR_RVAR: /* TODO */
	case OCRPT_EXPR_VVAR: /* TODO */
		break;
	case OCRPT_EXPR_IDENT:
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
			if (q->result) {
				qr = q->result;
				cols = q->cols;
			} else
				q->source->input->describe(q, &qr, &cols);

			for (i = 0; i < cols; i++) {
				if (!strcmp(e->name->str, qr[i].name)) {
					e->result[0] = &qr[i].result;
					e->result[1] = &qr[cols + i].result;
					found = true;
					break;
				}
			}
			if (found)
				break;
		}
		if (!found) {
			char *error;
			int len;

			len = snprintf(NULL, 0, "invalid identifier '%s%s%s'", (e->query ? e->query->str : ""), (e->dotprefixed ? "." : ""), e->name->str);
			error = alloca(len + 1);
			snprintf(error, len + 1, "invalid identifier '%s%s%s'", (e->query ? e->query->str : ""), (e->dotprefixed ? "." : ""), e->name->str);
			ocrpt_expr_make_error_result(o, e, error);
		}
		break;
	case OCRPT_EXPR:
		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_resolve_worker(o, e->ops[i]);
		break;
	default:
		break;
	}
}

DLL_EXPORT_SYM void ocrpt_expr_resolve(opencreport *o, ocrpt_expr *e) {
	ocrpt_expr_resolve_worker(o, e);
}

static void ocrpt_expr_eval_worker(opencreport *o, ocrpt_expr *e) {
	int i;

	if (!o || !e)
		return;

	if (e->type != OCRPT_EXPR)
		return;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	if (!e->ops)
		return;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	for (i = 0; i < e->n_ops; i++)
		ocrpt_expr_eval_worker(o, e->ops[i]);

	if (e->func && e->func->func) {
		if (e->func->func) {
			//fprintf(stderr, "%s:%d: calling func() ptr\n", __func__, __LINE__);
			e->func->func(o, e);
		} else {
			fprintf(stderr, "%s: funccall is unset\n", __func__);
		}
	} else {
		fprintf(stderr, "%s: function is unknown (impossible, it is caught by the parser)\n", __func__);
	}
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_expr_eval(opencreport *o, ocrpt_expr *e) {
	ocrpt_expr_eval_worker(o, e);
	return e->result[o->residx];
}

ocrpt_result *ocrpt_expr_make_error_result(opencreport *o, ocrpt_expr *e, const char *error) {
	ocrpt_result *result = e->result[o->residx];

	if (!result) {
		result = ocrpt_mem_malloc(sizeof(ocrpt_result));
		if (!result)
			return NULL;

		memset(result, 0, sizeof(ocrpt_result));
		e->result[o->residx] = result;
		ocrpt_expr_set_result_owned(o, e, true);
	}

	result->type = OCRPT_RESULT_ERROR;
	ocrpt_mem_string_free(result->string, result->string_owned);
	result->string = ocrpt_mem_string_new(error, true);
	result->string_owned = true;

	result->isnull = false;

	return result;
}
