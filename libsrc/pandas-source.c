/*
 * OpenCReports spreadsheet based data sources
 *
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#include <config.h>

#include <string.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "datasource.h"

#if HAVE_LIBPYTHON

/* This must be after #include <datasource.h> */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
/* Python's header, unfortunately using a generic name. */
#include <datetime.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

static bool pandas_initialized = false;
static bool pandas_available = false;
static PyObject *main = NULL;
static PyObject *globals = NULL;
static PyObject *locals = NULL;
static PyObject *pandas = NULL;
static PyObject *pandas_script = NULL;
static PyObject *pandas_module = NULL;
static PyObject *pandas_excelfile_fn = NULL;
static PyObject *pandas_sheets_fn = NULL;
static PyObject *pandas_sheet_fn = NULL;
static PyObject *pandas_coltypes_fn = NULL;
static PyObject *pandas_nrows_fn = NULL;
static PyObject *pandas_row_fn = NULL;

static const char *pandas_script_str =
	"import pandas\n"
	"\n"
	"def ocrpt_excelfile(filename):\n"
	"    return pandas.ExcelFile(filename)\n"
	"\n"
	"def ocrpt_sheet_names(excelfile):\n"
	"    return excelfile.sheet_names\n"
	"\n"
	"def ocrpt_sheet(excelfile, sheet_name):\n"
	"    return excelfile.parse(sheet_name)\n"
	"\n"
	"def ocrpt_sheet_types(sheet):\n"
	"    return sheet.dtypes.to_dict()\n"
	"\n"
	"def ocrpt_sheet_nrows(sheet):\n"
	"    return len(sheet.values)\n"
	"\n"
	"def ocrpt_sheet_row(sheet, rowidx):\n"
	"    return sheet.values[rowidx].tolist()\n";

struct ocrpt_pandas_conn_private {
	PyObject *sheet_file;
};
typedef struct ocrpt_pandas_conn_private ocrpt_pandas_conn_private;

struct ocrpt_pandas_results {
	ocrpt_query_result *result;
	PyObject *sheet;
	PyObject *coltypes;
	Py_ssize_t rows;
	Py_ssize_t cols;
	Py_ssize_t row;
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
	if (!pandas_initialized || !pandas_available || !source || !params)
		return false;

	char *filename = NULL;

	for (int32_t i = 0; params[i].param_name; i++) {
		if (strcasecmp(params[i].param_name, "filename") == 0)
			filename = params[i].param_value;
	}

	if (!filename)
		return false;

	PyObject *args = PyTuple_New(1);

	PyObject *value = PyUnicode_FromString(filename);
	PyTuple_SetItem(args, 0, value);

	PyObject *sheet_file = PyObject_CallObject(pandas_excelfile_fn, args);

	Py_DecRef(args);

	if (!sheet_file || PyErr_Occurred()) {
		PyErr_Clear();
		return false;
	}

	ocrpt_pandas_conn_private *priv = ocrpt_mem_malloc(sizeof(ocrpt_pandas_conn_private));
	if (!priv) {
		Py_DecRef(sheet_file);
		return false;
	}

	priv->sheet_file = sheet_file;

	ocrpt_datasource_set_private(source, priv);

	return true;
}

