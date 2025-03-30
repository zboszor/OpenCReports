/*
 * OpenCReports PHP module array datasource and misc initialization
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

#if PHP_VERSION_ID >= 70000
static zval *php_opencreport_get_zval_direct(zval *zv) {
	if (zv == NULL)
		return NULL;

	/*
	 * This loop is not infinite.
	 * The maze of indirection and references may be deep,
	 * but not endless.
	 */
	for (int type = Z_TYPE_P(zv); type == IS_INDIRECT || type == IS_REFERENCE; type = Z_TYPE_P(zv)) {
		switch (type) {
		case IS_INDIRECT:
			zv = Z_INDIRECT_P(zv);
			type = Z_TYPE_P(zv);
			break;
		case IS_REFERENCE:
			zv = Z_REFVAL_P(zv);
			type = Z_TYPE_P(zv);
			break;
		default:
			break;
		}
	}

	return zv;
}
#else
#define php_opencreport_get_zval_direct(zv) (zv)
#endif

static zval *php_opencreport_get_zval_of_type_raw(zval *zv, int expected_type) {
	zval *zv1 = php_opencreport_get_zval_direct(zv);

	return (zv1 && (Z_TYPE_P(zv1) == expected_type)) ? zv1 : NULL;
}

static zval *php_opencreport_get_zval_of_type(const char *name, int expected_type) {
	if (!name || !*name)
		return NULL;

	zval *zv = ocrpt_hash_str_find(&EG(symbol_table), name, strlen(name));

	return php_opencreport_get_zval_of_type_raw(zv, expected_type);
}

static ocrpt_result *php_opencreport_env_query(opencreport *o, const char *env) {
	if (!env)
		return NULL;

	ocrpt_result *result = ocrpt_result_new(o);
	if (!result)
		return NULL;

	bool found = false;
	zval *var = php_opencreport_get_zval_direct(ocrpt_hash_str_find(&EG(symbol_table), env, strlen(env)));
	if (var) {
		found = true;
		if (Z_TYPE_P(var) == IS_STRING)
			ocrpt_result_set_string(result, Z_STRVAL_P(var));
		else if (Z_TYPE_P(var) == IS_LONG)
			ocrpt_result_set_long(result, Z_LVAL_P(var));
		else if (Z_TYPE_P(var) == IS_DOUBLE)
			ocrpt_result_set_double(result, Z_DVAL_P(var));
		else if (Z_TYPE_P(var) == IS_NULL)
			ocrpt_result_set_string(result, NULL);
		else
			found = false;
	}

	if (!found) {
		char *value = getenv(env);
		ocrpt_result_set_string(result, value);
	}

	return result;
}

static void php_opencreport_query_discover_array(const char *arrayname, void **array, int32_t *rows, int32_t *cols, const char *typesname, void **types, int32_t *types_cols, bool *free_types) {
	/* The PHP module does not deal with backing a PHP array with a C array */
	if (array)
		*array = NULL;
	if (rows)
		*rows = 0;
	if (cols)
		*cols = 0;

	if (!typesname || !*typesname || !types)
		goto out_error;

	zval *zv_types = php_opencreport_get_zval_of_type(typesname, IS_ARRAY);
	if (!zv_types)
		goto out_error;

	HashTable *htab1 = Z_ARRVAL_P(zv_types);
	HashPosition htab1p;

	int32_t t_cols = zend_hash_num_elements(htab1);
	if (t_cols <= 0)
		goto out_error;

	int32_t sz = t_cols * sizeof(int32_t);
	*types = ocrpt_mem_malloc(sz);
	if (!*types)
		goto out_error;

	memset(*types, 0, sz);

	int32_t col;
	for (col = 0, zend_hash_internal_pointer_reset_ex(htab1, &htab1p); col < t_cols; col++, zend_hash_move_forward_ex(htab1, &htab1p)) {
		zval *cell = ocrpt_hash_get_current_data_ex(htab1, &htab1p);
		int32_t data_result;

		if (Z_TYPE_P(cell) == IS_LONG) {
#if PHP_VERSION_ID >= 70000
			zend_long l = Z_LVAL_P(cell);
#else
			long l = Z_LVAL_P(cell);
#endif
			if (l >= OCRPT_RESULT_STRING && l <= OCRPT_RESULT_ERROR)
				data_result = l;
			else
				data_result = OCRPT_RESULT_STRING;
		} else if (Z_TYPE_P(cell) == IS_STRING) {
			char *s = Z_STRVAL_P(cell);

			if (strcasecmp(s, "number") == 0 || strcasecmp(s, "numeric") == 0)
				data_result = OCRPT_RESULT_NUMBER;
			else if (strcasecmp(s, "datetime") == 0)
				data_result = OCRPT_RESULT_DATETIME;
			else if (strcasecmp(s, "error") == 0)
				data_result = OCRPT_RESULT_ERROR;
			else /* if (strcasecmp(s, "string") == 0) */
				data_result = OCRPT_RESULT_STRING;
		} else {
			/* Handle error? */
			data_result = OCRPT_RESULT_STRING;
		}

		((int32_t *)(*types))[col] = data_result;
	}

	if (types_cols)
		*types_cols = t_cols;
	if (free_types)
		*free_types = true;
	return;

	out_error:

	if (types)
		*types = NULL;
	if (types_cols)
		*types_cols = 0;
	if (free_types)
		*free_types = false;
}

