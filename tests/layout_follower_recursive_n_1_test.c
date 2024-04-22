/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
#include "test_common.h"

#define ROWS 6
#define COLS 2

const char *array[ROWS + 1][COLS] = {
	{ "id", "name" },
	{ "1", "Snow White" },
	{ "2", "Batman" },
	{ "3", "Cinderella" },
	{ "4", "Hansel" },
	{ "5", "Little Red Riding Hood" },
	{ "6", "Robin Hood" }
};

#define ROWS1 13
#define COLS1 3
const char *sidekicks[ROWS1 + 1][COLS1] = {
	{ "id", "boss_id", "name" },
	{ "1", "1", "Doc" },
	{ "2", "1", "Dopey" },
	{ "3", "1", "Sneezy" },
	{ "4", "1", "Happy" },
	{ "5", "1", "Bashful" },
	{ "6", "1", "Sleepy" },
	{ "7", "1", "Grumpy" },
	{ "8", "2", "Robin" },
	{ "9", "3", "Fairy Godmother" },
	{ "10", "3", "Mice" },
	{ "11", "3", "Pidgeons" },
	{ "12", "4", "Gretel" },
	/* Little Red Riding Hood does not have a sidekick */
	{ "13", "6", "Little John" }
};

#define ROWS2 6
#define COLS2 2
const char *sidekicks2[ROWS2 + 1][COLS2] = {
	{ "sk_id", "name" },
	{ "3", "Coughy" },
	{ "3", "Crippley" },
	{ "9", "Prince Charming" },
	{ "9", "Shrek" },
	{ "13", "Will Scarlet" },
	{ "13", "Brother Tuck" }
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);
	ocrpt_query *q = ocrpt_query_add_data(ds, "data", (const char **)array, ROWS, COLS, NULL, 0);
	ocrpt_query *q2 = ocrpt_query_add_data(ds, "more_data", (const char **)sidekicks, ROWS1, COLS1, NULL, 0);
	ocrpt_query *q3 = ocrpt_query_add_data(ds, "moar_data", (const char **)sidekicks2, ROWS2, COLS2, NULL, 0);
	ocrpt_expr *match, *match2;

	match = ocrpt_expr_parse(o, "data.id = more_data.boss_id", NULL);
	if (!ocrpt_query_add_follower_n_to_1(q, q2, match)) {
		fprintf(stderr, "Failed to add follower q <- q2\n");
		return 0;
	}

	match2 = ocrpt_expr_parse(o, "more_data.id = moar_data.sk_id", NULL);
	if (!ocrpt_query_add_follower_n_to_1(q2, q3, match2)) {
		fprintf(stderr, "Failed to add follower q2 <- q3\n");
		return 0;
	}

	if (!ocrpt_parse_xml(o, "follower_recursive_n_1_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
