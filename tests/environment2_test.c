/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include "opencreport.h"

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_expr *e;
	char *err;

	setenv("OCRPTENV", "This is a test string", 1);
	printf("OCRPTENV is set.\n");

	err = NULL;
	e = ocrpt_expr_parse(o, "m.OCRPTENV", &err);
	if (e) {
		printf("Before resolving: ");
		ocrpt_expr_print(e);
		ocrpt_expr_resolve(e);
		ocrpt_expr_optimize(e);
		printf("After resolving: ");
		ocrpt_expr_print(e);
		ocrpt_expr_free(e);
	} else {
		printf("expr failed to parse: %s\n", err);
		ocrpt_strfree(err);
	}

	unsetenv("OCRPTENV");
	printf("OCRPTENV is unset.\n");

	err = NULL;
	e = ocrpt_expr_parse(o, "m.OCRPTENV", &err);
	if (e) {
		printf("Before resolving: ");
		ocrpt_expr_print(e);
		ocrpt_expr_resolve(e);
		ocrpt_expr_optimize(e);
		printf("After resolving: ");
		ocrpt_expr_print(e);
		ocrpt_expr_free(e);
	} else {
		printf("expr failed to parse: %s\n", err);
		ocrpt_strfree(err);
	}

	ocrpt_set_mvariable(o, "OCRPTENV", "extravar");
	printf("OCRPTENV is added as mvariable.\n");

	err = NULL;
	e = ocrpt_expr_parse(o, "m.OCRPTENV", &err);
	if (e) {
		printf("Before resolving: ");
		ocrpt_expr_print(e);
		ocrpt_expr_resolve(e);
		ocrpt_expr_optimize(e);
		printf("After resolving: ");
		ocrpt_expr_print(e);
		ocrpt_expr_free(e);
	} else {
		printf("expr failed to parse: %s\n", err);
		ocrpt_strfree(err);
	}

	ocrpt_set_mvariable(o, "OCRPTENV", NULL);
	printf("OCRPTENV is removed as mvariable.\n");

	err = NULL;
	e = ocrpt_expr_parse(o, "m.OCRPTENV", &err);
	if (e) {
		printf("Before resolving: ");
		ocrpt_expr_print(e);
		ocrpt_expr_resolve(e);
		ocrpt_expr_optimize(e);
		printf("After resolving: ");
		ocrpt_expr_print(e);
		ocrpt_expr_free(e);
	} else {
		printf("expr failed to parse: %s\n", err);
		ocrpt_strfree(err);
	}

	ocrpt_free(o);

	return 0;
}