static int php_opencreport_std_printf(const char *fmt, ...) {
	ocrpt_string *s;
	va_list va;
	int len;

	va_start(va, fmt);
	len = vsnprintf(NULL, 0, fmt, va);
	va_end(va);

	if (len <= 0)
		return len;

	va_start(va, fmt);
	s = ocrpt_mem_string_new_vnprintf(len, fmt, va);
	va_end(va);

	php_printf("%s", s->str);

	ocrpt_mem_string_free(s, true);

	return len;
}

static int php_opencreport_err_printf(const char *fmt, ...) {
	ocrpt_string *s;
	va_list va;
	int len;

	va_start(va, fmt);
	len = vsnprintf(NULL, 0, fmt, va);
	va_end(va);

	if (len <= 0)
		return len;

	va_start(va, fmt);
	s = ocrpt_mem_string_new_vnprintf(len, fmt, va);
	va_end(va);

	php_error_docref(NULL, E_WARNING, "%s", s->str);

	ocrpt_mem_string_free(s, true);

	return len;
}

static void *opencreport_emalloc(size_t size) {
	return emalloc(size);
}

static void *opencreport_erealloc(void *ptr, size_t size) {
	return erealloc(ptr, size);
}

static void opencreport_efree(const void *ptr) {
	return efree((void *)ptr);
}

static char *opencreport_estrdup(const char *s) {
	return estrdup(s);
}

static char *opencreport_estrndup(const char *s, size_t size) {
	return estrndup(s, size);
}

/* PHP array datasource methods */
static const char *php_opencreport_array_input_names[] = { "array", NULL };

struct php_opencreport_array_results {
	char *array_name;
	char *types_name;
	ocrpt_query_result *result;
	ocrpt_string *converted;
#if opencreports_use_static_array
	char **data;
	int32_t rows;
	int32_t current_row;
#else
	HashTable *ahash;
	HashPosition ahashpos;
#endif
	int32_t cols;
	bool atstart:1;
	bool isdone:1;
};

static bool php_opencreport_array_connect(ocrpt_datasource *ds, const ocrpt_input_connect_parameter *conn_params) {
	ocrpt_datasource_set_private(ds, (iconv_t)-1);
	return true;
}

