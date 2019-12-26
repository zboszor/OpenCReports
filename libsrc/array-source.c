/*
 * OpenCReports array data source
 *
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "opencreport-private.h"
#include "datasource.h"

struct ocrpt_array_results {
	char *name;
	const char **data;
	const enum ocrpt_result_type *types;
	ocrpt_query_result *result;
	int32_t rows;
	int32_t cols;
	int32_t current_row;
	bool atstart;
	bool isdone;
};

static void ocrpt_array_describe(ocrpt_query *query, ocrpt_query_result **qresult, int32_t *cols) {
	struct ocrpt_array_results *result = query->priv;
	int32_t i;

	if (!result->result) {
		ocrpt_query_result *qr = ocrpt_mem_malloc(2 * result->cols * sizeof(ocrpt_query_result));

		if (!qr) {
			if (qresult)
				*qresult = NULL;
			if (cols)
				*cols = 0;
			return;
		}

		memset(qr, 0, 2 * result->cols * sizeof(ocrpt_query_result));

		for (i = 0; i < result->cols; i++) {
			qr[i].name = result->data[i];
			qr[result->cols + i].name = result->data[i];

			if (result->types) {
				qr[i].result.type = result->types[i];
				qr[result->cols + i].result.type = result->types[i];
			} else {
				qr[i].result.type = OCRPT_RESULT_STRING;
				qr[result->cols + i].result.type = OCRPT_RESULT_STRING;
			}

			if (qr[i].result.type == OCRPT_RESULT_NUMBER) {
				mpfr_init2(qr[i].result.number, query->source->o->prec);
				qr[i].result.number_initialized = true;
				mpfr_init2(qr[result->cols + i].result.number, query->source->o->prec);
				qr[result->cols + i].result.number_initialized = true;
			}

			qr[i].result.isnull = true;
			qr[result->cols + i].result.isnull = true;
		}

		result->result = qr;
	}

	if (qresult)
		*qresult = result->result;
	if (cols)
		*cols = result->cols;
}

static void ocrpt_array_rewind(ocrpt_query *query) {
	struct ocrpt_array_results *result = query->priv;

	if (result == NULL)
		return;

	result->current_row = 0;
	result->atstart = true;
	result->isdone = false;
}

static bool ocrpt_array_populate_result(ocrpt_query *query) {
	opencreport *o = query->source->o;
	struct ocrpt_array_results *result = query->priv;
	int i, base;

	base = (o->residx ? query->cols: 0);

	if (result->isdone) {
		for (i = 0; i < query->cols; i++)
			query->result[base + i].result.isnull = true;
		return false;
	}

	for (i = 0; i < query->cols; i++) {
		int32_t dataidx = result->current_row * query->cols + i;
		ocrpt_result *r = &query->result[base + i].result;
		const char *str;

		//fprintf(stderr, "%s:%d: cols: %d, rows: %d, current row: %d current col: %d, computed idx: %d\n", __func__, __LINE__, query->cols, result->rows, result->current_row, i, dataidx);
		str = result->data[dataidx];

		if (str) {
			r->isnull = false;

			ocrpt_mem_string_free(r->string, r->string_owned);
			r->string = ocrpt_mem_string_new(str, false);
			r->string_owned = false;

			if (r->type == OCRPT_RESULT_NUMBER) {
				if (!r->number_initialized)
					mpfr_init2(r->number, o->prec);
				if (!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "t"))
					str = "1";
				if (!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "f"))
					str = "0";
				mpfr_set_str(r->number, str, 10, o->rndmode);
			}
		} else {
			r->isnull = true;
			if (r->number_initialized)
				mpfr_set_ui(r->number, 0, o->rndmode);
		}
	}

	return true;
}

static bool ocrpt_array_next(ocrpt_query *query) {
	struct ocrpt_array_results *result = query->priv;

	if (result == NULL)
		return false;

	result->atstart = false;
	result->current_row++;
	result->isdone = (result->current_row > result->rows);

	return ocrpt_array_populate_result(query);
}

static bool ocrpt_array_isdone(ocrpt_query *query) {
	struct ocrpt_array_results *result = query->priv;

	if (result == NULL)
		return true;

	return result->isdone;
}

static void ocrpt_array_free(ocrpt_query *query) {
	ocrpt_mem_free(query->priv);
	query->priv = NULL;
}

static const ocrpt_input ocrpt_array_input = {
	.describe = ocrpt_array_describe,
	.rewind = ocrpt_array_rewind,
	.next = ocrpt_array_next,
	.populate_result = ocrpt_array_populate_result,
	.isdone = ocrpt_array_isdone,
	.free = ocrpt_array_free
};

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_add_array(opencreport *o, const char *source_name) {
	return ocrpt_datasource_add(o, source_name, &ocrpt_array_input);
}

static ocrpt_query *array_query_add(opencreport *o, const ocrpt_datasource *source, const char *name, const char **array, int32_t rows, int32_t cols, const enum ocrpt_result_type *types) {
	ocrpt_query *query;
	struct ocrpt_array_results *priv;

	query = ocrpt_query_alloc(o, source, name);
	if (!query)
		return NULL;

	query->source = source;
	query->cols = cols;

	priv = ocrpt_mem_malloc(sizeof(struct ocrpt_array_results));
	if (!priv) {
		ocrpt_query_free(o, query);
		return NULL;
	}

	memset(priv, 0, sizeof(struct ocrpt_array_results));
	priv->rows = rows;
	priv->cols = cols;
	priv->data = array;
	priv->types = types;
	priv->current_row = 0;
	priv->atstart = true;
	priv->isdone = false;
	query->priv = priv;

	return query;
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_array(opencreport *o, ocrpt_datasource *source, const char *name, const char **array, int32_t rows, int32_t cols, const enum ocrpt_result_type *types) {
	if (!ocrpt_datasource_validate(o, source))
		return NULL;

	return array_query_add(o, source, name, array, rows, cols, types);
}

DLL_EXPORT_SYM ocrpt_query_discover_func ocrpt_query_discover_array = ocrpt_query_discover_array_c;

DLL_EXPORT_SYM void ocrpt_query_set_discover_func(ocrpt_query_discover_func func) {
	ocrpt_query_discover_array = func;
}

DLL_EXPORT_SYM void ocrpt_query_discover_array_c(const char *arrayname, void **array, const char *typesname, void **types) {
	void *handle = dlopen(NULL, RTLD_NOW);

	if (array) {
		if (arrayname && *arrayname) {
			dlerror();
			*array = dlsym(handle, arrayname);
			dlerror();
		} else
			*array = NULL;
	}
	if (types) {
		if (typesname && *typesname) {
			dlerror();
			*types = dlsym(handle, typesname);
			dlerror();
		} else
			*types = NULL;
	}

	dlclose(handle);
}
