/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>

#include <opencreport.h>

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

#define ROWS1 2
static const char *array2[ROWS1 + 1][COLS] = {
	{ "id", "name", "property", "age", "adult" },
	{ "2", "Betty Rubble", "beautiful", "27", "yes" },
	{ "1", "Barney Rubble", "small", "29", "yes" },
};

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "array");
	ocrpt_query *a, *b, *c, *d;
	ocrpt_expr *match_ab, *match_bc, *match_ac, *match_cd;
	bool retval;

	printf("added query a\n\n");
	a = ocrpt_query_add_array(ds, "a", (const char **)array, ROWS, COLS, coltypes, COLS);
	printf("added query b\n\n");
	b = ocrpt_query_add_array(ds, "b", (const char **)array2, ROWS1, COLS, coltypes, COLS);

	printf("adding N:1 follower a -> b, should succeed\n");
	match_ab = ocrpt_expr_parse(o, "a.id = b.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(a, b, match_ab);
	printf("added N:1 follower a -> b, retval %d\n\n", retval);

	printf("adding N:1 follower a -> b (duplicate), should fail\n");
	match_ab = ocrpt_expr_parse(o, "a.id = b.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(a, b, match_ab);
	printf("added N:1 follower a -> b, retval %d\n\n", retval);

	printf("adding follower a -> b (N:1 exists), should fail\n");
	retval = ocrpt_query_add_follower(a, b);
	printf("added follower a -> b, retval %d\n\n", retval);

	printf("adding N:1 follower b -> a (reverse N:1 exists), should fail\n");
	match_ab = ocrpt_expr_parse(o, "a.id = b.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(b, a, match_ab);
	printf("added N:1 follower b -> a, retval %d\n\n", retval);

	printf("adding follower b -> a (reverse N:1 exists), should fail\n");
	retval = ocrpt_query_add_follower(b, a);
	printf("added follower b -> a, retval %d\n\n", retval);

	c = NULL;

	printf("adding N:1 follower b -> c (query c does not exist), should fail\n");
	match_bc = ocrpt_expr_parse(o, "b.id = c.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(b, c, match_bc);
	printf("added N:1 follower b -> c, retval %d\n\n", retval);

	printf("adding follower b -> c (query c does not exist), should fail\n");
	retval = ocrpt_query_add_follower(b, c);
	printf("added follower b -> c, retval %d\n\n", retval);

	c = ocrpt_query_add_array(ds, "c", (const char **)array2, ROWS1, COLS, coltypes, COLS);
	printf("added query c\n\n");

	printf("adding N:1 follower b -> c, should succeed\n");
	match_bc = ocrpt_expr_parse(o, "b.id = c.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(b, c, match_bc);
	printf("added N:1 follower b -> c, retval %d\n\n", retval);

	printf("adding follower b -> c (N:1 follower exists), should fail\n");
	retval = ocrpt_query_add_follower(b, c);
	printf("added follower b -> c, retval %d\n\n", retval);

	printf("adding N:1 follower c -> a (circular followers), should fail\n");
	match_ac = ocrpt_expr_parse(o, "a.id = c.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(c, a, match_ac);
	printf("added N:1 follower c -> a, retval %d\n\n", retval);

	printf("adding follower c -> a (a->b->c exists), should fail\n");
	retval = ocrpt_query_add_follower(c, a);
	printf("added N:1 follower c -> a, retval %d\n\n", retval);

	printf("adding N:1 follower a -> c (c would be a follower on two paths), should fail\n");
	match_ac = ocrpt_expr_parse(o, "a.id = c.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(a, c, match_ac);
	printf("added N:1 follower a -> c, retval %d\n\n", retval);

	printf("adding follower a -> c (c would be a follower on two paths), should fail\n");
	retval = ocrpt_query_add_follower(a, c);
	printf("added follower a -> c, retval %d\n\n", retval);

	d = ocrpt_query_add_array(ds, "d", (const char **)array2, ROWS1, COLS, coltypes, COLS);
	printf("added query d\n\n");

	printf("adding follower c -> d, should succeed\n");
	match_cd = ocrpt_expr_parse(o, "c.id = d.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(c, d, match_cd);
	printf("added follower c -> d, retval %d\n\n", retval);

	printf("adding follower d -> a, should fail\n");
	match_cd = ocrpt_expr_parse(o, "c.id = d.id", NULL);
	retval = ocrpt_query_add_follower_n_to_1(d, a, match_cd);
	printf("added follower d -> a, retval %d\n\n", retval);

	ocrpt_free(o);

	return 0;
}