#if opencreports_use_static_array
static char **php_opencreport_array_set_backing_array(HashTable *ahash, int32_t a_cols, int32_t *ret_rows) {
	int32_t a_rows, array_rows = zend_hash_num_elements(ahash);
	int32_t sz = array_rows * a_cols * sizeof(char *);
	char **data = ocrpt_mem_malloc(sz);

	if (!data)
		return NULL;

	memset(data, 0, sz);

	HashPosition ahashpos;

	for (a_rows = 0, zend_hash_internal_pointer_reset_ex(ahash, &ahashpos); a_rows < array_rows; a_rows++, zend_hash_move_forward_ex(ahash, &ahashpos)) {
		zval *row = ocrpt_hash_get_current_data_ex(ahash, &ahashpos);
		row = php_opencreport_get_zval_of_type_raw(row, IS_ARRAY);

		/* If row is not an array, use an all-NULL row instead. */
		if (!row)
			continue;

		HashTable *rowhash = Z_ARRVAL_P(row);
		HashPosition rowhashpos;
		int32_t i;

		for (i = 0, zend_hash_internal_pointer_reset_ex(rowhash, &rowhashpos); i < a_cols; i++, zend_hash_move_forward_ex(rowhash, &rowhashpos)) {
			zval *row = ocrpt_hash_get_current_data_ex(ahash, &ahashpos);
			row = php_opencreport_get_zval_of_type_raw(row, IS_ARRAY);

			zval *cell = ocrpt_hash_get_current_data_ex(rowhash, &rowhashpos);
			cell = php_opencreport_get_zval_direct(cell);

			char *str = NULL, *newstr = NULL;
			int32_t len = 0;
			zval copy;

			if (cell && Z_TYPE_P(cell) != IS_NULL) {
#if PHP_VERSION_ID >= 70000
				ZVAL_STR(&copy, _zval_get_string_func(cell));
#else
				ZVAL_COPY_VALUE(&copy, cell);
				zval_copy_ctor(&copy);
				convert_to_string(&copy);
#endif
				str = Z_STRVAL(copy);
				len = Z_STRLEN(copy);
			}

			if (len > 0) {
				newstr = ocrpt_mem_malloc(len + 1);

				memcpy(newstr, str, len);
				newstr[len] = 0;
			}

#if PHP_VERSION_ID < 70000
			if (cell && Z_TYPE_P(cell) != IS_NULL) {
				zval_dtor(&copy);
			}
#endif

			data[a_rows * a_cols + i] = newstr;
		}
	}

	if (ret_rows)
		*ret_rows = array_rows - 1;

	return data;
}
#endif

static ocrpt_query *php_opencreport_array_query_add_symbolic(ocrpt_datasource *source,
										const char *name,
										const char *array_name, int32_t rows, int32_t cols,
										const char *types_name, int32_t types_cols) {
	if (!source || !name || !array_name)
		return NULL;

	zval *array = (array_name && *array_name) ? php_opencreport_get_zval_of_type(array_name, IS_ARRAY) : NULL;

	if (!array)
		return NULL;

	HashTable *ahash = Z_ARRVAL_P(array);
	HashPosition ahashpos;

	zend_hash_internal_pointer_reset_ex(ahash, &ahashpos);
	zval *row = ocrpt_hash_get_current_data_ex(ahash, &ahashpos);
	row = php_opencreport_get_zval_of_type_raw(row, IS_ARRAY);

	if (!row)
		return NULL;

	HashTable *rowhash = Z_ARRVAL_P(row);
	HashPosition rowhashpos;
	int32_t a_cols = 0, array_cols = zend_hash_num_elements(rowhash);
	zval *cell = NULL;

	zend_hash_internal_pointer_reset_ex(rowhash, &rowhashpos);
	for (a_cols = 0; a_cols < array_cols; a_cols++, zend_hash_move_forward_ex(rowhash, &rowhashpos)) {
		cell = ocrpt_hash_get_current_data_ex(rowhash, &rowhashpos);
		cell = php_opencreport_get_zval_of_type_raw(cell, IS_STRING);

		if (cell == NULL)
			return NULL;
	}

	if (!a_cols)
		return NULL;

	ocrpt_query *query = ocrpt_query_alloc(source, name);
	if (!query)
		return NULL;

	struct php_opencreport_array_results *result = ocrpt_mem_malloc(sizeof(struct php_opencreport_array_results));
	if (!result) {
		ocrpt_query_free(query);
		return NULL;
	}

#if opencreports_use_static_array
	int32_t array_rows = 0;
	char **data = php_opencreport_array_set_backing_array(ahash, a_cols, &array_rows);

	if (!data) {
		ocrpt_query_free(query);
		return NULL;
	}
#endif

	memset(result, 0, sizeof(struct php_opencreport_array_results));
	result->array_name = ocrpt_mem_strdup(array_name);
	result->types_name = ocrpt_mem_strdup(types_name);
#if opencreports_use_static_array
	result->data = data;
	result->rows = array_rows;
	result->current_row = 0;
#else
	result->ahash = ahash;
	zend_hash_internal_pointer_reset_ex(result->ahash, &result->ahashpos);
#endif
	result->cols = a_cols;
	result->atstart = true;
	result->isdone = false;
	ocrpt_query_set_private(query, result);

	return query;
}

