/*
 * OpenCReports spreadsheet based data sources
 *
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#include <config.h>

#include <string.h>

#include "opencreport.h"
#include "datasource.h"

#if HAVE_LIBPYTHON

/* This must be after #include <datasource.h> */
#define PY_SSIZE_T_CLEAN
#include <Python.h>

struct ocrpt_pandas_conn_private {
	char *filename;
};
typedef struct ocrpt_pandas_conn_private ocrpt_pandas_conn_private;

struct ocrpt_pandas_results {
	ocrpt_query_result *result;

	int32_t *types;
	int32_t cols;
	int32_t row;
	bool atstart:1;
	bool isdone:1;
};
typedef struct ocrpt_pandas_results ocrpt_pandas_results;

static const ocrpt_input_connect_parameter ocrpt_pandas_connect_method1[] = {
	{ .param_name = "filename", { .optional = false } },
	{ .param_name = NULL }
};

static const ocrpt_input_connect_parameter *ocrpt_pandas_connect_methods[] = {
	ocrpt_pandas_connect_method1,
	NULL
};

static bool ocrpt_pandas_connect(ocrpt_datasource *source, const ocrpt_input_connect_parameter *params) {
	if (!source || !params)
		return false;

	char *filename = NULL;

	for (int32_t i = 0; params[i].param_name; i++) {
		if (strcasecmp(params[i].param_name, "filename") == 0)
			filename = params[i].param_value;
	}

	if (!filename)
		return false;

	ocrpt_pandas_conn_private *priv = ocrpt_mem_malloc(sizeof(ocrpt_pandas_conn_private));
	if (!priv)
		return false;

	bool file_opened = false;

	if (file_opened) {
		priv->filename = ocrpt_mem_strdup(filename);

		ocrpt_datasource_set_private(source, priv);
	} else
		ocrpt_mem_free(priv);

	return file_opened;
}

static ocrpt_query *ocrpt_pandas_query_add(ocrpt_datasource *source,
										const char *name, const char *sheet_name,
										const int32_t *types,
										int32_t types_cols) {
	//ocrpt_pandas_conn_private *priv = ocrpt_datasource_get_private(source);
	ocrpt_query *query = ocrpt_query_alloc(source, name);

	if (!query)
		return NULL;

	int32_t cols = 0;
	int32_t *column_types = NULL;

	struct ocrpt_pandas_results *result = ocrpt_mem_malloc(sizeof(struct ocrpt_pandas_results));

	if (!result) {
		ocrpt_query_free(query);
		return NULL;
	}

	memset(result, 0, sizeof(struct ocrpt_pandas_results));

	result->types = column_types;
	result->cols = cols;

	ocrpt_query_set_private(query, result);

	return query;
}

static void ocrpt_pandas_describe(ocrpt_query *query, ocrpt_query_result **qresult, int32_t *cols) {
	struct ocrpt_pandas_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	//ocrpt_pandas_conn_private *priv = ocrpt_datasource_get_private(source);
	opencreport *o = ocrpt_datasource_get_opencreport(source);
	int32_t i;

	if (!result->result) {
		ocrpt_query_result *qr = ocrpt_mem_malloc(OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));

		if (!qr) {
			if (qresult)
				*qresult = NULL;
			if (cols)
				*cols = 0;
			return;
		}

		memset(qr, 0, OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));

		for (i = 0; i < result->cols; i++) {
			for (int j = 0; j < OCRPT_EXPR_RESULTS; j++) {
				qr[j * result->cols + i].result.o = o;

				enum ocrpt_result_type type;
				if (result->types)
					type = result->types[i];
				else
					type = OCRPT_RESULT_STRING;
				qr[j * result->cols + i].result.type = type;
				qr[j * result->cols + i].result.orig_type = type;

				if (qr[i].result.type == OCRPT_RESULT_NUMBER) {
					mpfr_init2(qr[j * result->cols + i].result.number, ocrpt_get_numeric_precision_bits(o));
					qr[j * result->cols + i].result.number_initialized = true;
				}

				qr[j * result->cols + i].result.isnull = true;
			}
		}

		result->result = qr;
	}

	if (qresult)
		*qresult = result->result;
	if (cols)
		*cols = result->cols;
}

static bool ocrpt_pandas_populate_result(ocrpt_query *query) {
	struct ocrpt_pandas_results *result = ocrpt_query_get_private(query);
	int32_t i;

	if (result->atstart || result->isdone) {
		ocrpt_query_result_set_values_null(query);
		return false;
	}

	for (i = 0; i < result->cols; i++) {
		/* TODO
		 * ...
		ocrpt_query_result_set_value(query, i, isnull, (iconv_t)-1, str, len);
		 */
	}

	return true;
}

static void ocrpt_pandas_rewind(ocrpt_query *query) {
}

static bool ocrpt_pandas_next(ocrpt_query *query) {
	return ocrpt_pandas_populate_result(query);
}

static bool ocrpt_pandas_isdone(ocrpt_query *query) {
	ocrpt_pandas_results *result = ocrpt_query_get_private(query);

	return result->isdone;
}

static void ocrpt_pandas_free(ocrpt_query *query) {
	ocrpt_pandas_results *result = ocrpt_query_get_private(query);

	ocrpt_mem_free(result->types);
	ocrpt_query_set_private(query, NULL);
	ocrpt_mem_free(result);
}

static void ocrpt_pandas_close(const ocrpt_datasource *source) {
	ocrpt_pandas_conn_private *priv = ocrpt_datasource_get_private(source);

	ocrpt_mem_free(priv);
	ocrpt_datasource_set_private((ocrpt_datasource *)source, NULL);
}

static const char *ocrpt_pandas_input_names[] = {
	"pandas", "spreadsheet", "ods", "xls", "xlsx", NULL
};

const ocrpt_input ocrpt_pandas_input = {
	.names = ocrpt_pandas_input_names,
	.connect_parameters = ocrpt_pandas_connect_methods,
	.connect = ocrpt_pandas_connect,
	.query_add_file = ocrpt_pandas_query_add,
	.describe = ocrpt_pandas_describe,
	.rewind = ocrpt_pandas_rewind,
	.next = ocrpt_pandas_next,
	.populate_result = ocrpt_pandas_populate_result,
	.isdone = ocrpt_pandas_isdone,
	.free = ocrpt_pandas_free,
	.close = ocrpt_pandas_close
};

#endif /* HAVE_LIBPYTHON */
