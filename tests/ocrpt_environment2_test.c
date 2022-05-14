/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
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
	e = ocrpt_expr_parse(o, NULL, "m.OCRPTENV", &err);
	if (e) {
		printf("Before resolving: ");
		ocrpt_expr_print(o, e);
		ocrpt_expr_resolve(o, NULL, e, NULL);
		ocrpt_expr_optimize(o, NULL, e);
		printf("After resolving: ");
		ocrpt_expr_print(o, e);
		ocrpt_expr_free(o, NULL, e);
	} else {
		printf("expr failed to parse: %s\n", err);
		ocrpt_strfree(err);
	}

	unsetenv("OCRPTENV");
	printf("OCRPTENV is unset.\n");

	err = NULL;
	e = ocrpt_expr_parse(o, NULL, "m.OCRPTENV", &err);
	if (e) {
		printf("Before resolving: ");
		ocrpt_expr_print(o, e);
		ocrpt_expr_resolve(o, NULL, e, NULL);
		ocrpt_expr_optimize(o, NULL, e);
		printf("After resolving: ");
		ocrpt_expr_print(o, e);
		ocrpt_expr_free(o, NULL, e);
	} else {
		printf("expr failed to parse: %s\n", err);
		ocrpt_strfree(err);
	}

	ocrpt_free(o);

	return 0;
}
