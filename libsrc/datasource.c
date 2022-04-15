/*
 * OpenCReports array data source
 *
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <iconv.h>

#include "opencreport.h"
#include "datasource.h"

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_add(opencreport *o, const char *source_name, const ocrpt_input *input) {
	ocrpt_datasource *s = NULL;

	if (!o || !source_name || !*source_name || !input)
		return NULL;

	s = ocrpt_datasource_get(o, source_name);
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

	o->datasources = ocrpt_list_append(o->datasources, s);

	return s;
}

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_get(opencreport *o, const char *source_name) {
	ocrpt_list *ptr;

	if (!o || !source_name)
		return NULL;

	for (ptr = o->datasources; ptr; ptr = ptr->next) {
		ocrpt_datasource *s = (ocrpt_datasource *)ptr->data;
		if (!strcmp(s->name, source_name))
			return s;
	}

	return NULL;
}

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_validate(opencreport *o, ocrpt_datasource *source) {
	ocrpt_list *ptr;

	if (!o || !source)
		return NULL;

	for (ptr = o->datasources; ptr; ptr = ptr->next) {
		if (ptr->data == source)
			return source;
	}

	return NULL;
}

DLL_EXPORT_SYM void ocrpt_datasource_set_encoding(opencreport *o, ocrpt_datasource *source, const char *encoding) {
	if (!ocrpt_datasource_validate(o, source))
		return;

	if (source->input->set_encoding)
		source->input->set_encoding(source, encoding);
}

/*
 * Allocate space in the o->queries list
 * Returns:
 * - NULL if any error occurred
 * - a valid pointer if the query space was successfully
 *   allocated and added to o->queries
 */
