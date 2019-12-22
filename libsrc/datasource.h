/*
 * OpenCReports data source methods
 *
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _DATASOURCE_H_
#define _DATASOURCE_H_

#include <stdbool.h>
#include <stdint.h>
#include <iconv.h>

#include <opencreport.h>
#include "opencreport-private.h"

struct ocrpt_datasource {
	opencreport *o;
	const ocrpt_input *input;
	const char *name;
	void *priv;
	char charset[64];
	iconv_t encoder;
};

struct ocrpt_query {
	char *name;
	const ocrpt_datasource *source;
	ocrpt_query_result *result;
	void *priv;

	int32_t cols;
	int32_t query_index;	/* index in r->queries and r->results */
	int32_t current_row;	/* virtual current row, can be larger than
							 * the actual number of rows in the input source */
	bool n_to_1_empty;		/* shortcut to track 0-row resultsets */
	bool n_to_1_started;	/* track rows in n:1 followers */
	bool n_to_1_matched;
	int32_t fcount;
	int32_t fcount_n1;
	struct ocrpt_query *leader;
	ocrpt_list *followers;		/* list of ocrpt_query structures */
	ocrpt_list *followers_n_to_1;	/* list of ocrpt_query_follower structures */

	bool next_failed;
	bool navigation_failed;
};

typedef struct ocrpt_query ocrpt_query;

struct ocrpt_query_follower {
	ocrpt_query *follower;
	ocrpt_expr *expr;
};

typedef struct ocrpt_query_follower ocrpt_query_follower;

ocrpt_query *ocrpt_alloc_query(opencreport *o, const ocrpt_datasource *source, const char *name);

void ocrpt_free_query0(ocrpt_query *q);

void ocrpt_free_query_result(ocrpt_query *q);

#endif /* _DATASOURCE_H_ */
