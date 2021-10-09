/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

OCRPT_STATIC_FUNCTION(my_inc) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		mpfr_set_ui(e->result[o->residx]->number, 1, o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(my_dec) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		mpfr_set_ui(e->result[o->residx]->number, 0, o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_expr *e1, *e2;
	char *err;

	/* Override the stock increment and decrement functions with constant 1 and 0 */
	ocrpt_function_add(o, "inc", my_inc, 1, false, false, false, false);
	ocrpt_function_add(o, "dec", my_dec, 1, false, false, false, false);

	err = NULL;
	e1 = ocrpt_expr_parse(o, NULL, "100++", &err);
	if (e1) {
		ocrpt_expr_print(o, e1);
		printf("e1 nodes: %d\n", ocrpt_expr_nodes(e1));
		ocrpt_expr_optimize(o, NULL, e1);
		ocrpt_expr_print(o, e1);
		printf("e1 nodes: %d\n", ocrpt_expr_nodes(e1));
	} else {
		printf("%s\n", err);
		ocrpt_strfree(err);
	}

	err = NULL;
	e2 = ocrpt_expr_parse(o, NULL, "100--", &err);
	if (e2) {
		ocrpt_expr_print(o, e2);
		printf("e2 nodes: %d\n", ocrpt_expr_nodes(e2));
		ocrpt_expr_optimize(o, NULL, e2);
		ocrpt_expr_print(o, e2);
		printf("e2 nodes: %d\n", ocrpt_expr_nodes(e2));
	} else {
		printf("%s\n", err);
		ocrpt_strfree(err);
	}

	ocrpt_expr_free(o, NULL, e1);
	ocrpt_expr_free(o, NULL, e2);
	ocrpt_free(o);

	return 0;
}