static void php_opencreport_array_describe(ocrpt_query *query, ocrpt_query_result **qresult, int32_t *cols) {
	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	opencreport *o = ocrpt_datasource_get_opencreport(source);

	if (!result->result) {
		ocrpt_query_result *qr = ocrpt_mem_malloc(OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));
		if (!qr)
			goto out_error;

		memset(qr, 0, OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));

		zval *array = php_opencreport_get_zval_of_type(result->array_name, IS_ARRAY);
		if (!array)
			goto out_error;

		HashTable *ahash = Z_ARRVAL_P(array);
		HashPosition ahashpos;

		zend_hash_internal_pointer_reset_ex(ahash, &ahashpos);
		zval *row = ocrpt_hash_get_current_data_ex(ahash, &ahashpos);
		row = php_opencreport_get_zval_of_type_raw(row, IS_ARRAY);

		if (!row)
			goto out_error;

		HashTable *rowhash = Z_ARRVAL_P(row);
		HashPosition rowhashpos;
		int32_t i;

		if (result->cols != zend_hash_num_elements(rowhash))
			goto out_error;

		for (i = 0, zend_hash_internal_pointer_reset_ex(rowhash, &rowhashpos); i < result->cols; i++, zend_hash_move_forward_ex(rowhash, &rowhashpos)) {
			zval *cell = ocrpt_hash_get_current_data_ex(rowhash, &rowhashpos);
			cell = php_opencreport_get_zval_direct(cell);

			char *data_result;
			zval copy;

#if PHP_VERSION_ID >= 70000
			ZVAL_STR(&copy, _zval_get_string_func(cell));
#else
			ZVAL_COPY_VALUE(&copy, cell);
			zval_copy_ctor(&copy);
			convert_to_string(&copy);
#endif
			data_result = Z_STRVAL(copy);

			for (int32_t j = 0; j < OCRPT_EXPR_RESULTS; j++) {
				qr[j * result->cols + i].result.o = o;
				qr[j * result->cols + i].name = (j == 0) ? ocrpt_mem_strdup(data_result) : qr[i].name;
				qr[j * result->cols + i].name_allocated = (j == 0);
			}

#if PHP_VERSION_ID < 70000
			zval_dtor(&copy);
#endif
		}

		i = 0;

		zval *types = (result->types_name && *result->types_name) ? php_opencreport_get_zval_of_type(result->types_name, IS_ARRAY) : NULL;
		if (types) {
			HashTable *typeshash = Z_ARRVAL_P(types);
			HashPosition typeshashpos;
			int types_cols = zend_hash_num_elements(typeshash);

			for (zend_hash_internal_pointer_reset_ex(typeshash, &typeshashpos); i < types_cols && i < result->cols; i++, zend_hash_move_forward_ex(typeshash, &typeshashpos)) {
				zval *cell = ocrpt_hash_get_current_data_ex(typeshash, &typeshashpos);
				cell = php_opencreport_get_zval_of_type_raw(cell, IS_LONG);

				enum ocrpt_result_type type = OCRPT_RESULT_ERROR;
				if (cell) {
#if PHP_VERSION_ID >= 70000
					zend_long l = Z_LVAL_P(cell);
#else
					long l = Z_LVAL_P(cell);
#endif

					if (l >= OCRPT_RESULT_STRING && l <= OCRPT_RESULT_ERROR)
						type = l;
					else
						type = OCRPT_RESULT_STRING;
				} else
					type = OCRPT_RESULT_STRING;

				for (int j = 0; j < OCRPT_EXPR_RESULTS; j++) {

					qr[j * result->cols + i].result.type = type;
					qr[j * result->cols + i].result.orig_type = type;

					if (qr[i].result.type == OCRPT_RESULT_NUMBER) {
						mpfr_init2(qr[j * result->cols + i].result.number, ocrpt_get_numeric_precision_bits(o));
						qr[j * result->cols + i].result.number_initialized = true;
					}

					qr[j * result->cols + i].result.isnull = true;
				}
			}
		}

		for (; i < result->cols; i++) {
			for (int j = 0; j < OCRPT_EXPR_RESULTS; j++) {
				qr[j * result->cols + i].result.type = OCRPT_RESULT_STRING;
				qr[j * result->cols + i].result.orig_type = OCRPT_RESULT_STRING;
				qr[j * result->cols + i].result.isnull = true;
			}
		}

		result->result = qr;
	}

	if (qresult)
		*qresult = result->result;
	if (cols)
		*cols = result->cols;

	return;

	out_error:

	if (qresult)
		*qresult = NULL;
	if (cols)
		*cols = 0;
}

