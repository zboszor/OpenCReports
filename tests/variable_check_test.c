/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 3
#define COLS 5
static const char *array[ROWS + 1][COLS] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

static const int32_t coltypes[COLS] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q UNUSED;
	ocrpt_report *r;
	ocrpt_var *v;

	r = ocrpt_part_column_new_report(ocrpt_part_row_new_column(ocrpt_part_new_row(ocrpt_part_new(o))));

	q = ocrpt_query_add_array(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);

	ocrpt_variable_new(r, OCRPT_VARIABLE_EXPRESSION, "var1", "id + 1", NULL);

	/* Exercise duplicate variable name */
	ocrpt_variable_new(r, OCRPT_VARIABLE_EXPRESSION, "var1", "id + 1", NULL);

	/* Exercise expression variable containing another (known) expression variable */
	v = ocrpt_variable_new(r, OCRPT_VARIABLE_EXPRESSION, "var2", "v.var1", NULL);
	printf("adding 'var2' %s\n", v ? "succeeded" : "failed");

	/* Exercise other variable containing expression variable */
	v = ocrpt_variable_new(r, OCRPT_VARIABLE_SUM, "var3", "v.var1", NULL);
	printf("adding 'var3' %s\n", v ? "succeeded" : "failed");

	/* Exercise another variable type containing non-expression-type variable */
	ocrpt_variable_new(r, OCRPT_VARIABLE_HIGHEST, "var4", "v.var3", NULL);

	/* Exercise another variable type containing unknown variable */
	ocrpt_variable_new(r, OCRPT_VARIABLE_HIGHEST, "var5", "v.varX", NULL);

	ocrpt_free(o);

	return 0;
}