static ocrpt_query *ocrpt_pandas_query_add(ocrpt_datasource *source,
										const char *name, const char *sheet_name,
										const int32_t *types UNUSED,
										int32_t types_cols UNUSED) {
	ocrpt_pandas_conn_private *priv = ocrpt_datasource_get_private(source);

	PyObject *args = PyTuple_New(1);

	/* Protect against reference stealing by PyTuple_SetItem */
	Py_IncRef(priv->sheet_file);
	PyTuple_SetItem(args, 0, priv->sheet_file);

	PyObject *sheets = PyObject_CallObject(pandas_sheets_fn, args);

	Py_DecRef(args);

	if (!sheets || PyErr_Occurred()) {
		PyErr_Clear();
		return NULL;
	}

	bool sheet_found = false;

	if (PyList_Check(sheets)) {
		Py_ssize_t sz = PyList_Size(sheets);
		Py_ssize_t i;

		for (i = 0; i < sz; i++) {
			/* Borrowed reference, don't call Py_DecRef() on it. */
			PyObject *item = PyList_GetItem(sheets, i);

			if (PyUnicode_Check(item)) {
				const char *item_utf8 = PyUnicode_AsUTF8(item);

				if (strcmp(sheet_name, item_utf8) == 0) {
					sheet_found = true;
					break;
				}
			}
		}
	}

	if (!sheet_found)
		return NULL;

	ocrpt_query *query = ocrpt_query_alloc(source, name);

	if (!query)
		return NULL;

	struct ocrpt_pandas_results *result = ocrpt_mem_malloc(sizeof(struct ocrpt_pandas_results));

	if (!result) {
		ocrpt_query_free(query);
		return NULL;
	}

	memset(result, 0, sizeof(struct ocrpt_pandas_results));

	args = PyTuple_New(2);

	/* Protect against reference stealing by PyTuple_SetItem */
	Py_IncRef(priv->sheet_file);
	PyTuple_SetItem(args, 0, priv->sheet_file);

	PyObject *value = PyUnicode_FromString(sheet_name);
	PyTuple_SetItem(args, 1, value);

	result->sheet = PyObject_CallObject(pandas_sheet_fn, args);

	Py_DecRef(args);

	if (!result->sheet || PyErr_Occurred()) {
		PyErr_Clear();
		ocrpt_mem_free(result);
		ocrpt_query_free(query);
		return NULL;
	}

	result->rows = 0;

	args = PyTuple_New(1);

	/* Protect against reference stealing by PyTuple_SetItem */
	Py_IncRef(result->sheet);
	PyTuple_SetItem(args, 0, result->sheet);

	PyObject *rows = PyObject_CallObject(pandas_nrows_fn, args);

	Py_DecRef(args);

	if (!rows || PyErr_Occurred()) {
		PyErr_Clear();
		Py_DecRef(result->sheet);
		ocrpt_mem_free(result);
		ocrpt_query_free(query);
		return NULL;
	}

	if (!PyNumber_Check(rows)) {
		PyErr_Clear();
		Py_DecRef(rows);
		Py_DecRef(result->sheet);
		ocrpt_mem_free(result);
		ocrpt_query_free(query);
		return NULL;
	}

	result->rows = PyNumber_AsSsize_t(rows, NULL);

	if (PyErr_Occurred()) {
		PyErr_Clear();
		Py_DecRef(rows);
		Py_DecRef(result->sheet);
		ocrpt_mem_free(result);
		ocrpt_query_free(query);
		return NULL;
	}

	Py_DecRef(rows);

	args = PyTuple_New(1);

	/* Protect against reference stealing by PyTuple_SetItem */
	Py_IncRef(result->sheet);
	PyTuple_SetItem(args, 0, result->sheet);

	result->coltypes = PyObject_CallObject(pandas_coltypes_fn, args);

	Py_DecRef(args);

	if (!result->coltypes || PyErr_Occurred()) {
		PyErr_Clear();
		Py_DecRef(result->sheet);
		ocrpt_mem_free(result);
		ocrpt_query_free(query);
		return NULL;
	}

	if (!PyDict_Check(result->coltypes)) {
		PyErr_Clear();
		Py_DecRef(result->coltypes);
		Py_DecRef(result->sheet);
		ocrpt_mem_free(result);
		ocrpt_query_free(query);
		return NULL;
	}

	result->cols = PyDict_Size(result->coltypes);

	/* There's no point in creating a query that has no columns */
	if (result->cols <= 0) {
		PyErr_Clear();
		Py_DecRef(result->coltypes);
		Py_DecRef(result->sheet);
		ocrpt_mem_free(result);
		ocrpt_query_free(query);
		return NULL;
	}

	result->row = -1;
	result->atstart = true;
	result->isdone = false;

	ocrpt_query_set_private(query, result);

	return query;
}

