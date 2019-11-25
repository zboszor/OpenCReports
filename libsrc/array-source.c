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

#include <opencreport-private.h>
#include "datasource.h"

struct ocrpt_array_results {
	char *name;
	const char **data;
	const enum ocrpt_result_type *types;
	int32_t rows;
	int32_t current_row;
	bool atstart;
	bool isdone;
};

static void ocrpt_array_rewind(ocrpt_query *query) {
	struct ocrpt_array_results *result = query->priv;

	if (result == NULL)
		return;

	result->current_row = 0;
	result->atstart = true;
	result->isdone = false;
}

static bool ocrpt_array_next(ocrpt_query *query) {
	opencreport *o = query->source->o;
	struct ocrpt_array_results *result = query->priv;
	int i;

	if (result == NULL)
		return false;

	result->atstart = false;
	result->current_row++;
	result->isdone = (result->current_row > result->rows);

	if (result->isdone) {
		for (i = 0; i < query->cols; i++)
			query->result[i].result.isnull = true;
		return false;
	}

	for (i = 0; i < query->cols; i++) {
		int32_t dataidx = result->current_row * query->cols + i;
		const char *str;

		//fprintf(stderr, "%s:%d: cols: %d, rows: %d, current row: %d current col: %d, computed idx: %d\n", __func__, __LINE__, query->cols, result->rows, result->current_row, i, dataidx);
		str = result->data[dataidx];

		if (str) {
			query->result[i].result.isnull = false;

			if (query->result[i].result.string_owned)
				ocrpt_strfree(query->result[i].result.string);
			query->result[i].result.string = str;
			query->result[i].result.string_owned = false;

			if (query->result[i].result.type == OCRPT_RESULT_NUMBER) {
				if (!query->result[i].result.number_initialized)
					mpfr_init2(query->result[i].result.number, o->prec);
				mpfr_set_str(query->result[i].result.number, str, 10, o->rndmode);
			}
		} else {
			query->result[i].result.isnull = true;
			if (query->result[i].result.number_initialized)
				mpfr_set_ui(query->result[i].result.number, 0, o->rndmode);
		}
	}

	return true;
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
	.rewind = ocrpt_array_rewind,
	.next = ocrpt_array_next,
	.isdone = ocrpt_array_isdone,
	.free = ocrpt_array_free
};

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_add_array_datasource(opencreport *o, const char *input_name) {
	return ocrpt_add_datasource(o, input_name, &ocrpt_array_input);
}

static ocrpt_query *add_array_query(opencreport *o, const ocrpt_datasource *source, const char *name, void *array, int32_t rows, int32_t cols, const enum ocrpt_result_type *types) {
	ocrpt_query *query;
	struct ocrpt_array_results *priv;
	ocrpt_query_result *result;
	int i;

	query = ocrpt_alloc_query(o, source, name);
	if (!query)
		return NULL;

	query->source = source;
	query->cols = cols;

	priv = ocrpt_mem_malloc(sizeof(struct ocrpt_array_results));
	if (!priv) {
		ocrpt_free_query(o, query);
		return NULL;
	}

	memset(priv, 0, sizeof(struct ocrpt_array_results));
	priv->rows = rows;
	priv->data = array;
	priv->types = types;
	priv->current_row = 0;
	priv->atstart = true;
	priv->isdone = false;
	query->priv = priv;

	result = ocrpt_mem_malloc(cols * sizeof(ocrpt_query_result));
	if (!result) {
		ocrpt_free_query(o, query);
		return NULL;
	}

	memset(result, 0, cols * sizeof(ocrpt_query_result));
	for (i = 0; i < cols; i++) {
		result[i].name = priv->data[i];
		if (types)
			result[i].result.type = types[i];
		else
			result[i].result.type = OCRPT_RESULT_STRING;

		if (result[i].result.type == OCRPT_RESULT_NUMBER) {
			mpfr_init2(result[i].result.number, o->prec);
			result[i].result.number_initialized = true;
		}
	}

	query->result = result;

	return query;
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_add_array_query(opencreport *o, ocrpt_datasource *source, const char *name, void *array, int32_t rows, int32_t cols, const enum ocrpt_result_type *types) {
	if (!ocrpt_validate_datasource(o, source))
		return NULL;

	return add_array_query(o, source, name, array, rows, cols, types);
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_add_array_query_as(opencreport *o, const char *source_name, const char *name, void *array, int32_t rows, int32_t cols, const enum ocrpt_result_type *types) {
	ocrpt_datasource *source = ocrpt_find_datasource(o, source_name);

	if (!source)
		return NULL;

	return add_array_query(o, source, name, array, rows, cols, types);
}
