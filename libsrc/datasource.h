/*
 * OpenCReports data source methods
 *
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _DATASOURCE_H_
#define _DATASOURCE_H_

#include <stdbool.h>
#include <stdint.h>
#include <iconv.h>

#include "opencreport.h"

extern const ocrpt_input ocrpt_postgresql_input;
extern const ocrpt_input ocrpt_mariadb_input;
extern const ocrpt_input ocrpt_odbc_input;
extern const ocrpt_input ocrpt_array_input;
extern const ocrpt_input ocrpt_csv_input;
extern const ocrpt_input ocrpt_json_input;
extern const ocrpt_input ocrpt_xml_input;

struct ocrpt_datasource {
	opencreport *o;
	const ocrpt_input *input;
	const char *name;
	void *priv;
};

struct ocrpt_query {
	char *name;
	const ocrpt_datasource *source;
	ocrpt_query_result *result;
	void *priv;

	/*
	 * These are lists of ocrpt_query structures.
	 * - regular followers
	 * - N:1 followers from the user
	 * - flattened list of N:1 followers
	 */
	ocrpt_list *followers;
	ocrpt_list *followers_n_to_1;
	ocrpt_list *global_followers_n_to_1;
	ocrpt_list *global_followers_n_to_1_next;

	/* pointer to the topmost parent for regular or N:1 followers */
	struct ocrpt_query *leader;
	ocrpt_expr *match;
	/* same as current_row+1, used by rownum() function */
	ocrpt_expr *rownum;
	/*
	 * virtual current row, can be larger than
	 * the actual number of rows in the input source
	 */
	int32_t current_row;
	int32_t cols;
	int32_t query_index;	/* index in r->queries and r->results */
	int32_t fcount;
	int32_t fcount_n1;
	bool leader_is_n_1:1;	/* whether the leader points to a chain of regular or N:1 followers  */
	bool n_to_1_empty:1;	/* shortcut to track 0-row resultsets */
	bool n_to_1_started:1;	/* track rows in n:1 followers */
	bool n_to_1_matched:1;
};

void ocrpt_query_result_set_values_null(ocrpt_query *q);
void ocrpt_query_result_set_value(ocrpt_query *q, int32_t i, bool isnull, iconv_t conv, const char *str, size_t len);

void ocrpt_query_free0(ocrpt_query *q);

void ocrpt_query_result_free(ocrpt_query *q);

void ocrpt_query_finalize_followers(ocrpt_query *q);

#endif /* _DATASOURCE_H_ */