static void ocrpt_pandas_describe(ocrpt_query *query, ocrpt_query_result **qresult, int32_t *cols) {
	struct ocrpt_pandas_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	opencreport *o = ocrpt_datasource_get_opencreport(source);

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

		PyObject *key, *value;
		Py_ssize_t pos = 0, i;

		while (PyDict_Next(result->coltypes, &pos, &key, &value)) {
			enum ocrpt_result_type type = OCRPT_RESULT_STRING;

			i = pos - 1;
			assert(PyUnicode_Check(key));

			char *name = ocrpt_mem_strdup(PyUnicode_AsUTF8(key));

			for (int j = 0; j < OCRPT_EXPR_RESULTS; j++) {
				qr[j * result->cols + i].result.o = o;
				qr[j * result->cols + i].name = name;
				if (j == 0)
					qr[j * result->cols + i].name_allocated = true;

				if (PyDataType_ISNUMBER(value))
					type = OCRPT_RESULT_NUMBER;
				else if (PyDataType_ISDATETIME(value))
					type = OCRPT_RESULT_DATETIME;
				else if (PyDataType_ISOBJECT(value))
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
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	opencreport *o = ocrpt_datasource_get_opencreport(source);

	if (result->atstart || result->isdone) {
		ocrpt_query_result_set_values_null(query);
		return false;
	}

	PyObject *args = PyTuple_New(2);

	/* Protect against reference stealing by PyTuple_SetItem */
	Py_IncRef(result->sheet);
	PyTuple_SetItem(args, 0, result->sheet);

	PyObject *value = PyLong_FromLong(result->row);
	PyTuple_SetItem(args, 1, value);

	PyObject *row = PyObject_CallObject(pandas_row_fn, args);

	Py_DecRef(args);

	if (!row || PyErr_Occurred()) {
		PyErr_Clear();
		return false;
	}

	Py_ssize_t cols = PyList_Size(row);
	int32_t base = o->residx * result->cols;
	Py_ssize_t i;

	if (cols > result->cols)
		cols = result->cols;

	for (i = 0; i < cols; i++) {
		ocrpt_result *r = &result->result[base + i].result;
		PyObject *item = PyList_GetItem(row, i);

		r->isnull = false;

		if (PyUnicode_Check(item)) {
			const char *str = PyUnicode_AsUTF8(item);
			int32_t len = str ? strlen(str) : 0;

			r->type = OCRPT_RESULT_STRING;
			ocrpt_query_result_set_value(query, i, (str == NULL), (iconv_t)-1, str, len);
		} else if (PyFloat_Check(item)) {
			double val = PyFloat_AsDouble(item);

			/*
			 * When pandas finds an empty cell, it sometimes
			 * says it's a PyFloat with a NAN value.
			 * Use the original type and set NULL value in this case.
			 */
			if (isnan(val)) {
				r->type = r->orig_type;
				r->isnull = true;
			} else {
				r->isnull = false;
				r->type = OCRPT_RESULT_NUMBER;

				if (!r->number_initialized) {
					mpfr_init2(r->number, o->prec);
					r->number_initialized = true;
				}
				mpfr_set_d(r->number, val, o->rndmode);
			}
		} else if (PyLong_Check(item)) {
			long val = PyLong_AsLong(item);

			r->type = OCRPT_RESULT_NUMBER;

			if (!r->number_initialized) {
				mpfr_init2(r->number, o->prec);
				r->number_initialized = true;
			}

			mpfr_set_si(r->number, val, o->rndmode);
		} else if (PyDateTime_Check(item)) {
			int32_t y = PyDateTime_GET_YEAR(item);
			int32_t m = PyDateTime_GET_MONTH(item);
			int32_t d = PyDateTime_GET_DAY(item);
			int32_t h = PyDateTime_DATE_GET_HOUR(item);
			int32_t min = PyDateTime_DATE_GET_MINUTE(item);
			int32_t s = PyDateTime_DATE_GET_SECOND(item);

			/*
			 * If pandas detects an empty field with datetime type,
			 * the returned value is 0001-01-01 00:00:00.
			 * Use the original type and set NULL value in this case.
			 */
			if (y == 1 && m == 1 && d == 1 && h == 0 && min == 0 && s == 0) {
				r->type = r->orig_type;
				r->isnull = true;
			} else {
				r->type = OCRPT_RESULT_DATETIME;

				r->datetime.tm_year = y - 1900;
				r->datetime.tm_mon = m - 1;
				r->datetime.tm_mday = d;
				r->datetime.tm_hour = h;
				r->datetime.tm_min = min;
				r->datetime.tm_sec = s;
				r->datetime.tm_isdst = -1;
				r->date_valid = true;
				r->time_valid = true;
			}
		} else if (PyDate_Check(item)) {
			int32_t y = PyDateTime_GET_YEAR(item);
			int32_t m = PyDateTime_GET_MONTH(item);
			int32_t d = PyDateTime_GET_DAY(item);

			/*
			 * Try to do the same as above for a complete datetime,
			 * but only with date parts.
			 */
			if (y == 1 && m == 1 && d == 1) {
				r->type = r->orig_type;
				r->isnull = true;
			} else {
				r->type = OCRPT_RESULT_DATETIME;

				r->datetime.tm_year = y - 1900;
				r->datetime.tm_mon = m - 1;
				r->datetime.tm_mday = d;
				r->datetime.tm_hour = 0;
				r->datetime.tm_min = 0;
				r->datetime.tm_sec = 0;
				r->datetime.tm_isdst = -1;
				r->date_valid = true;
				r->time_valid = false;
			}
		} else if (PyTime_Check(item)) {
			r->type = OCRPT_RESULT_DATETIME;

			r->datetime.tm_year = 0;
			r->datetime.tm_mon = 0;
			r->datetime.tm_mday = 0;
			r->datetime.tm_hour = PyDateTime_TIME_GET_HOUR(item);
			r->datetime.tm_min = PyDateTime_TIME_GET_MINUTE(item);
			r->datetime.tm_sec = PyDateTime_TIME_GET_SECOND(item);
			r->datetime.tm_isdst = 0;
			r->date_valid = false;
			r->time_valid = true;
		}
	}

	for (; i < result->cols; i++) {
		ocrpt_result *r = &result->result[base + i].result;

		r->type = r->orig_type;
		r->isnull = true;
	}

	return true;
}

static void ocrpt_pandas_rewind(ocrpt_query *query) {
	ocrpt_pandas_results *result = ocrpt_query_get_private(query);

	result->row = -1;
	result->atstart = true;
	result->isdone = false;
}

static bool ocrpt_pandas_next(ocrpt_query *query) {
	ocrpt_pandas_results *result = ocrpt_query_get_private(query);

	if (result == NULL)
		return false;

	result->row++;
	result->atstart = false;
	result->isdone = (result->row >= result->rows);

	return ocrpt_pandas_populate_result(query);
}

static bool ocrpt_pandas_isdone(ocrpt_query *query) {
	ocrpt_pandas_results *result = ocrpt_query_get_private(query);

	if (result == NULL)
		return true;

	return result->isdone;
}

static void ocrpt_pandas_free(ocrpt_query *query) {
	ocrpt_pandas_results *result = ocrpt_query_get_private(query);

	ocrpt_query_set_private(query, NULL);

	Py_DecRef(result->coltypes);
	Py_DecRef(result->sheet);
	ocrpt_mem_free(result);
}

static void ocrpt_pandas_close(const ocrpt_datasource *source) {
	ocrpt_pandas_conn_private *priv = ocrpt_datasource_get_private(source);

	Py_DecRef(priv->sheet_file);
	ocrpt_mem_free(priv);
	ocrpt_datasource_set_private((ocrpt_datasource *)source, NULL);
}

static const char *ocrpt_pandas_input_names[] = {
	"pandas", "spreadsheet", NULL
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

DLL_EXPORT_SYM bool ocrpt_pandas_initialize(void) {
	if (pandas_initialized)
		return pandas_available;

	Py_InitializeEx(0);

	PyObject *main = PyImport_AddModule("__main__");
	PyObject *globals = PyModule_GetDict(main);
	PyObject *locals = PyDict_New();
	PyObject *pandas = PyImport_ImportModuleEx("pandas", globals, locals, NULL);

	pandas_available = (pandas && !PyErr_Occurred());

	if (!pandas_available) {
		PyErr_Clear();
		pandas_initialized = true;
		return pandas_available;
	}

	PyDateTime_IMPORT;

	pandas_script = Py_CompileString(pandas_script_str, "", Py_file_input);
	pandas_available = (pandas_script && !PyErr_Occurred());

	if (!pandas_available) {
		PyErr_Clear();
		pandas_initialized = true;
		return pandas_available;
	}

	pandas_module = PyImport_ExecCodeModule("__opencreport_datasource__", pandas_script);
	pandas_available = (pandas_module && !PyErr_Occurred());

	if (!pandas_available) {
		PyErr_Clear();
		Py_DecRef(pandas_script);
		pandas_initialized = true;
		return pandas_available;
	}

	/*
	 * No need to check that these with
	 *   if (..._fn != NULL && PyObject_GetAttrString(pandas_module, .._fn))
	 * because these are what the script text contains as Python functions.
	 */
	pandas_excelfile_fn = PyObject_GetAttrString(pandas_module, "ocrpt_excelfile");
	pandas_sheets_fn = PyObject_GetAttrString(pandas_module, "ocrpt_sheet_names");
	pandas_sheet_fn = PyObject_GetAttrString(pandas_module, "ocrpt_sheet");
	pandas_coltypes_fn = PyObject_GetAttrString(pandas_module, "ocrpt_sheet_types");
	pandas_nrows_fn = PyObject_GetAttrString(pandas_module, "ocrpt_sheet_nrows");
	pandas_row_fn = PyObject_GetAttrString(pandas_module, "ocrpt_sheet_row");

	pandas_initialized = true;
	return pandas_available;
}

DLL_EXPORT_SYM void ocrpt_pandas_deinitialize(void) {
	if (!pandas_initialized || !pandas_available)
		return;

	Py_DecRef(pandas_row_fn);
	Py_DecRef(pandas_nrows_fn);
	Py_DecRef(pandas_coltypes_fn);
	Py_DecRef(pandas_sheet_fn);
	Py_DecRef(pandas_sheets_fn);
	Py_DecRef(pandas_excelfile_fn);
	Py_DecRef(pandas_module);
	Py_DecRef(pandas_script);
	Py_DecRef(pandas);
	Py_DecRef(locals);
	Py_DecRef(globals);
	Py_DecRef(main);

	Py_FinalizeEx();

	pandas_available = false;
	pandas_initialized = false;
}

#endif /* HAVE_LIBPYTHON */