static bool php_opencreport_array_refresh(ocrpt_query *query) {
	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);
	zval *array = php_opencreport_get_zval_of_type(result->array_name, IS_ARRAY);

	if (!array) {
		zend_throw_error(NULL, "php_opencreport_array_refresh: array \"%s\" not found", result->array_name);
		return false;
	}

#if opencreports_use_static_array
	HashTable *ahash = Z_ARRVAL_P(array);
	int32_t array_rows = 0;
	char **data = php_opencreport_array_set_backing_array(ahash, result->cols, &array_rows);

	if (!data)
		return false;

	int32_t max = (result->rows + 1) * result->cols;
	for (int32_t i = 0; i < max; i++)
		ocrpt_mem_free(result->data[i]);
	ocrpt_mem_free(result->data);

	result->data = data;
	result->rows = array_rows;
	result->current_row = 0;
#else
	result->ahash = Z_ARRVAL_P(array);
	zend_hash_internal_pointer_reset_ex(result->ahash, &result->ahashpos);
#endif

	return true;
}

static void php_opencreport_array_rewind(ocrpt_query *query) {
	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);

	if (result == NULL)
		return;

#if opencreports_use_static_array
	result->current_row = 0;
#else
	zval *array = php_opencreport_get_zval_of_type(result->array_name, IS_ARRAY);
	result->ahash = Z_ARRVAL_P(array);
	zend_hash_internal_pointer_reset_ex(result->ahash, &result->ahashpos);
#endif

	result->atstart = true;
	result->isdone = false;
}

static bool php_opencreport_array_populate_result(ocrpt_query *query) {
	struct ocrpt_datasource *source = ocrpt_query_get_source(query);
	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);

	if (result->atstart || result->isdone) {
		ocrpt_query_result_set_values_null(query);
		return !result->isdone;
	}

#if opencreports_use_static_array
	for (int32_t i = 0; i < result->cols; i++) {
		int32_t dataidx = result->current_row * result->cols + i;
		const char *str = result->data[dataidx];
		int32_t len = str ? strlen(str) : 0;

		ocrpt_query_result_set_value(query, i, (str == NULL), ocrpt_datasource_get_private(source), str, len);
	}
#else
	zval *row = ocrpt_hash_get_current_data_ex(result->ahash, &result->ahashpos);
	row = php_opencreport_get_zval_of_type_raw(row, IS_ARRAY);

	HashTable *rowhash = Z_ARRVAL_P(row);
	HashPosition rowhashpos;
	int32_t i;

	for (i = 0, zend_hash_internal_pointer_reset_ex(rowhash, &rowhashpos); i < result->cols; i++, zend_hash_move_forward_ex(rowhash, &rowhashpos)) {
		zval *cell = ocrpt_hash_get_current_data_ex(rowhash, &rowhashpos);
		cell = php_opencreport_get_zval_direct(cell);

		char *str = NULL;
		int32_t len = 0;

		if (cell && Z_TYPE_P(cell) != IS_NULL) {
			zval copy;

			ZVAL_STR(&copy, _zval_get_string_func(cell));
			str = Z_STRVAL(copy);
			len = Z_STRLEN(copy);
		}

		ocrpt_query_result_set_value(query, i, (str == NULL), ocrpt_datasource_get_private(source), str, len);
	}
#endif

	return true;
}

static bool php_opencreport_array_next(ocrpt_query *query) {
	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);

	if (result == NULL)
		return false;

	result->atstart = false;
