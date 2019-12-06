/*
 * OpenCReports array data source
 *
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <opencreport.h>
#include "opencreport-private.h"
#include "datasource.h"

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_add_datasource(opencreport *o, const char *source_name, const ocrpt_input *input) {
	List *l;
	ocrpt_datasource *s = NULL;

	if (!o || !source_name || !*source_name || !input)
		return NULL;

	for (l = o->datasources; l; l = l->next) {
		s = (ocrpt_datasource *)l->data;
		if (strcmp(s->name, source_name) == 0)
			break;
	}

	if (s) {
		if (s->input == input)
			return s;
		return NULL;
	}

	s = ocrpt_mem_malloc(sizeof(ocrpt_datasource));

	if (!s)
		return NULL;

	memset(s, 0, sizeof(ocrpt_datasource));
	s->name = ocrpt_mem_strdup(source_name);
	s->o = o;
	s->input = input;
	s->encoder = iconv_open("UTF-8", "UTF-8");

	o->datasources = list_append(o->datasources, s);

	return s;
}

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_find_datasource(opencreport *o, const char *source_name) {
	List *ptr;

	for (ptr = o->datasources; ptr; ptr = ptr->next) {
		ocrpt_datasource *s = (ocrpt_datasource *)ptr->data;
		if (!strcmp(s->name, source_name))
			return s;
	}

	return NULL;
}

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_validate_datasource(opencreport *o, ocrpt_datasource *source) {
	List *ptr;

	for (ptr = o->datasources; ptr; ptr = ptr->next) {
		if (ptr->data == source)
			return source;
	}

	return NULL;
}

/*
 * Allocate space in the o->queries list
 * Returns:
 * - NULL if any error occurred
 * - a valid pointer if the query space was successfully
 *   allocated and added to o->queries
 */
ocrpt_query *ocrpt_alloc_query(opencreport *o, const ocrpt_datasource *source, const char *name) {
	ocrpt_query *q = ocrpt_mem_malloc(sizeof(ocrpt_query));
	int32_t llen;

	if (!q)
		return NULL;

	memset(q, 0, sizeof(ocrpt_query));

	q->name = ocrpt_mem_strdup(name);
	if (!q->name) {
		ocrpt_mem_free(q);
		return NULL;
	}

	q->source = source;
	llen = list_length(o->queries);
	o->queries = list_append(o->queries, q);

	if (list_length(o->queries) == llen) {
		ocrpt_strfree(q->name);
		ocrpt_mem_free(q);
		return NULL;
	}

	return q;
}

void ocrpt_free_query0(ocrpt_query *q) {
	opencreport *o = q->source->o;
	List *ptr, *ptr1;

	for (ptr = o->queries; ptr; ptr = ptr->next) {
		ocrpt_query *q1 = (ocrpt_query *)ptr->data;
		ocrpt_query_follower *f = NULL;

		if (q1 == q)
			continue;

		q1->followers = list_remove(q1->followers, q);

		for (ptr1 = q1->followers_n_to_1; ptr1; ptr1 = ptr1->next) {
			ocrpt_query_follower *f0 = (ocrpt_query_follower *)ptr1->data;

			if (f0->follower == q) {
				ocrpt_free_expr(f0->expr);
				f = f0;
				break;
			}
		}
		if (f) {
			q1->followers_n_to_1 = list_remove(q1->followers_n_to_1, f);
			ocrpt_mem_free(f);
		}
	}

	if (q->source && q->source->input && q->source->input->free)
		q->source->input->free(q);

	ocrpt_free_query_result(q);
	ocrpt_strfree(q->name);
	q->name = NULL;
	list_free(q->followers);
	for (ptr = q->followers_n_to_1; ptr; ptr = ptr->next) {
		ocrpt_query_follower *f = (ocrpt_query_follower *)ptr->data;
		ocrpt_free_expr(f->expr);
	}
	list_free_deep(q->followers_n_to_1, ocrpt_mem_free);
	ocrpt_mem_free(q);
}

DLL_EXPORT_SYM void ocrpt_free_query(opencreport *o, ocrpt_query *q) {
	ocrpt_free_query0(q);
	o->queries = list_remove(o->queries, q);
}

DLL_EXPORT_SYM ocrpt_query_result *ocrpt_query_get_result(ocrpt_query *q, int32_t *cols) {
	if (!q) {
		if (cols)
			*cols = 0;
		return NULL;
	}

	if (!q->result)
		q->source->input->describe(q, &q->result, &q->cols);
	if (cols)
		*cols = q->cols;
	return (q->source->o->residx ? &q->result[q->cols] : q->result);
}

void ocrpt_free_query_result(ocrpt_query *q) {
	ocrpt_query_result *result = q->result;
	int32_t cols = q->cols, i;

	if (result && cols) {
		for (i = 0; i < 2 * cols; i++) {
			if (result[i].name_allocated) {
				ocrpt_strfree(result[i].name);
				result[i].name = NULL;
			}
			if (result[i].result.string_owned) {
				ocrpt_strfree(result[i].result.string);
				result[i].result.string = NULL;
			}
			if (result[i].result.number_initialized)
				mpfr_clear(result[i].result.number);
		}
	}

	ocrpt_mem_free(result);
	q->result = NULL;
	q->cols = 0;
}

DLL_EXPORT_SYM bool ocrpt_add_query_follower_n_to_1(opencreport *o, ocrpt_query *leader, ocrpt_query *follower, ocrpt_expr *match) {
	ocrpt_query_follower *fo;
	int32_t len;
	bool ret;

	if (!o || !leader || !leader->source || !leader->source->o || o != leader->source->o) {
		fprintf(stderr, "%s:%d: opencreport and ocrpt_query structures do not match\n", __func__, __LINE__);
		return false;
	}

	if (!o || !follower || !follower->source || !follower->source->o || o != follower->source->o) {
		fprintf(stderr, "%s:%d: opencreport and ocrpt_query structures do not match\n", __func__, __LINE__);
		return false;
	}

	ocrpt_expr_optimize(o, match);
	ocrpt_expr_resolve(o, match);

	fo = ocrpt_mem_malloc(sizeof(ocrpt_query_follower));
	if (!fo)
		return false;

	fo->follower = follower;
	fo->expr = match;

	len = list_length(leader->followers_n_to_1);
	leader->followers_n_to_1 = list_append(leader->followers_n_to_1, fo);
	ret = (len < list_length(leader->followers_n_to_1));

	if (!ret)
		ocrpt_mem_free(fo);

	return ret;
}

DLL_EXPORT_SYM bool ocrpt_add_query_follower(opencreport *o, ocrpt_query *leader, ocrpt_query *follower) {
	int32_t len;

	if (!o || !leader || !leader->source || !leader->source->o || o != leader->source->o) {
		fprintf(stderr, "%s:%d: opencreport and ocrpt_query structures do not match\n", __func__, __LINE__);
		return false;
	}

	if (!o || !follower || !follower->source || !follower->source->o || o != follower->source->o) {
		fprintf(stderr, "%s:%d: opencreport and ocrpt_query structures do not match\n", __func__, __LINE__);
		return false;
	}

	len = list_length(leader->followers);
	leader->followers = list_append(leader->followers, follower);
	return (len < list_length(leader->followers));
}
