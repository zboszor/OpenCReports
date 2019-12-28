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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <csv.h>

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

const ocrpt_input ocrpt_array_input = {
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
	if (!ocrpt_datasource_validate(o, source)) {
		fprintf(stderr, "%s:%d: datasource is not for this opencreport structure\n", __func__, __LINE__);
		return NULL;
	}

	if (source->input != &ocrpt_array_input) {
		fprintf(stderr, "%s:%d: datasource is not array\n", __func__, __LINE__);
		return NULL;
	}

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

static void ocrpt_csv_free(ocrpt_query *query) {
	struct ocrpt_array_results *result = query->priv;
	int32_t i;
	int32_t max = (result->rows + 1) * result->cols;

	for (i = 0; i < max; i++)
		ocrpt_mem_free(result->data[i]);

	ocrpt_mem_free(result->data);
	ocrpt_mem_free(query->priv);
	query->priv = NULL;
}

const ocrpt_input ocrpt_csv_input = {
	.describe = ocrpt_array_describe,
	.rewind = ocrpt_array_rewind,
	.next = ocrpt_array_next,
	.populate_result = ocrpt_array_populate_result,
	.isdone = ocrpt_array_isdone,
	.free = ocrpt_csv_free
};

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_add_csv(opencreport *o, const char *source_name) {
	return ocrpt_datasource_add(o, source_name, &ocrpt_csv_input);
}

struct ocrpt_file_query {
	ocrpt_list *rowlist;
	ocrpt_list *row_last;
	ocrpt_list *collist;
	ocrpt_list *col_last;
	enum ocrpt_result_type *types;
	int32_t rows;
	int32_t cols;
	bool firstrow:1;
	bool firstcol:1;
};
typedef struct ocrpt_file_query ocrpt_file_query;

static void csv_parser_field_cb(void *field_ptr, size_t field_sz, void *user_data) {
	struct ocrpt_file_query *fq = user_data;
	char *field;

	if (!fq->firstrow && (ocrpt_list_length(fq->collist) >= fq->cols))
		return;

	field = ocrpt_mem_malloc(field_sz + 1);
	if (field) {
		char *end = stpncpy(field, field_ptr, field_sz);
		*end = 0;
	}

	if (fq->firstcol) {
		fq->firstcol = false;
		fq->collist = fq->col_last = ocrpt_makelist1(field);
		fq->rowlist = ocrpt_list_end_append(fq->rowlist, &fq->row_last, fq->collist);
	} else {
		fq->collist = ocrpt_list_end_append(fq->collist, &fq->col_last, field);
	}

	if (fq->firstrow)
		fq->cols++;
}

static void csv_parser_eol_cb(int eolchar UNUSED, void *user_data) {
	struct ocrpt_file_query *fq = user_data;

	fq->rows++;
	fq->firstrow = false;
	fq->firstcol = true;
	fq->collist = NULL;
	fq->col_last = NULL;
}

static void ocrpt_file_query_free_rows(const void *ptr) {
	ocrpt_list_free_deep((ocrpt_list *)ptr, ocrpt_mem_free);
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_csv(opencreport *o, ocrpt_datasource *source,
												const char *name, const char *filename,
												const enum ocrpt_result_type *types) {
	struct stat st;
	struct csv_parser csv;
	char *buf;
	const char **array;
	ocrpt_query *retval;
	ocrpt_file_query *fq;
	ocrpt_list *rowptr;
	size_t parser_retval;
	int32_t fd, row;

	if (!ocrpt_datasource_validate(o, source)) {
		fprintf(stderr, "%s:%d: datasource is not for this opencreport structure\n", __func__, __LINE__);
		return NULL;
	}

	if (source->input != &ocrpt_csv_input) {
		fprintf(stderr, "%s:%d: datasource is not csv\n", __func__, __LINE__);
		return NULL;
	}

	if (stat(filename, &st) != 0) {
		fprintf(stderr, "%s: error opening file\n", __func__);
		return NULL;
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: error opening file\n", __func__);
		return NULL;
	}

	buf = ocrpt_mem_malloc(st.st_size);
	if (!buf) {
		close(fd);
		fprintf(stderr, "%s: out of memory\n", __func__);
		return NULL;
	}

	if (read(fd, buf, st.st_size) != st.st_size) {
		close(fd);
		ocrpt_mem_free(buf);
		fprintf(stderr, "%s: error reading file\n", __func__);
		return NULL;
	}

	close(fd);

	if (csv_init(&csv, CSV_STRICT | CSV_REPALL_NL | CSV_STRICT_FINI | CSV_APPEND_NULL) != 0) {
		ocrpt_mem_free(buf);
		fprintf(stderr, "%s: error initializing CSV parser\n", __func__);
		return NULL;
	}

	fq = ocrpt_mem_malloc(sizeof(ocrpt_file_query));
	if (!fq) {
		ocrpt_mem_free(buf);
		csv_free(&csv);
		fprintf(stderr, "%s: out of memory\n", __func__);
		return NULL;
	}

	memset(fq, 0, sizeof(ocrpt_file_query));
	fq->firstrow = true;
	fq->firstcol = true;

	parser_retval = csv_parse(&csv, buf, st.st_size, csv_parser_field_cb, csv_parser_eol_cb, fq);
	if (parser_retval != st.st_size) {
		ocrpt_mem_free(buf);
		csv_free(&csv);
		ocrpt_list_free_deep(fq->rowlist, ocrpt_file_query_free_rows);
		ocrpt_mem_free(fq);
		fprintf(stderr, "%s: parsing CSV file \"%s\" failed\n", __func__, filename);
		return NULL;
	}

	csv_free(&csv);

	ocrpt_mem_free(buf);

	array = ocrpt_mem_malloc(fq->rows * fq->cols * sizeof(char *));
	if (!array) {
		ocrpt_list_free_deep(fq->rowlist, ocrpt_file_query_free_rows);
		ocrpt_mem_free(fq);
		fprintf(stderr, "%s: out of memory\n", __func__);
		return NULL;
	}

	for (rowptr = fq->rowlist, row = 0; rowptr; rowptr = rowptr->next, row++) {
		const ocrpt_list *colptr;
		int32_t col;

		for (colptr = rowptr->data, col = 0; colptr; colptr = colptr->next, col++)
			array[row * fq->cols + col] = colptr->data;

		for (; col < fq->cols; col++)
			array[row * fq->cols + col] = NULL;
	}

	retval = array_query_add(o, source, name, array, fq->rows - 1, fq->cols, types);

	ocrpt_list_free_deep(fq->rowlist, (ocrpt_mem_free_t)ocrpt_list_free);
	ocrpt_mem_free(fq);

	return retval;
}
