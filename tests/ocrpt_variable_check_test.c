/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "ocrpt_test_common.h"

static const char *array[4][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

static const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *q UNUSED;
	ocrpt_report *r;
	ocrpt_expr *e;
	ocrpt_var *v;
	char *err;

	r = ocrpt_report_new(o);
	ocrpt_part_append_report(o, NULL, r);

	q = ocrpt_query_add_array(o, ds, "a", (const char **)array, 3, 5, coltypes);

	err = NULL;
	e = ocrpt_expr_parse(o, r, "id + 1", &err);
	ocrpt_strfree(err);
	ocrpt_variable_new(o, r, OCRPT_VARIABLE_EXPRESSION, "var1", e, NULL);

	/* Exercise duplicate variable name */
	err = NULL;
	e = ocrpt_expr_parse(o, r, "id + 1", &err);
	ocrpt_strfree(err);
	ocrpt_variable_new(o, r, OCRPT_VARIABLE_EXPRESSION, "var1", e, NULL);

	/* Exercise expression variable containing another (known) expression variable */
	err = NULL;
	e = ocrpt_expr_parse(o, r, "v.var1", &err);
	ocrpt_strfree(err);
	v = ocrpt_variable_new(o, r, OCRPT_VARIABLE_EXPRESSION, "var2", e, NULL);
	printf("adding 'var2' %s\n", v ? "succeeded" : "failed");

	/* Exercise other variable containing expression variable */
	err = NULL;
	e = ocrpt_expr_parse(o, r, "v.var1", &err);
	ocrpt_strfree(err);
	v = ocrpt_variable_new(o, r, OCRPT_VARIABLE_SUM, "var3", e, NULL);
	printf("adding 'var3' %s\n", v ? "succeeded" : "failed");

	/* Exercise another variable type containing non-expression-type variable */
	err = NULL;
	e = ocrpt_expr_parse(o, r, "v.var3", &err);
	ocrpt_strfree(err);
	ocrpt_variable_new(o, r, OCRPT_VARIABLE_HIGHEST, "var4", e, NULL);

	/* Exercise another variable type containing unknown variable */
	err = NULL;
	e = ocrpt_expr_parse(o, r, "v.varX", &err);
	ocrpt_strfree(err);
	ocrpt_variable_new(o, r, OCRPT_VARIABLE_HIGHEST, "var5", e, NULL);

	ocrpt_free(o);

	return 0;
}
