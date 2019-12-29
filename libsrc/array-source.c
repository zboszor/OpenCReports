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
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>

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
	bool atstart:1;
	bool isdone:1;
	bool free_types:1;
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
	.type = OCRPT_INPUT_ARRAY,
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

static ocrpt_query *array_query_add(opencreport *o, const ocrpt_datasource *source, const char *name,
									const char **array, int32_t rows, int32_t cols,
									const enum ocrpt_result_type *types, bool free_types) {
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
	priv->free_types = free_types;
	query->priv = priv;

	return query;
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_array(opencreport *o, ocrpt_datasource *source, const char *name, const char **array, int32_t rows, int32_t cols, const enum ocrpt_result_type *types) {
	if (!ocrpt_datasource_validate(o, source)) {
		fprintf(stderr, "%s:%d: datasource is not for this opencreport structure\n", __func__, __LINE__);
		return NULL;
	}

	if (source->input->type != OCRPT_INPUT_ARRAY) {
		fprintf(stderr, "%s:%d: datasource is not array\n", __func__, __LINE__);
		return NULL;
	}

	return array_query_add(o, source, name, array, rows, cols, types, false);
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

static void ocrpt_file_free(ocrpt_query *query) {
	struct ocrpt_array_results *result = query->priv;
	int32_t i;
	int32_t max = (result->rows + 1) * result->cols;

	for (i = 0; i < max; i++)
		ocrpt_mem_free(result->data[i]);

	if (result->free_types)
		ocrpt_mem_free(result->types);
	ocrpt_mem_free(result->data);
	ocrpt_mem_free(query->priv);
	query->priv = NULL;
}

static const ocrpt_input ocrpt_csv_input = {
	.type = OCRPT_INPUT_CSV,
	.describe = ocrpt_array_describe,
	.rewind = ocrpt_array_rewind,
	.next = ocrpt_array_next,
	.populate_result = ocrpt_array_populate_result,
	.isdone = ocrpt_array_isdone,
	.free = ocrpt_file_free
};

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_add_csv(opencreport *o, const char *source_name) {
	return ocrpt_datasource_add(o, source_name, &ocrpt_csv_input);
}

/*
 * Common structure for all file based query types'
 * internal processing
 */
struct ocrpt_file_query {
	ocrpt_list *rowlist;
	ocrpt_list *row_last;
	ocrpt_list *collist;
	ocrpt_list *col_last;
	const char **names;
	const char **fields;
	enum ocrpt_result_type *types;
	int32_t rows;
	int32_t cols;
	int32_t depth;
	int32_t current_col;
	bool firstrow:1;
	bool firstcol:1;
	bool inheader:1;
	bool incoltypes:1;
	bool coltypesset:1;
	bool inrows:1;
	bool inrow:1;
	bool inrowvalue:1;
};
typedef struct ocrpt_file_query ocrpt_file_query;

static void csv_parser_field_cb(void *field_ptr, size_t field_sz, void *user_data) {
	struct ocrpt_file_query *fq = user_data;
	char *field = NULL;

	if (!fq->firstrow && (ocrpt_list_length(fq->collist) >= fq->cols))
		return;

	if (field_ptr) {
		field = ocrpt_mem_malloc(field_sz + 1);
		if (field) {
			char *end = stpncpy(field, field_ptr, field_sz);
			*end = 0;
		}
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
	if (fq->firstrow)
		fq->firstrow = false;
	fq->firstcol = true;
	fq->collist = NULL;
	fq->col_last = NULL;
}

static void ocrpt_file_query_free_rows(const void *ptr) {
	ocrpt_list_free_deep((ocrpt_list *)ptr, ocrpt_mem_free);
}

static void ocrpt_file_query_free(ocrpt_file_query *fq, bool in_error) {
	ocrpt_mem_free(fq->names);
	ocrpt_mem_free(fq->fields);

	if (in_error) {
		ocrpt_mem_free(fq->types);
		ocrpt_list_free_deep(fq->rowlist, ocrpt_file_query_free_rows);
	} else
		ocrpt_list_free_deep(fq->rowlist, (ocrpt_mem_free_t)ocrpt_list_free);
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_csv(opencreport *o, ocrpt_datasource *source,
												const char *name, const char *filename,
												const enum ocrpt_result_type *types) {
	struct stat st;
	struct csv_parser csv;
	char *buf;
	const char **array;
	ocrpt_query *retval;
	ocrpt_file_query fq = {};
	ocrpt_list *rowptr;
	size_t parser_retval;
	int32_t fd, row;

	if (!ocrpt_datasource_validate(o, source)) {
		fprintf(stderr, "%s:%d: datasource is not for this opencreport structure\n", __func__, __LINE__);
		return NULL;
	}

	if (source->input->type != OCRPT_INPUT_CSV) {
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

	if (csv_init(&csv, CSV_STRICT | CSV_REPALL_NL | CSV_STRICT_FINI | CSV_APPEND_NULL | CSV_EMPTY_IS_NULL) != 0) {
		ocrpt_mem_free(buf);
		fprintf(stderr, "%s: error initializing CSV parser\n", __func__);
		return NULL;
	}

	memset(&fq, 0, sizeof(ocrpt_file_query));
	fq.firstrow = true;
	fq.firstcol = true;

	parser_retval = csv_parse(&csv, buf, st.st_size, csv_parser_field_cb, csv_parser_eol_cb, &fq);
	if (parser_retval != st.st_size) {
		ocrpt_mem_free(buf);
		csv_free(&csv);
		ocrpt_file_query_free(&fq, true);
		fprintf(stderr, "%s: parsing CSV file \"%s\" failed\n", __func__, filename);
		return NULL;
	}

	csv_free(&csv);

	ocrpt_mem_free(buf);

	array = ocrpt_mem_malloc(fq.rows * fq.cols * sizeof(char *));
	if (!array) {
		ocrpt_file_query_free(&fq, true);
		fprintf(stderr, "%s: out of memory\n", __func__);
		return NULL;
	}

	for (rowptr = fq.rowlist, row = 0; rowptr; rowptr = rowptr->next, row++) {
		const ocrpt_list *colptr;
		int32_t col;

		for (colptr = rowptr->data, col = 0; colptr; colptr = colptr->next, col++)
			array[row * fq.cols + col] = colptr->data;

		for (; col < fq.cols; col++)
			array[row * fq.cols + col] = NULL;
	}

	retval = array_query_add(o, source, name, array, fq.rows - 1, fq.cols, types, false);

	ocrpt_file_query_free(&fq, false);

	return retval;
}

static const ocrpt_input ocrpt_json_input = {
	.type = OCRPT_INPUT_JSON,
	.describe = ocrpt_array_describe,
	.rewind = ocrpt_array_rewind,
	.next = ocrpt_array_next,
	.populate_result = ocrpt_array_populate_result,
	.isdone = ocrpt_array_isdone,
	.free = ocrpt_file_free
};

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_add_json(opencreport *o, const char *source_name) {
	return ocrpt_datasource_add(o, source_name, &ocrpt_json_input);
}

static int ocrpt_yajl_null(void *ctx) {
	return false;
}

static int ocrpt_yajl_set_field(ocrpt_file_query *fq, const char *str, size_t len) {
	char *field;

	if (fq->depth == 3 && fq->inrows && fq->inrowvalue) {
		/* Ignore unknown elements */
		if (fq->current_col < 0)
			return true;
		field = ocrpt_mem_malloc(len + 1);
		if (field) {
			char *end = stpncpy(field, (char *)str, len);
			*end = 0;
		}
		ocrpt_mem_free(fq->fields[fq->current_col]);
		fq->fields[fq->current_col] = field;
		fq->inrowvalue = false;
		return true;
	}
	return false;
}

static int ocrpt_yajl_number(void *ctx, const char *str, size_t len) {
	ocrpt_file_query *fq = ctx;

	return ocrpt_yajl_set_field(fq, str, len);
}

static int ocrpt_yajl_string(void *ctx, const unsigned char *str, size_t len) {
	ocrpt_file_query *fq = ctx;
	char *field;

	if (fq->depth == 2 && fq->inheader) {
		field = ocrpt_mem_malloc(len + 1);

		if (field) {
			char *end = stpncpy(field, (char *)str, len);
			*end = 0;
		}

		fq->collist = ocrpt_list_end_append(fq->collist, &fq->col_last, field);
		fq->cols++;
		return true;
	} else if (fq->depth == 2 && fq->incoltypes) {
		/* Ignore excess elements in "coltypes" */
		if (fq->current_col >= fq->cols)
			return true;
		if (!strncmp((char *)str, "string", len)) {
			fq->types[fq->current_col++] = OCRPT_RESULT_STRING;
			fq->coltypesset = true;
			return true;
		}
		if (!strncmp((char *)str, "number", len)) {
			fq->types[fq->current_col++] = OCRPT_RESULT_NUMBER;
			fq->coltypesset = true;
			return true;
		}
		if (!strncmp((char *)str, "datetime", len)) {
			fq->types[fq->current_col++] = OCRPT_RESULT_DATETIME;
			fq->coltypesset = true;
			return true;
		}
	} else if (ocrpt_yajl_set_field(fq, (char *)str, len))
		return true;

	return false;
}

static int ocrpt_yajl_map_key(void *ctx, const unsigned char *str, size_t len) {
	ocrpt_file_query *fq = ctx;

	if (fq->depth == 1 && !strncmp((char *)str, "columns", len)) {
		fq->inheader = true;
		return true;
	} else if (fq->depth == 1 && !strncmp((char *)str, "coltypes", len)) {
		fq->incoltypes = true;
		return true;
	} else if (fq->depth == 1 && !strncmp((char *)str, "rows", len)) {
		fq->inrows = true;
		return true;
	} else if (fq->depth == 3 && fq->inrows) {
		int32_t i;

		fq->current_col = -1;
		fq->inrowvalue = true;
		for (i = 0; i < fq->cols; i++) {
			if (!strncmp(fq->names[i], (char *)str, len)) {
				fq->current_col = i;
				break;
			}
		}
		return true;
	}

	return false;
}

static int ocrpt_yajl_start_map(void *ctx) {
	ocrpt_file_query *fq = ctx;
	fq->depth++;

	if (fq->depth == 1 && !fq->inheader && !fq->incoltypes && !fq->inrows && !fq->inrow)
		return true;
	if (fq->depth == 3 && fq->inrows && !fq->inrow) {
		fq->inrow = true;
		return true;
	}

	return false;
}

static int ocrpt_yajl_end_map(void *ctx) {
	ocrpt_file_query *fq = ctx;

	fq->depth--;

	if (fq->depth == 0 && !fq->inheader && !fq->incoltypes && !fq->inrows && !fq->inrow)
		return true;
	if (fq->depth == 1) {
		if (fq->inrows)
			fq->inrows = false;
	} if (fq->depth == 2 && fq->inrows && fq->inrow) {
		int32_t i;

		for (i = 0; i < fq->cols; i++) {
			fq->collist = ocrpt_list_end_append(fq->collist, &fq->col_last, fq->fields[i]);
			fq->fields[i] = NULL;
		}

		fq->rowlist = ocrpt_list_end_append(fq->rowlist, &fq->row_last, fq->collist);
		fq->collist = NULL;
		fq->col_last = NULL;
		fq->rows++;
		fq->inrow = false;
		return true;
	}

	return false;
}

static int ocrpt_yajl_start_array(void *ctx) {
	ocrpt_file_query *fq = ctx;
	fq->depth++;

	if (fq->depth == 2) {
		if (fq->inheader && !fq->incoltypes && !fq->inrows)
			return true;
		if (!fq->inheader && fq->incoltypes && !fq->inrows)
			return true;
		if (!fq->inheader && !fq->incoltypes && fq->inrows)
			return true;
	}

	return false;
}

static int ocrpt_yajl_end_array(void *ctx) {
	ocrpt_file_query *fq = ctx;
	int32_t i;

	fq->depth--;

	if (fq->depth == 1) {
		if (fq->inheader) {
			const ocrpt_list *ptr;

			fq->names = ocrpt_mem_malloc(fq->cols * sizeof(char *));
			fq->fields = ocrpt_mem_malloc(fq->cols * sizeof(char *));
			for (ptr = fq->collist, i = 0; ptr; ptr = ptr->next, i++) {
				fq->names[i] = ptr->data;
				fq->fields[i] = NULL;
			}

			fq->types = ocrpt_mem_malloc(fq->cols * sizeof(enum ocrpt_result_type));
			for (i = 0; i < fq->cols; i++)
				fq->types[i] = OCRPT_RESULT_STRING;

			fq->rowlist = ocrpt_list_end_append(fq->rowlist, &fq->row_last, fq->collist);
			fq->collist = NULL;
			fq->col_last = NULL;
			fq->rows++;

			fq->inheader = false;
			return true;
		}
		if (fq->incoltypes) {
			fq->incoltypes = false;
			return true;
		}
		if (fq->inrows) {
			fq->inrows = false;
			return true;
		}
		
	}

	return false;
}

static yajl_callbacks ocrpt_yajl_cb = {
	ocrpt_yajl_null,
	NULL, /* boolean */
	NULL, /* integer */
	NULL, /* double */
	ocrpt_yajl_number,
	ocrpt_yajl_string,
	ocrpt_yajl_start_map,
	ocrpt_yajl_map_key,
	ocrpt_yajl_end_map,
	ocrpt_yajl_start_array,
	ocrpt_yajl_end_array
};

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_json(opencreport *o, ocrpt_datasource *source,
												const char *name, const char *filename,
												const enum ocrpt_result_type *types) {
	struct stat st;
	char *buf;
	const char **array;
	ocrpt_query *retval;
	ocrpt_file_query fq;
	yajl_status yajl_stat;
	yajl_handle yhandler;
	ocrpt_list *rowptr;
	int32_t fd, row;

	if (!ocrpt_datasource_validate(o, source)) {
		fprintf(stderr, "%s:%d: datasource is not for this opencreport structure\n", __func__, __LINE__);
		return NULL;
	}

	if (source->input->type != OCRPT_INPUT_JSON) {
		fprintf(stderr, "%s:%d: datasource is not json\n", __func__, __LINE__);
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

	memset(&fq, 0, sizeof(ocrpt_file_query));

	yhandler = yajl_alloc(&ocrpt_yajl_cb, NULL, (void *) &fq);
	yajl_config(yhandler, yajl_allow_comments, 1);

	yajl_stat = yajl_parse(yhandler, (unsigned char *)buf, st.st_size);
	if (yajl_stat != yajl_status_ok) {
		ocrpt_file_query_free(&fq, true);
		yajl_free(yhandler);
		ocrpt_mem_free(buf);
		return NULL;
	}

	ocrpt_mem_free(buf);

	yajl_free(yhandler);

	array = ocrpt_mem_malloc(fq.rows * fq.cols * sizeof(char *));
	if (!array) {
		ocrpt_file_query_free(&fq, true);
		fprintf(stderr, "%s: out of memory\n", __func__);
		return NULL;
	}

	for (rowptr = fq.rowlist, row = 0; rowptr; rowptr = rowptr->next, row++) {
		const ocrpt_list *colptr;
		int32_t col;

		for (colptr = rowptr->data, col = 0; colptr; colptr = colptr->next, col++)
			array[row * fq.cols + col] = colptr->data;

		for (; col < fq.cols; col++)
			array[row * fq.cols + col] = NULL;
	}

	retval = array_query_add(o, source, name, array, fq.rows - 1, fq.cols, (fq.coltypesset ? fq.types : types), fq.coltypesset);

	ocrpt_file_query_free(&fq, false);
	if (fq.types && !fq.coltypesset)
		ocrpt_mem_free(fq.types);

	return retval;
}