ocrpt_query *ocrpt_query_alloc(opencreport *o, const ocrpt_datasource *source, const char *name) {
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
	llen = ocrpt_list_length(o->queries);
	o->queries = ocrpt_list_append(o->queries, q);

	if (ocrpt_list_length(o->queries) == llen) {
		ocrpt_strfree(q->name);
		ocrpt_mem_free(q);
		return NULL;
	}

	return q;
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_get(opencreport *o, const char *name) {
	ocrpt_list *ptr;

	if (!o)
		return NULL;

	for (ptr = o->queries; ptr; ptr = ptr->next) {
		ocrpt_query *q = (ocrpt_query *)ptr->data;
		if (!strcmp(q->name, name))
			return q;
	}

	return NULL;
}

void ocrpt_query_free0(ocrpt_query *q) {
	opencreport *o;
	ocrpt_list *ptr, *ptr1;

	if (!q)
		return;

	o = q->source->o;
	for (ptr = o->queries; ptr; ptr = ptr->next) {
		ocrpt_query *q1 = (ocrpt_query *)ptr->data;
		ocrpt_query_follower *f = NULL;

		if (q1 == q)
			continue;

		q1->followers = ocrpt_list_remove(q1->followers, q);

		for (ptr1 = q1->followers_n_to_1; ptr1; ptr1 = ptr1->next) {
			ocrpt_query_follower *f0 = (ocrpt_query_follower *)ptr1->data;

			if (f0->follower == q) {
				ocrpt_expr_free(o, NULL, f0->expr);
				f = f0;
				break;
			}
		}
		if (f) {
			q1->followers_n_to_1 = ocrpt_list_remove(q1->followers_n_to_1, f);
			ocrpt_mem_free(f);
		}
	}

	if (q->source && q->source->input && q->source->input->free)
		q->source->input->free(q);

	ocrpt_query_result_free(q);
	ocrpt_strfree(q->name);
	q->name = NULL;
	ocrpt_list_free(q->followers);
	for (ptr = q->followers_n_to_1; ptr; ptr = ptr->next) {
		ocrpt_query_follower *f = (ocrpt_query_follower *)ptr->data;
		ocrpt_expr_free(o, NULL, f->expr);
	}
	ocrpt_list_free_deep(q->followers_n_to_1, ocrpt_mem_free);
	ocrpt_mem_free(q);
}

DLL_EXPORT_SYM void ocrpt_query_free(opencreport *o, ocrpt_query *q) {
	ocrpt_query_free0(q);
	o->queries = ocrpt_list_remove(o->queries, q);
}

DLL_EXPORT_SYM ocrpt_query_result *ocrpt_query_get_result(ocrpt_query *q, int32_t *cols) {
	if (!q) {
		if (cols)
			*cols = 0;
		return NULL;
	}

	if (!q->result) {
		q->source->input->describe(q, &q->result, &q->cols);
		if (!q->result)
			q->cols = 0;
	}
	if (cols)
		*cols = q->cols;
	return (q->result ? (q->source->o->residx ? &q->result[q->cols] : q->result) : NULL);
}

void ocrpt_query_result_set_values_null(ocrpt_query *q) {
	opencreport *o = q->source->o;
	int32_t base = (o->residx ? q->cols: 0);
	int32_t i;

	for (i = 0; i < q->cols; i++)
		q->result[base + i].result.isnull = true;
}

void ocrpt_query_result_set_value(ocrpt_query *q, int32_t i, bool isnull, iconv_t conv, const char *str, size_t len) {
	opencreport *o = q->source->o;
	int32_t base = (o->residx ? q->cols: 0);
	ocrpt_result *r = &q->result[base + i].result;
	ocrpt_string *rstring;

	r->isnull = isnull;
	if (isnull)
		return;

	if (conv != (iconv_t)-1) {
		int32_t converted_len = (o->converted ? o->converted->allocated_len - 1 : len);
		ocrpt_string *string = NULL;
		bool firstrun = true, restart = false;

		while (firstrun || restart) {
			string = ocrpt_mem_string_resize(o->converted, converted_len);
			char *inbuf;
			char *outbuf;
			size_t inbytesleft, outbytesleft;
			bool exit_loop = false;

			firstrun = false;
			restart = false;

			if (string) {
				if (!o->converted)
					o->converted = string;
			} else
				break;

			inbuf = (char *)str;
			inbytesleft = len;
			outbuf = string->str;
			outbytesleft = string->allocated_len - 1;

			while (!exit_loop && (iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)) {
				switch (errno) {
				case E2BIG:
					converted_len <<= 1;
					restart = true;
					exit_loop = true;
					break;
				case EILSEQ:
				case EINVAL:
					inbuf++;
					inbytesleft--;
					break;
				default:
					exit_loop = true;
					break;
				}
			}
			string->len = string->allocated_len - 1 - outbytesleft;
			string->str[string->len] = 0;
		}

		if (string) {
			str = string->str;
			len = string->len;
		}
	}

	switch (r->type) {
	case OCRPT_RESULT_NUMBER:
		if (!r->number_initialized)
			mpfr_init2(r->number, o->prec);
		if (!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "t"))
			str = "1";
		if (!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "f"))
			str = "0";
		mpfr_set_str(r->number, str, 10, o->rndmode);
		break;
	case OCRPT_RESULT_STRING:
	default:
		if (len < 0)
			len = (str ? strlen(str) : 0);
		rstring = ocrpt_mem_string_resize(r->string, len);
		if (rstring) {
			if (!r->string) {
				r->string = rstring;
				r->string_owned = true;
			}
			rstring->len = 0;
		}
		ocrpt_mem_string_append_len(rstring, str, len);
		break;
	}
}

void ocrpt_query_result_free(ocrpt_query *q) {
	ocrpt_query_result *result = q->result;
	int32_t cols = q->cols, i;

	if (result && cols) {
		for (i = 0; i < 2 * cols; i++) {
			if (result[i].name_allocated) {
				ocrpt_strfree(result[i].name);
				result[i].name = NULL;
			}
			ocrpt_mem_string_free(result[i].result.string, result[i].result.string_owned);
			result[i].result.string = NULL;
			if (result[i].result.number_initialized) {
				mpfr_clear(result[i].result.number);
				result[i].result.number_initialized = false;
			}
		}
	}

	ocrpt_mem_free(result);
	q->result = NULL;
	q->cols = 0;
}

static bool ocrpt_query_follower_circular(opencreport *o, ocrpt_query *leader, ocrpt_query *follower) {
	ocrpt_list *ptr;

	for (ptr = leader->followers; ptr; ptr = ptr->next) {
		ocrpt_query *fptr = (ocrpt_query *)ptr->data;
		if (fptr == follower)
			return false;
		if (!ocrpt_query_follower_circular(o, fptr, follower))
			return false;
	}

	for (ptr = leader->followers_n_to_1; ptr; ptr = ptr->next) {
		ocrpt_query_follower *fptr = (ocrpt_query_follower *)ptr->data;
		if (fptr->follower == follower)
			return false;
		if (!ocrpt_query_follower_circular(o, fptr->follower, follower))
			return false;
	}

	for (ptr = follower->followers; ptr; ptr = ptr->next) {
		ocrpt_query *fptr = (ocrpt_query *)ptr->data;
		if (fptr == leader)
			return false;
		if (!ocrpt_query_follower_circular(o, fptr, leader))
			return false;
	}

	for (ptr = follower->followers_n_to_1; ptr; ptr = ptr->next) {
		ocrpt_query_follower *fptr = (ocrpt_query_follower *)ptr->data;
		if (fptr->follower == leader)
			return false;
		if (!ocrpt_query_follower_circular(o, fptr->follower, leader))
			return false;
	}

	return true;
}

