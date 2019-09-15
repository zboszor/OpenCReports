/*
 * Expression tree utilities
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include "exprutil.h"

void ocrpt_expr_print_worker(ocrpt_expr *e, int depth) {
	int i;

	switch (e->type) {
	case OCRPT_EXPR_ERROR:
		printf("ERROR");
		break;
	case OCRPT_EXPR_STRING:
		printf("(string)%s", e->string);
		break;
	case OCRPT_EXPR_DATETIME:
	case OCRPT_EXPR_NUMBER:
		printf("%s", e->string);
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
		printf("%s(", e->fname);
		for (i = 0; i < e->n_ops; i++) {
			if (i > 0)
				printf(",");
			if (e->uops[i].type == OCRPT_EXPR)
				ocrpt_expr_print_worker(e->uops[i].op, depth + 1);
			else {
				/* TODO: print flattened operand */
			}
		}
		printf(")");
		break;
	case OCRPT_NUMBER_VALUE:
	case OCRPT_FLAT_EXPR:
		/* Ignore these for now. Printing them is TODO */
		break;
	}

	if (depth == 0)
		printf("\n");
}

void ocrpt_expr_print(ocrpt_expr *e) {
	ocrpt_expr_print_worker(e, 0);
}

static void ocrpt_expr_nodes_worker(ocrpt_expr *e, int *nodes) {
	(*nodes)++;

	if (e->type == OCRPT_EXPR) {
		int i;

		for (i = 0; i < e->n_ops; i++)
			ocrpt_expr_nodes_worker(e->uops[i].op, nodes);
	}
}

int ocrpt_expr_nodes(ocrpt_expr *e) {
	int nodes = 0;

	if (e)
		ocrpt_expr_nodes_worker(e, &nodes);
	return nodes;
}
