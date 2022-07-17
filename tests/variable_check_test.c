/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
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
	ocrpt_var *v;

	r = ocrpt_report_new(o);
	ocrpt_part_append_report(o, NULL, NULL, NULL, r);

	q = ocrpt_query_add_array(o, ds, "a", (const char **)array, 3, 5, coltypes);

	ocrpt_variable_new(o, r, OCRPT_VARIABLE_EXPRESSION, "var1", "id + 1", NULL);

	/* Exercise duplicate variable name */
	ocrpt_variable_new(o, r, OCRPT_VARIABLE_EXPRESSION, "var1", "id + 1", NULL);

	/* Exercise expression variable containing another (known) expression variable */
	v = ocrpt_variable_new(o, r, OCRPT_VARIABLE_EXPRESSION, "var2", "v.var1", NULL);
	printf("adding 'var2' %s\n", v ? "succeeded" : "failed");

	/* Exercise other variable containing expression variable */
	v = ocrpt_variable_new(o, r, OCRPT_VARIABLE_SUM, "var3", "v.var1", NULL);
	printf("adding 'var3' %s\n", v ? "succeeded" : "failed");

	/* Exercise another variable type containing non-expression-type variable */
	ocrpt_variable_new(o, r, OCRPT_VARIABLE_HIGHEST, "var4", "v.var3", NULL);

	/* Exercise another variable type containing unknown variable */
	ocrpt_variable_new(o, r, OCRPT_VARIABLE_HIGHEST, "var5", "v.varX", NULL);

	ocrpt_free(o);

	return 0;
}