static bool ocrpt_query_follower_validity(opencreport *o, ocrpt_query *leader, ocrpt_query *follower) {
	if (!o) {
		fprintf(stderr, "invalid opencreport pointer\n");
		return false;
	}

	if (!leader) {
		fprintf(stderr, "leader query unset\n");
		return false;
	}

	if (!leader->source) {
		fprintf(stderr, "%s leader query's source unset\n", leader->name);
		return false;
	}

	if (!follower) {
		fprintf(stderr, "follower query unset\n");
		return false;
	}

	if (!follower->source) {
		fprintf(stderr, "%s follower query's source unset\n", follower->name);
		return false;
	}

	if (!leader->source->o || o != leader->source->o) {
		fprintf(stderr, "opencreport and %s:%s leader query structures do not match\n", leader->source->name, leader->name);
		return false;
	}

	if (!follower->source->o || o != follower->source->o) {
		fprintf(stderr, "opencreport and %s:%s follower query structures do not match\n", follower->source->name, follower->name);
		return false;
	}

	if (leader == follower) {
		fprintf(stderr, "leader and follower queries cannot be identical\n");
		return false;
	}

	if (!ocrpt_query_follower_circular(o, leader, follower)) {
		fprintf(stderr, "%s:%s follower would create a circular graph for %s:%s leader\n", follower->source->name, follower->name, leader->source->name, leader->name);
		return false;
	}

	return true;
}

DLL_EXPORT_SYM bool ocrpt_query_add_follower_n_to_1(opencreport *o, ocrpt_query *leader, ocrpt_query *follower, ocrpt_expr *match) {
	ocrpt_query_follower *fo;
	int32_t len;
	uint32_t mask;
	bool ret;

	if (!ocrpt_query_follower_validity(o, leader, follower)) {
		ocrpt_expr_free(o, NULL, match);
		return false;
	}

	if (ocrpt_expr_references(o, NULL, match, OCRPT_VARREF_MVAR | OCRPT_VARREF_RVAR | OCRPT_VARREF_VVAR, NULL)) {
		fprintf(stderr, "invalid expression for follower query\n");
		ocrpt_expr_free(o, NULL, match);
		return false;
	}

	mask = 0;
	if (ocrpt_expr_references(o, NULL, match, OCRPT_VARREF_IDENT, &mask)) {
		if ((mask & OCRPT_IDENT_UNKNOWN_BIT) == OCRPT_IDENT_UNKNOWN_BIT) {
			fprintf(stderr, "invalid expression for follower query\n");
			ocrpt_expr_free(o, NULL, match);
			return false;
		}
	}

	fo = ocrpt_mem_malloc(sizeof(ocrpt_query_follower));
	if (!fo) {
		ocrpt_expr_free(o, NULL, match);
		return false;
	}

	fo->follower = follower;
	fo->expr = match;

	len = ocrpt_list_length(leader->followers_n_to_1);
	leader->followers_n_to_1 = ocrpt_list_append(leader->followers_n_to_1, fo);
	ret = (len < ocrpt_list_length(leader->followers_n_to_1));

	if (!ret) {
		ocrpt_expr_free(o, NULL, match);
		ocrpt_mem_free(fo);
	}

	ocrpt_expr_optimize(o, NULL, match);
	ocrpt_expr_resolve(o, NULL, match);

	return ret;
}

DLL_EXPORT_SYM bool ocrpt_query_add_follower(opencreport *o, ocrpt_query *leader, ocrpt_query *follower) {
	int32_t len;

	if (!ocrpt_query_follower_validity(o, leader, follower))
		return false;

	len = ocrpt_list_length(leader->followers);
	leader->followers = ocrpt_list_append(leader->followers, follower);
	return (len < ocrpt_list_length(leader->followers));
}
