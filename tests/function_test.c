/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

OCRPT_STATIC_FUNCTION(my_inc) {
	if (ocrpt_expr_get_num_operands(e) != 1 || !ocrpt_expr_operand_get_result(e, 0)) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	switch (ocrpt_result_get_type(ocrpt_expr_operand_get_result(e, 0))) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		ocrpt_expr_set_long_value(e, 1L);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_result *rs = ocrpt_expr_operand_get_result(e, 0);
		ocrpt_expr_make_error_result(e, rs ? ocrpt_result_get_string(rs)->str : "invalid operand(s)");
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(my_dec) {
	if (ocrpt_expr_get_num_operands(e) != 1 || !ocrpt_expr_operand_get_result(e, 0)) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	switch (ocrpt_result_get_type(ocrpt_expr_operand_get_result(e, 0))) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		ocrpt_expr_set_long_value(e, 0L);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_result *rs = ocrpt_expr_operand_get_result(e, 0);
		ocrpt_expr_make_error_result(e, rs ? ocrpt_result_get_string(rs)->str : "invalid operand(s)");
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		break;
	}
}

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_expr *e1, *e2;
	char *err;

	/* Override the stock increment and decrement functions with constant 1 and 0 */
	ocrpt_function_add(o, "inc", my_inc, NULL, 1, false, false, false, false);
	ocrpt_function_add(o, "dec", my_dec, NULL, 1, false, false, false, false);

	err = NULL;
	e1 = ocrpt_expr_parse(o, "100++", &err);
	if (e1) {
		ocrpt_expr_print(e1);
		printf("e1 nodes: %d\n", ocrpt_expr_nodes(e1));
		ocrpt_expr_optimize(e1);
		ocrpt_expr_print(e1);
		printf("e1 nodes: %d\n", ocrpt_expr_nodes(e1));
	} else {
		printf("%s\n", err);
		ocrpt_strfree(err);
	}

	err = NULL;
	e2 = ocrpt_expr_parse(o, "100--", &err);
	if (e2) {
		ocrpt_expr_print(e2);
		printf("e2 nodes: %d\n", ocrpt_expr_nodes(e2));
		ocrpt_expr_optimize(e2);
		ocrpt_expr_print(e2);
		printf("e2 nodes: %d\n", ocrpt_expr_nodes(e2));
	} else {
		printf("%s\n", err);
		ocrpt_strfree(err);
	}

	ocrpt_expr_free(e1);
	ocrpt_expr_free(e2);
	ocrpt_free(o);

	return 0;
}
