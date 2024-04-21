/*
 * OpenCReports array data source
 *
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "exprutil.h"
#include "datasource.h"
#include "datetime.h"
#include "fallthrough.h"

static pthread_mutex_t input_register_mutex = PTHREAD_MUTEX_INITIALIZER;
ocrpt_input **ocrpt_inputs = NULL;
int32_t n_ocrpt_inputs = 0;

static int inputsortname(const void *key1, const void *key2) {
	return strcasecmp((*(const ocrpt_input **)key1)->names[0], (*(const ocrpt_input **)key2)->names[0]);
}

DLL_EXPORT_SYM bool ocrpt_input_register(const ocrpt_input * const input) {
	if (!input || !input->names || !input->names[0])
		return false;

	/*
	 * Check presence of crucial functions in the input.
	 * Optional ones are ->connect, ->clone, ->set_encoding and ->free
	 */
	if (!input->describe || !input->rewind || !input->next || !input->populate_result || !input->isdone)
		return false;

	pthread_mutex_lock(&input_register_mutex);

	for (int32_t i = 0; i < n_ocrpt_inputs; i++) {
		/* A previously registered datasource input type can be replaced */
		for (int32_t j = 0; ocrpt_inputs[i]->names[j]; j++) {
			for (int32_t k = 0; input->names[k]; k++) {
				if (strcasecmp(ocrpt_inputs[i]->names[j], input->names[k]) == 0) {
					ocrpt_inputs[i] = (ocrpt_input *)input;
					pthread_mutex_unlock(&input_register_mutex);
					return true;
				}
			}
		}
	}

	ocrpt_input **new_inputs = ocrpt_mem_realloc(ocrpt_inputs, (n_ocrpt_inputs + 1) * sizeof(ocrpt_input *));

	if (!new_inputs) {
		pthread_mutex_unlock(&input_register_mutex);
		return false;
	}

	ocrpt_inputs = new_inputs;
	ocrpt_inputs[n_ocrpt_inputs] = (ocrpt_input *)input;
	n_ocrpt_inputs++;

	qsort(ocrpt_inputs, n_ocrpt_inputs, sizeof(ocrpt_input *), inputsortname);

	pthread_mutex_unlock(&input_register_mutex);

	return true;
}

DLL_EXPORT_SYM const ocrpt_input * const ocrpt_input_get(const char *name) {
	const ocrpt_input *input = NULL;

	pthread_mutex_lock(&input_register_mutex);

	for (int32_t i = 0; i < n_ocrpt_inputs; i++) {
		for (int32_t j = 0; ocrpt_inputs[i]->names[j]; j++) {
			if (strcasecmp(ocrpt_inputs[i]->names[j], name) == 0) {
				input = ocrpt_inputs[i];
				break;
			}
		}
	}

	pthread_mutex_unlock(&input_register_mutex);

	return input;
}

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_add(opencreport *o, const char *source_name, const char *type, const ocrpt_input_connect_parameter *conn_params) {
	if (!o || !source_name || !*source_name || !type)
		return NULL;

	const ocrpt_input *input = ocrpt_input_get(type);
	if (!input)
		return NULL;

	ocrpt_datasource *s = ocrpt_datasource_get(o, source_name);
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

	bool connected = input->connect ? input->connect(s, conn_params) : true;

	if (!connected) {
		ocrpt_strfree(s->name);
		ocrpt_mem_free(s);;
		return NULL;
	}

	o->datasources = ocrpt_list_append(o->datasources, s);

	return s;
}

DLL_EXPORT_SYM opencreport *ocrpt_datasource_get_opencreport(const ocrpt_datasource *ds) {
	return ds ? ds->o : NULL;
}

DLL_EXPORT_SYM const char *ocrpt_datasource_get_name(const ocrpt_datasource *ds) {
	return ds ? ds->name : NULL;
}

DLL_EXPORT_SYM const ocrpt_input *ocrpt_datasource_get_input(const ocrpt_datasource *ds) {
	return ds ? ds->input : NULL;
}

DLL_EXPORT_SYM void ocrpt_datasource_set_private(ocrpt_datasource *ds, void *priv) {
	if (!ds)
		return;

	/* Be careful overwriting it. */
	ds->priv = priv;
}

DLL_EXPORT_SYM void *ocrpt_datasource_get_private(const ocrpt_datasource *ds) {
	return ds ? ds->priv : NULL;
}

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_get(opencreport *o, const char *source_name) {
	if (!o || !source_name)
		return NULL;

	for (ocrpt_list *ptr = o->datasources; ptr; ptr = ptr->next) {
		ocrpt_datasource *s = (ocrpt_datasource *)ptr->data;
		if (!strcmp(s->name, source_name))
			return s;
	}

	return NULL;
}

