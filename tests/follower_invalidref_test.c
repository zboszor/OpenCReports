/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

static const char *array[4][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes" },
	{ "2", "Wilma Flintstone", "charming", "28", "yes" },
	{ "3", "Pebbles Flintstone", "young", "5e-1", "no" }
};

static const enum ocrpt_result_type coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

static const char *array2[3][5] = {
	{ "id", "name", "property", "age", "adult" },
	{ "2", "Betty Rubble", "beautiful", "27", "yes" },
	{ "1", "Barney Rubble", "small", "29", "yes" },
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *a, *b;
	ocrpt_expr *match;
	bool retval;

	printf("added query a\n\n");
	a = ocrpt_query_add_array(ds, "a", (const char **)array, 3, 5, coltypes);
	printf("added query b\n\n");
	b = ocrpt_query_add_array(ds, "b", (const char **)array2, 2, 5, coltypes);

	printf("adding N:1 follower a -> b with v.var expression, should fail\n");
	match = ocrpt_expr_parse(o, "v.var", NULL);
	retval = ocrpt_query_add_follower_n_to_1(a, b, match);
	printf("%s\n\n", retval ? "succeeded" : "failed");

	printf("adding N:1 follower a -> b with r.var expression, should fail\n");
	match = ocrpt_expr_parse(o, "r.var", NULL);
	retval = ocrpt_query_add_follower_n_to_1(a, b, match);
	printf("%s\n\n", retval ? "succeeded" : "failed");

	printf("adding N:1 follower a -> b with m.var expression, should fail\n");
	match = ocrpt_expr_parse(o, "m.var", NULL);
	retval = ocrpt_query_add_follower_n_to_1(a, b, match);
	printf("%s\n\n", retval ? "succeeded" : "failed");

	printf("adding N:1 follower a -> b with unknown domain reference, should fail\n");
	match = ocrpt_expr_parse(o, "a.id = c.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(a, b, match);
	printf("%s\n\n", retval ? "succeeded" : "failed");

	printf("adding N:1 follower a -> b with unknown ident reference, should fail\n");
	match = ocrpt_expr_parse(o, "a.id = b.xxx", NULL);
	retval = ocrpt_query_add_follower_n_to_1(a, b, match);
	printf("%s\n\n", retval ? "succeeded" : "failed");

	ocrpt_free(o);

	return 0;
}
