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

static void ocrpt_expr_print_worker(ocrpt_expr *e, int depth) {
	int i;

	switch (e->type) {
	case OCRPT_EXPR_ERROR:
		printf("(ERROR)%s", e->result->string);
		break;
	case OCRPT_EXPR_STRING:
		printf("(string)%s", e->result->isnull ? "NULL" : e->result->string);
		break;
	case OCRPT_EXPR_DATETIME:
		printf("(datetime)%s", e->result->isnull ? "NULL" : e->result->string);
		break;
	case OCRPT_EXPR_NUMBER:
		if (e->result->isnull)
			printf("NULL");
		else
			mpfr_printf("%RF", e->result->number);
		break;
	case OCRPT_EXPR_MVAR:
		printf("m.'%s'", e->name);
		break;
	case OCRPT_EXPR_RVAR:
		printf("r.'%s'", e->name);
		break;
	case OCRPT_EXPR_VVAR:
		printf("v.'%s'", e->name);
		break;
	case OCRPT_EXPR_IDENT:
		if (e->query)
			printf("'%s'.'%s'", e->query, e->name);
		else
			printf(".'%s'", e->name);
		break;
	case OCRPT_EXPR:
		printf("%s(", e->func->fname);
		for (i = 0; i < e->n_ops; i++) {
			if (i > 0)
				printf(",");
			ocrpt_expr_print_worker(e->ops[i], depth + 1);
		}
		printf(")");
		break;
	}

	if (depth == 0)
		printf("\n");
}

DLL_EXPORT_SYM void ocrpt_expr_print(ocrpt_expr *e) {
	ocrpt_expr_print_worker(e, 0);
}

DLL_EXPORT_SYM void ocrpt_expr_result_print(ocrpt_result *r) {
	switch (r->type) {
	case OCRPT_RESULT_ERROR:
		printf("(ERROR)%s\n", r->string);
		break;
	case OCRPT_RESULT_STRING:
		printf("(string)%s\n", r->string);
		break;
	case OCRPT_RESULT_DATETIME:
		printf("(datetime)%s\n", r->string);
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
 */
static void ocrpt_expr_optimize_worker(opencreport *o, ocrpt_expr *e) {
	int i, nconst;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	if (!o || !e)
		return;

	//fprintf(stderr, "%s:%d: type is %d\n", __func__, __LINE__, e->type);
	if (e->type != OCRPT_EXPR)
		return;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	if (!e->ops)
		return;

	//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
	for (i = 0, nconst = 0; i < e->n_ops; i++) {
		if (!ocrpt_expr_is_const(e->ops[i])) {
			//fprintf(stderr, "%s:%d\n", __func__, __LINE__);
			ocrpt_expr_optimize_worker(o, e->ops[i]);
		}
		if (ocrpt_expr_is_const(e->ops[i]))
			nconst++;
	}
	if (nconst == e->n_ops) {
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
				e->type = e->result->type;
			} else {
				//fprintf(stderr, "%s: funccall is unset\n", __func__);
			}
		} else {
			//fprintf(stderr, "%s: function is unknown (impossible, it is caught by the parser)\n", __func__);
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
		if (e->result && e->result->type != OCRPT_RESULT_ERROR)
			break;
		for (ptr = o->queries; ptr; ptr = ptr->next) {
			ocrpt_query *q = (ocrpt_query *)ptr->data;
			ocrpt_query_result *qr;
			int32_t cols;

			/* Identifier is domain-qualified and it doesn't match the query name */
			if (e->query && strcmp(e->query, q->name))
				continue;

			qr = ocrpt_query_get_result(q, &cols);
			for (i = 0; i < cols; i++) {
				if (!strcmp(e->name, qr[i].name)) {
					e->result = &qr[i].result;
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

			len = snprintf(NULL, 0, "invalid identifier '%s%s%s'", (e->query ? e->query : ""), (e->dotprefixed ? "." : ""), e->name);
			error = alloca(len + 1);
			snprintf(error, len + 1, "invalid identifier '%s%s%s'", (e->query ? e->query : ""), (e->dotprefixed ? "." : ""), e->name);
			ocrpt_expr_make_error_result(e, error);
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
	return e->result;
}

ocrpt_result *ocrpt_expr_make_error_result(ocrpt_expr *e, const char *error) {
	if (!e->result) {
		e->result = ocrpt_mem_malloc(sizeof(ocrpt_result));
		if (!e->result)
			return NULL;

		memset(e->result, 0, sizeof(ocrpt_result));
		e->result_owned = true;
	}

	e->result->type = OCRPT_RESULT_ERROR;
	if (e->result->string_owned)
		ocrpt_strfree(e->result->string);
	e->result->string = ocrpt_mem_strdup(error);
	e->result->string_owned = true;

	e->result->isnull = false;

	return e->result;
}