DLL_EXPORT_SYM void ocrpt_datasource_set_encoding(ocrpt_datasource *source, const char *encoding) {
	if (!source || !source->input)
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
DLL_EXPORT_SYM ocrpt_query *ocrpt_query_alloc(const ocrpt_datasource *source, const char *name) {
	int32_t llen;

	if (!source)
		return NULL;

	ocrpt_query *q = ocrpt_mem_malloc(sizeof(ocrpt_query));
	memset(q, 0, sizeof(ocrpt_query));

	q->name = ocrpt_mem_strdup(name);
	if (!q->name) {
		ocrpt_mem_free(q);
		return NULL;
	}

	q->rownum = ocrpt_expr_parse(source->o, "r.rownum", NULL);
	if (!q->rownum) {
		ocrpt_mem_free(q->name);
		ocrpt_mem_free(q);
		return NULL;
	}

	opencreport *o = source->o;

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

DLL_EXPORT_SYM char *ocrpt_query_get_name(const ocrpt_query *query) {
	return (query ? query->name : NULL);
}

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_query_get_source(const ocrpt_query *query) {
	return (query ? (ocrpt_datasource *)query->source : NULL);
}

DLL_EXPORT_SYM void ocrpt_query_set_private(ocrpt_query *query, const void *priv) {
	if (!query)
		return;

	/* Be careful overwriting it. */
	query->priv = (void *)priv;
}

DLL_EXPORT_SYM void *ocrpt_query_get_private(const ocrpt_query *query) {
	return (query ? query->priv : NULL);
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_array(ocrpt_datasource *source, const char *name, const char **array, int32_t rows, int32_t cols, const int32_t *types, int32_t types_cols) {
	if (!source || !name || !array)
		return NULL;

	const ocrpt_input *input = ocrpt_datasource_get_input(source);

	if (!input || !input->query_add_array)
		return NULL;

	return input->query_add_array(source, name, array, rows, cols, types, types_cols);
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_symbolic_array(ocrpt_datasource *source, const char *name, const char *array_name, int32_t rows, int32_t cols, const char *types_name, int32_t types_cols) {
	if (!source || !name || !array_name)
		return NULL;

	const ocrpt_input *input = ocrpt_datasource_get_input(source);

	if (!input || !input->query_add_symbolic_array)
		return NULL;

	return input->query_add_symbolic_array(source, name, array_name, rows, cols, types_name, types_cols);
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_file(ocrpt_datasource *source,
												const char *name, const char *filename,
												const int32_t *types,
												int32_t types_cols) {
	if (!source || !source->input || !source->input->query_add_file || !name || !filename)
		return NULL;

	return source->input->query_add_file(source, name, filename, types, types_cols);
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_sql(ocrpt_datasource *source, const char *name, const char *querystr) {
	if (!source || !source->input || !source->input->query_add_sql || !name || !querystr)
		return NULL;

	return source->input->query_add_sql(source, name, querystr);
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_get(opencreport *o, const char *name) {
	if (!o || !name)
		return NULL;

	for (ocrpt_list *ptr = o->queries; ptr; ptr = ptr->next) {
		ocrpt_query *q = (ocrpt_query *)ptr->data;
		if (!strcmp(q->name, name))
			return q;
	}

	return NULL;
}

DLL_EXPORT_SYM const char *ocrpt_query_result_column_name(ocrpt_query_result *qr, int32_t col) {
	if (!qr)
		return NULL;

	return qr[col].name;
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_query_result_column_result(ocrpt_query_result *qr, int32_t col) {
	if (!qr)
		return NULL;

	return &qr[col].result;
}

void ocrpt_query_free0(ocrpt_query *q) {
	if (!q)
		return;

	if (q->leader) {
		q->leader->followers_n_to_1 = ocrpt_list_remove(q->leader->followers_n_to_1, q);
		q->leader->followers = ocrpt_list_remove(q->leader->followers, q);
	}

	if (q->source && q->source->input && q->source->input->free)
		q->source->input->free(q);

	ocrpt_expr_free(q->rownum);
	ocrpt_expr_free(q->match);
	ocrpt_query_result_free(q);
	ocrpt_strfree(q->name);
	q->name = NULL;

	ocrpt_list *l;

	for (l = q->followers; l; l = l->next) {
		ocrpt_query *q1 = (ocrpt_query *)l->data;
		q1->leader = NULL;
	}
	ocrpt_list_free(q->followers);

	for (l = q->followers_n_to_1; l; l = l->next) {
		ocrpt_query *q1 = (ocrpt_query *)l->data;
		q1->leader = NULL;
		q1->leader_is_n_1 = false;
	}
	ocrpt_list_free(q->followers_n_to_1);

	ocrpt_list_free(q->global_followers_n_to_1);

	ocrpt_mem_free(q);
}

DLL_EXPORT_SYM void ocrpt_query_free(ocrpt_query *q) {
	if (!q || !q->source || !q->source->o)
		return;

	opencreport *o = q->source->o;

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
	return (q->result ? &q->result[q->source->o->residx * q->cols] : NULL);
}

DLL_EXPORT_SYM void ocrpt_query_result_set_values_null(ocrpt_query *q) {
	opencreport *o = q->source->o;
	int32_t base = o->residx * q->cols;
	int32_t i;

	for (i = 0; i < q->cols; i++) {
		q->result[base + i].result.type = q->result[base + i].result.orig_type;
		q->result[base + i].result.isnull = true;
	}
}

DLL_EXPORT_SYM void ocrpt_query_result_set_value(ocrpt_query *q, int32_t i, bool isnull, iconv_t conv, const char *str, size_t len) {
	opencreport *o = q->source->o;
	int32_t base = o->residx * q->cols;
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

	enum ocrpt_result_type type = r->orig_type;
	switch (type) {
	case OCRPT_RESULT_NUMBER:
		if (!r->number_initialized)
			mpfr_init2(r->number, o->prec);
		if (!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "t"))
			str = "1";
		if (!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "f"))
			str = "0";
		mpfr_set_str(r->number, str, 10, o->rndmode);
		r->type = type;
		break;
	case OCRPT_RESULT_DATETIME:
		if (ocrpt_parse_datetime(o, str, len, r)) {
			r->type = type;
			break;
		} else if (ocrpt_parse_interval(o, str, len, r)) {
			r->type = type;
			break;
		}
		type = OCRPT_RESULT_ERROR;
		str = "invalid datetime or interval string";
		len = strlen(str);
		FALLTHROUGH;
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
		r->type = type;
		break;
	}
}

void ocrpt_query_result_free(ocrpt_query *q) {
	ocrpt_query_result *result = q->result;
	int32_t cols = q->cols, i;

	if (result && cols) {
		for (i = 0; i < OCRPT_EXPR_RESULTS * cols; i++) {
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

static bool ocrpt_query_follower_circular(ocrpt_query *leader, ocrpt_query *follower) {
	ocrpt_list *ptr;

	for (ptr = leader->followers; ptr; ptr = ptr->next) {
		ocrpt_query *fptr = (ocrpt_query *)ptr->data;
		if (fptr == follower)
			return false;
		if (!ocrpt_query_follower_circular(fptr, follower))
			return false;
	}

	for (ptr = leader->followers_n_to_1; ptr; ptr = ptr->next) {
		ocrpt_query *f = (ocrpt_query *)ptr->data;
		if (f == follower)
			return false;
		if (!ocrpt_query_follower_circular(f, follower))
			return false;
	}

	for (ptr = follower->followers; ptr; ptr = ptr->next) {
		ocrpt_query *fptr = (ocrpt_query *)ptr->data;
		if (fptr == leader)
			return false;
		if (!ocrpt_query_follower_circular(fptr, leader))
			return false;
	}

	for (ptr = follower->followers_n_to_1; ptr; ptr = ptr->next) {
		ocrpt_query *f = (ocrpt_query *)ptr->data;
		if (f == leader)
			return false;
		if (!ocrpt_query_follower_circular(f, leader))
			return false;
	}

	return true;
}

static bool ocrpt_query_follower_validity(ocrpt_query *leader, ocrpt_query *follower) {
	if (!leader) {
		ocrpt_err_printf("leader query unset\n");
		return false;
	}

	if (!leader->source) {
		ocrpt_err_printf("%s leader query's source unset\n", leader->name);
		return false;
	}

	if (!follower) {
		ocrpt_err_printf("follower query unset\n");
		return false;
	}

	if (!follower->source) {
		ocrpt_err_printf("%s follower query's source unset\n", follower->name);
		return false;
	}

	if (leader->source->o != follower->source->o) {
		ocrpt_err_printf("Parent pointers for leader ('%s') and follower ('%s') do not match\n", leader->name, follower->name);
		return false;
	}

	if (leader == follower) {
		ocrpt_err_printf("leader and follower queries cannot be identical\n");
		return false;
	}

	if (!ocrpt_query_follower_circular(leader, follower)) {
		ocrpt_err_printf("%s:%s follower would create a circular graph for %s:%s leader\n", follower->source->name, follower->name, leader->source->name, leader->name);
		return false;
	}

	if (follower->leader) {
		ocrpt_err_printf("Follower ('%s') is already a follower of '%s'\n", follower->name, follower->leader->name);
		return false;
	}

	return true;
}

DLL_EXPORT_SYM bool ocrpt_query_add_follower_n_to_1(ocrpt_query *leader, ocrpt_query *follower, ocrpt_expr *match) {
	if (!ocrpt_query_follower_validity(leader, follower)) {
		ocrpt_expr_free(match);
		return false;
	}

	if (ocrpt_expr_references(match, OCRPT_VARREF_MVAR | OCRPT_VARREF_RVAR | OCRPT_VARREF_VVAR, NULL)) {
		ocrpt_err_printf("invalid expression for follower query\n");
		ocrpt_expr_free(match);
		return false;
	}

	uint32_t mask = 0;
	if (ocrpt_expr_references(match, OCRPT_VARREF_IDENT, &mask)) {
		if ((mask & OCRPT_IDENT_UNKNOWN_BIT) == OCRPT_IDENT_UNKNOWN_BIT) {
			ocrpt_err_printf("invalid expression for follower query\n");
			ocrpt_expr_free(match);
			return false;
		}
	}

	int32_t len = ocrpt_list_length(leader->followers_n_to_1);
	leader->followers_n_to_1 = ocrpt_list_append(leader->followers_n_to_1, follower);

	if (len >= ocrpt_list_length(leader->followers_n_to_1)) {
		ocrpt_expr_free(match);
		return false;
	}

	ocrpt_expr_optimize(match);
	ocrpt_expr_resolve(match);

	follower->leader = leader;
	follower->leader_is_n_1 = true;
	follower->match = match;

	if (follower->followers_n_to_1) {
		if (ocrpt_list_length(leader->followers_n_to_1) > 0) {
			ocrpt_list *ptr = ocrpt_list_last(leader->followers_n_to_1);
			ptr->next = follower->followers_n_to_1;
			leader->followers_n_to_1->len += ocrpt_list_length(follower->followers_n_to_1);
		} else
			leader->followers_n_to_1 = follower->followers_n_to_1;

		follower->followers_n_to_1 = NULL;
	}

	leader->source->o->n_to_1_lists_invalid = true;

	return true;
}

DLL_EXPORT_SYM bool ocrpt_query_add_follower(ocrpt_query *leader, ocrpt_query *follower) {
	if (!ocrpt_query_follower_validity(leader, follower))
		return false;

	int32_t len = ocrpt_list_length(leader->followers);
	leader->followers = ocrpt_list_append(leader->followers, follower);
	if (len >= ocrpt_list_length(leader->followers))
		return false;

	follower->leader = leader;
	follower->leader_is_n_1 = false;

	return true;
}

static void ocrpt_query_finalize_followers0(ocrpt_query *topq, ocrpt_query *q) {
	ocrpt_list *ptr;

	for (ptr = q->followers_n_to_1; ptr; ptr = ptr->next) {
		ocrpt_query *q1 = (ocrpt_query *)ptr->data;

		topq->global_followers_n_to_1 = ocrpt_list_prepend(topq->global_followers_n_to_1, q1);
	}

	for (ptr = q->followers; ptr; ptr = ptr->next)
		ocrpt_query_finalize_followers0(topq, (ocrpt_query *)ptr->data);

	for (ptr = q->followers_n_to_1; ptr; ptr = ptr->next)
		ocrpt_query_finalize_followers0(topq, (ocrpt_query *)ptr->data);
}

void ocrpt_query_finalize_followers(ocrpt_query *q) {
	if (!q->source->o->n_to_1_lists_invalid && q->global_followers_n_to_1)
		return;

	ocrpt_list_free(q->global_followers_n_to_1);
	q->global_followers_n_to_1 = NULL;

	ocrpt_query_finalize_followers0(q, q);

	q->source->o->n_to_1_lists_invalid = false;
}

DLL_EXPORT_SYM bool ocrpt_datasource_is_array(ocrpt_datasource *source) {
	return source && source->input && source->input->query_add_array;
}

DLL_EXPORT_SYM bool ocrpt_datasource_is_symbolic_array(ocrpt_datasource *source) {
	return source && source->input && source->input->query_add_symbolic_array;
}

DLL_EXPORT_SYM bool ocrpt_datasource_is_file(ocrpt_datasource *source) {
	return source && source->input && source->input->query_add_file;
}

DLL_EXPORT_SYM bool ocrpt_datasource_is_sql(ocrpt_datasource *source) {
	return source && source->input && source->input->query_add_sql;
}

DLL_EXPORT_SYM bool ocrpt_query_refresh(opencreport *o) {
	if (!o)
		return true;

	bool ret = true;

	for (ocrpt_list *ql = o->queries; ql; ql = ql->next) {
		ocrpt_query *q = (ocrpt_query *)ql->data;

		if (q->source->input->refresh) {
			bool success = q->source->input->refresh(q);

			ret = ret && success;
		}
	}

	return ret;
}