#if opencreports_use_static_array
	result->current_row++;
	result->isdone = (result->current_row > result->rows);
#else
	zend_result newrow = zend_hash_move_forward_ex(result->ahash, &result->ahashpos);
	result->isdone = (newrow != SUCCESS || result->ahashpos >= zend_hash_num_elements(result->ahash));
#endif

	return php_opencreport_array_populate_result(query);
}

static bool php_opencreport_array_isdone(ocrpt_query *query) {
	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);

	if (result == NULL)
		return true;

	return result->isdone;
}

static void php_opencreport_array_free(ocrpt_query *query) {
	if (!query)
		return;

	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);

#if opencreports_use_static_array
	int32_t max = (result->rows + 1) * result->cols;
	for (int32_t i = 0; i < max; i++)
		ocrpt_mem_free(result->data[i]);
	ocrpt_mem_free(result->data);
#endif
	ocrpt_mem_free(result->array_name);
	ocrpt_mem_free(result->types_name);
	ocrpt_mem_free(result);
	ocrpt_query_set_private(query, NULL);
}

static bool php_opencreport_array_set_encoding(ocrpt_datasource *ds, const char *encoding) {
	iconv_t c = ocrpt_datasource_get_private(ds);

	if (c != (iconv_t)-1)
		iconv_close(c);

	c = iconv_open("UTF-8", encoding);
	ocrpt_datasource_set_private(ds, c);

	return (c != (iconv_t)-1);
}

void php_opencreport_array_close(const ocrpt_datasource *ds) {
	iconv_t c = ocrpt_datasource_get_private((ocrpt_datasource *)ds);

	if (c != (iconv_t)-1)
		iconv_close(c);
}

static const ocrpt_input php_opencreport_array_input = {
	.names = php_opencreport_array_input_names,
	.connect = php_opencreport_array_connect,
	.query_add_symbolic_data = php_opencreport_array_query_add_symbolic,
	.describe = php_opencreport_array_describe,
	.refresh = php_opencreport_array_refresh,
	.rewind = php_opencreport_array_rewind,
	.next = php_opencreport_array_next,
	.populate_result = php_opencreport_array_populate_result,
	.isdone = php_opencreport_array_isdone,
	.free = php_opencreport_array_free,
	.set_encoding = php_opencreport_array_set_encoding,
	.close = php_opencreport_array_close
};
/* End of PHP array datasource methods */

static char dummy_report_xml[] =
	"<?xml version=\"1.0\"?>"
	"<!DOCTYPE report>"
	"<Report><Detail><FieldDetails><Output><Line>"
	"<field value=\"x\"/>"
	"</Line></Output></FieldDetails></Detail></Report>";

#define DUMMY_REPORT_ROWS 1
#define DUMMY_REPORT_COLS 1

const char *dummy_report_array[DUMMY_REPORT_ROWS + 1][DUMMY_REPORT_COLS] = {
	{ "x" }, { "1" }
};

bool opencreport_init(void) {
	/*
	 * Perform a dummy report run so the module doesn't crash
	 * when unloaded too quickly.
	 */
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add(o, "array", "array", NULL);

	ocrpt_query_add_data(ds, "data", (const char **)dummy_report_array, DUMMY_REPORT_ROWS, DUMMY_REPORT_COLS, NULL, 0);
	ocrpt_parse_xml_from_buffer(o, dummy_report_xml, sizeof(dummy_report_xml) - 1);
	ocrpt_set_output_format(o, OCRPT_OUTPUT_PDF);
	ocrpt_execute(o);
	ocrpt_free(o);
	/*
	 * End of workaround
	 */

	/*
	 * Register our array datasource input driver,
	 * overriding the C array driver.
	 */
	if (!ocrpt_input_register(&php_opencreport_array_input))
		return false;

	ocrpt_query_set_discover_func(php_opencreport_query_discover_array);
	ocrpt_env_set_query_func(php_opencreport_env_query);
	ocrpt_set_printf_func(php_opencreport_std_printf);
	ocrpt_set_err_printf_func(php_opencreport_err_printf);
	ocrpt_mem_set_alloc_funcs(opencreport_emalloc, opencreport_erealloc, NULL, opencreport_efree, opencreport_estrdup, opencreport_estrndup);

	return true;
}
