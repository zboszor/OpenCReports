/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include "opencreport.h"

int main(void) {
	const char *exps = "2 ^ 3";
	opencreport *o;
	ocrpt_expr *e;
	ocrpt_result *r;

	o = ocrpt_init();

	e = ocrpt_expr_parse(o, NULL, exps, NULL);

	printf("expr '%s' reprinted (OCRPT mode): ", exps);
	ocrpt_expr_print(o, e);

	r = ocrpt_expr_eval(o, NULL, e);
	ocrpt_result_print(r);

	ocrpt_expr_free(o, NULL, e);

	ocrpt_free(o);

	o = ocrpt_init();
	ocrpt_set_rlib_compat(o);

	e = ocrpt_expr_parse(o, NULL, "2 ^ 3", NULL);

	printf("expr '%s' reprinted (RLIB mode): ", exps);
	ocrpt_expr_print(o, e);

	r = ocrpt_expr_eval(o, NULL, e);
	ocrpt_result_print(r);

	ocrpt_expr_free(o, NULL, e);

	ocrpt_free(o);

	return 0;
}
