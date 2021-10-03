/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
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
	ocrpt_query *a, *b, *c, *d;
	ocrpt_expr *match_ab, *match_bc, *match_ac, *match_cd;
	char *err;
	bool retval;

	printf("added query a\n\n");
	a = ocrpt_query_add_array(o, ds, "a", (const char **)array, 3, 5, coltypes);
	printf("added query b\n\n");
	b = ocrpt_query_add_array(o, ds, "b", (const char **)array2, 2, 5, coltypes);

	err = NULL;
	match_ab = ocrpt_expr_parse(o, NULL, "a.id = b.id", &err);
	ocrpt_strfree(err);

	printf("adding N:1 follower a -> b, should succeed\n");
	retval = ocrpt_query_add_follower_n_to_1(o, a, b, match_ab);
	printf("added N:1 follower a -> b, retval %d\n\n", retval);

	printf("adding N:1 follower a -> b (duplicate), should fail\n");
	retval = ocrpt_query_add_follower_n_to_1(o, a, b, match_ab);
	printf("added N:1 follower a -> b, retval %d\n\n", retval);

	printf("adding follower a -> b (N:1 exists), should fail\n");
	retval = ocrpt_query_add_follower(o, a, b);
	printf("added follower a -> b, retval %d\n\n", retval);

	printf("adding N:1 follower b -> a (reverse N:1 exists), should fail\n");
	retval = ocrpt_query_add_follower_n_to_1(o, b, a, match_ab);
	printf("added N:1 follower b -> a, retval %d\n\n", retval);

	printf("adding follower b -> a (reverse N:1 exists), should fail\n");
	retval = ocrpt_query_add_follower(o, b, a);
	printf("added follower b -> a, retval %d\n\n", retval);

	c = NULL;

	err = NULL;
	match_bc = ocrpt_expr_parse(o, NULL, "b.id = c.id", &err);
	ocrpt_strfree(err);

	printf("adding N:1 follower b -> c (query c does not exist), should fail\n");
	retval = ocrpt_query_add_follower_n_to_1(o, b, c, match_bc);
	printf("added N:1 follower b -> c, retval %d\n\n", retval);

	printf("adding follower b -> c (query c does not exist), should fail\n");
	retval = ocrpt_query_add_follower(o, b, c);
	printf("added follower b -> c, retval %d\n\n", retval);

	c = ocrpt_query_add_array(o, ds, "c", (const char **)array2, 2, 5, coltypes);
	printf("added query c\n\n");

	printf("adding N:1 follower b -> c, should succeed\n");
	retval = ocrpt_query_add_follower_n_to_1(o, b, c, match_bc);
	printf("added N:1 follower b -> c, retval %d\n\n", retval);

	printf("adding follower b -> c (N:1 follower exists), should fail\n");
	retval = ocrpt_query_add_follower(o, b, c);
	printf("added follower b -> c, retval %d\n\n", retval);

	err = NULL;
	match_ac = ocrpt_expr_parse(o, NULL, "a.id = c.id", &err);
	ocrpt_strfree(err);

	printf("adding N:1 follower c -> a (circular followers), should fail\n");
	retval = ocrpt_query_add_follower_n_to_1(o, c, a, match_ac);
	printf("added N:1 follower c -> a, retval %d\n\n", retval);

	printf("adding follower c -> a (a->b->c exists), should fail\n");
	retval = ocrpt_query_add_follower(o, c, a);
	printf("added N:1 follower c -> a, retval %d\n\n", retval);

	printf("adding N:1 follower a -> c (c would be a follower on two paths), should fail\n");
	retval = ocrpt_query_add_follower_n_to_1(o, a, c, match_ac);
	printf("added N:1 follower a -> c, retval %d\n\n", retval);

	printf("adding follower a -> c (c would be a follower on two paths), should fail\n");
	retval = ocrpt_query_add_follower(o, a, c);
	printf("added follower a -> c, retval %d\n\n", retval);

	/*
	 * The above ocrpt_query_add_follower_n_to_1() calls
	 * using this expression failed so ocrpt_free() would
	 * not free it automatically.
	 */
	ocrpt_expr_free(match_ac);

	d = ocrpt_query_add_array(o, ds, "d", (const char **)array2, 2, 5, coltypes);
	printf("added query d\n\n");

	err = NULL;
	match_cd = ocrpt_expr_parse(o, NULL, "c.id = d.id", &err);
	ocrpt_strfree(err);

	printf("adding follower c -> d, should succeed\n");
	retval = ocrpt_query_add_follower_n_to_1(o, c, d, match_cd);
	printf("added follower c -> d, retval %d\n\n", retval);

	printf("adding follower d -> a, should fail\n");
	retval = ocrpt_query_add_follower_n_to_1(o, d, a, match_cd);
	printf("added follower d -> a, retval %d\n\n", retval);

	ocrpt_free(o);

	return 0;
}
