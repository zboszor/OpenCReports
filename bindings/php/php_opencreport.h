/*
 * OpenCReports PHP module header
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#ifndef PHP_OPENCREPORT_H
#define PHP_OPENCREPORT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "php.h"
#include "php_version.h"
#if PHP_VERSION_ID < 70000
#include "zend_exceptions.h"
#endif
#include "ext/standard/info.h"
#include "zend_interfaces.h"
#include <errno.h>
#include <stdio.h>

#include <opencreport.h>

/*
 * With PHP 7.2 or older, static array backing is needed.
 * With PHP 7.3 or newer, --with-opencreports-static-array
 * can switch back to this older method.
 */
#if PHP_VERSION_ID >= 70300
#include "include/php_opencreports_use_static_array.h"
#else
#define opencreports_use_static_array 1
#endif

/*
 * PHP compatibility wrappers
 */

/* Introduced in PHP 5.5 */

#if PHP_VERSION_ID < 50500
#if ZEND_DEBUG
#define ZEND_ASSERT(c) assert(c)
#else
#define ZEND_ASSERT(c)
#endif
#endif

/* Introduced in PHP 7.0 */

#if PHP_VERSION_ID < 70000
#define zend_throw_error(unused, format, ...) zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, format, ## __VA_ARGS__)

#define ZSTR_VAL(s) (s)
#define ZSTR_LEN(s) (s ## _len)

#define OCRPT_ZVAL_STRINGL(zv, str, len) ZVAL_STRINGL(zv, str, len, 1)
#define OCRPT_RETVAL_STRINGL(str, len) RETVAL_STRINGL(str, len, 1)
#define OCRPT_RETVAL_STRING(str) RETVAL_STRING(str, 1)
#define OCRPT_RETURN_STRINGL(str, len) RETURN_STRINGL(str, len, 1)
#define OCRPT_RETURN_STRING(str) RETURN_STRING(str, 1)

#else

#define OCRPT_ZVAL_STRINGL(zv, str, len) ZVAL_STRINGL(zv, str, len)
#define OCRPT_RETVAL_STRINGL(str, len) RETVAL_STRINGL(str, len)
#define OCRPT_RETVAL_STRING(str) RETVAL_STRING(str)
#define OCRPT_RETURN_STRINGL(str, len) RETURN_STRINGL(str, len)
#define OCRPT_RETURN_STRING(str) RETURN_STRING(str)

#endif

/* Introduced in PHP 7.1 */

#if PHP_VERSION_ID < 70100
#define IS_VOID (0)
#endif

/* Add common wrappers to hide the differences between php5, and different incarnations of phpng. */

#if PHP_VERSION_ID >= 70000

#define ocrpt_hash_get_current_data_ex(hash, hashposptr) zend_hash_get_current_data_ex(hash, hashposptr)
#define ocrpt_hash_str_find(hash, key, len) zend_hash_str_find(hash, key, len)

#else

static inline zval *ocrpt_hash_get_current_data_ex(HashTable *hash, HashPosition *pos) {
	void *data;

	zend_hash_get_current_data_ex(hash, &data, pos);
	return data ? *(zval **)data : NULL;
}

static inline zval *ocrpt_hash_str_find(const HashTable *hash, const char *key, size_t len) {
	void *data;

	if (zend_hash_find(hash, key, len + 1, (void**)&data) == SUCCESS)
		return *(zval **)data;

	return NULL;
}

#endif

#if PHP_VERSION_ID < 70000

/* For PHP 5.6 and older, arginfo is not needed. */

#elif PHP_VERSION_ID < 70200

#define OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null) \
		ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, NULL, allow_null)

#define OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(name, return_reference, required_num_args, class_name, allow_null) \
		ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, 0, #class_name, allow_null)

#define ZEND_ARG_VARIADIC_TYPE_INFO(pass_by_ref, name, type_hint, allow_null) { #name, NULL, type_hint, pass_by_ref, allow_null, 1 },
#define ZEND_ARG_VARIADIC_OBJ_INFO(pass_by_ref, name, classname, allow_null)  { #name, #classname, IS_OBJECT, pass_by_ref, allow_null, 1 },

static zend_always_inline zend_string *zend_string_init_interned(const char *str, size_t len, int persistent)
{
	zend_string *ret = zend_string_init(str, len, persistent);

	return zend_new_interned_string(ret);
}
#else
#define OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null) \
		ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null)

#define OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(name, return_reference, required_num_args, class_name, allow_null) \
		ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(name, return_reference, required_num_args, class_name, allow_null)
#endif

/* Introduced in PHP 7.3 */

#if PHP_VERSION_ID < 70000
static zend_always_inline void *zend_object_alloc(size_t obj_size, zend_class_entry *ce) {
	void *obj = emalloc(obj_size);

	memset(obj + sizeof(zend_object), 0, obj_size - sizeof(zend_object));

	return obj;
}
#elif PHP_VERSION_ID < 70300
static zend_always_inline void *zend_object_alloc(size_t obj_size, zend_class_entry *ce) {
	void *obj = emalloc(obj_size + zend_object_properties_size(ce));
	/* Subtraction of sizeof(zval) is necessary, because zend_object_properties_size() may be
	 * -sizeof(zval), if the object has no properties. */
	memset(obj, 0, obj_size - sizeof(zval));
	return obj;
}
#endif

#if PHP_VERSION_ID < 70000

#define ZEND_PARSE_PARAMETERS_NONE() { if (zend_parse_parameters_none() == FAILURE) return; }

#elif PHP_VERSION_ID < 70300

#define ZEND_PARSE_PARAMETERS_NONE()  \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

/* Introduced in PHP 8.0 */
#if PHP_VERSION_ID < 80100
typedef int zend_result;
#define RETURN_THROWS() do { ZEND_ASSERT(EG(exception)); (void) return_value; return; } while (0)
#define Z_PARAM_ARRAY_HT_OR_NULL(dest) Z_PARAM_ARRAY_HT_EX(dest, 1, 0)
#endif

/* Removed in PHP 8.0 */
#if PHP_VERSION_ID >= 80000
#define TSRMLS_DC
#define TSRMLS_CC
#define TSRMLS_FETCH()
#endif

/*
 * End of PHP compatibility wrappers
 */

/* Structure for main OpenCReport object. */
typedef struct _php_opencreport_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	opencreport *o;
	char *expr_error;
	bool free_me;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_object;

static inline php_opencreport_object *php_opencreport_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_object *)((char *)(obj) - XtOffsetOf(php_opencreport_object, zo));
#else
	return (php_opencreport_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_P(zv) php_opencreport_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_P(zv) ((php_opencreport_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

extern zend_class_entry *opencreport_ce;

typedef struct _php_opencreport_ds_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_datasource *ds;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_ds_object;

static inline php_opencreport_ds_object *php_opencreport_ds_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_ds_object *)((char *)(obj) - XtOffsetOf(php_opencreport_ds_object, zo));
#else
	return (php_opencreport_ds_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_DS_P(zv) php_opencreport_ds_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_DS_P(zv) ((php_opencreport_ds_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_query_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_query *q;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_query_object;

static inline php_opencreport_query_object *php_opencreport_query_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_query_object *)((char *)(obj) - XtOffsetOf(php_opencreport_query_object, zo));
#else
	return (php_opencreport_query_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_QUERY_P(zv) php_opencreport_query_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_QUERY_P(zv) ((php_opencreport_query_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_query_result_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_query_result *qr;
#if PHP_VERSION_ID >= 70000
	zend_long cols;
	zend_object zo;
#else
	long cols;
#endif
} php_opencreport_query_result_object;

static inline php_opencreport_query_result_object *php_opencreport_query_result_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_query_result_object *)((char *)(obj) - XtOffsetOf(php_opencreport_query_result_object, zo));
#else
	return (php_opencreport_query_result_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_QUERY_RESULT_P(zv) php_opencreport_query_result_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_QUERY_RESULT_P(zv) ((php_opencreport_query_result_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_expr_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_expr *e;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_expr_object;

static inline php_opencreport_expr_object *php_opencreport_expr_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_expr_object *)((char *)(obj) - XtOffsetOf(php_opencreport_expr_object, zo));
#else
	return (php_opencreport_expr_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_EXPR_P(zv) php_opencreport_expr_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_EXPR_P(zv) ((php_opencreport_expr_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_result_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	opencreport *o;
	ocrpt_result *r;
	bool freed_by_lib;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_result_object;

static inline php_opencreport_result_object *php_opencreport_result_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_result_object *)((char *)(obj) - XtOffsetOf(php_opencreport_result_object, zo));
#else
	return (php_opencreport_result_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_RESULT_P(zv) php_opencreport_result_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_RESULT_P(zv) ((php_opencreport_result_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_part_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	opencreport *o;
	ocrpt_part *p;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_part_object;

static inline php_opencreport_part_object *php_opencreport_part_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_part_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_object, zo));
#else
	return (php_opencreport_part_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_PART_P(zv) php_opencreport_part_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_PART_P(zv) ((php_opencreport_part_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_row_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_part *p;
	ocrpt_part_row *pr;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_row_object;

static inline php_opencreport_row_object *php_opencreport_row_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_row_object *)((char *)(obj) - XtOffsetOf(php_opencreport_row_object, zo));
#else
	return (php_opencreport_row_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_ROW_P(zv) php_opencreport_row_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_ROW_P(zv) ((php_opencreport_row_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_col_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_part_row *pr;
	ocrpt_part_column *pc;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_col_object;

static inline php_opencreport_col_object *php_opencreport_col_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_col_object *)((char *)(obj) - XtOffsetOf(php_opencreport_col_object, zo));
#else
	return (php_opencreport_col_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_COL_P(zv) php_opencreport_col_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_COL_P(zv) ((php_opencreport_col_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_report_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_part_column *pc;
	ocrpt_report *r;
	char *expr_error;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_report_object;

static inline php_opencreport_report_object *php_opencreport_report_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_report_object *)((char *)(obj) - XtOffsetOf(php_opencreport_report_object, zo));
#else
	return (php_opencreport_report_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_REPORT_P(zv) php_opencreport_report_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_REPORT_P(zv) ((php_opencreport_report_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_variable_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_var *v;
	ocrpt_report *r;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_variable_object;

static inline php_opencreport_variable_object *php_opencreport_variable_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_variable_object *)((char *)(obj) - XtOffsetOf(php_opencreport_variable_object, zo));
#else
	return (php_opencreport_variable_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_VARIABLE_P(zv) php_opencreport_variable_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_VARIABLE_P(zv) ((php_opencreport_variable_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_break_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_report *r;
	ocrpt_break *br;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_break_object;

static inline php_opencreport_break_object *php_opencreport_break_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_break_object *)((char *)(obj) - XtOffsetOf(php_opencreport_break_object, zo));
#else
	return (php_opencreport_break_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_BREAK_P(zv) php_opencreport_break_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_BREAK_P(zv) ((php_opencreport_break_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_output_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_output *output;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_output_object;

static inline php_opencreport_output_object *php_opencreport_output_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_output_object *)((char *)(obj) - XtOffsetOf(php_opencreport_output_object, zo));
#else
	return (php_opencreport_output_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_OUTPUT_P(zv) php_opencreport_output_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_OUTPUT_P(zv) ((php_opencreport_output_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_line_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_line *line;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_line_object;

static inline php_opencreport_line_object *php_opencreport_line_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_line_object *)((char *)(obj) - XtOffsetOf(php_opencreport_line_object, zo));
#else
	return (php_opencreport_line_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_LINE_P(zv) php_opencreport_line_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_LINE_P(zv) ((php_opencreport_line_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_hline_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_hline *hline;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_hline_object;

static inline php_opencreport_hline_object *php_opencreport_hline_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_hline_object *)((char *)(obj) - XtOffsetOf(php_opencreport_hline_object, zo));
#else
	return (php_opencreport_hline_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_HLINE_P(zv) php_opencreport_hline_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_HLINE_P(zv) ((php_opencreport_hline_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_image_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_image *image;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_image_object;

static inline php_opencreport_image_object *php_opencreport_image_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_image_object *)((char *)(obj) - XtOffsetOf(php_opencreport_image_object, zo));
#else
	return (php_opencreport_image_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_IMAGE_P(zv) php_opencreport_image_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_IMAGE_P(zv) ((php_opencreport_image_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_text_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_text *text;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_text_object;

static inline php_opencreport_text_object *php_opencreport_text_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_text_object *)((char *)(obj) - XtOffsetOf(php_opencreport_text_object, zo));
#else
	return (php_opencreport_text_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_TEXT_P(zv) php_opencreport_text_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_TEXT_P(zv) ((php_opencreport_text_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_barcode_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_barcode *bc;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_barcode_object;

static inline php_opencreport_barcode_object *php_opencreport_barcode_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_barcode_object *)((char *)(obj) - XtOffsetOf(php_opencreport_barcode_object, zo));
#else
	return (php_opencreport_barcode_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_BARCODE_P(zv) php_opencreport_barcode_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_BARCODE_P(zv) ((php_opencreport_barcode_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_output_element_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_output *output;
	ocrpt_output_element *elem;
	void *iter;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_output_element_object;

static inline php_opencreport_output_element_object *php_opencreport_output_element_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_output_element_object *)((char *)(obj) - XtOffsetOf(php_opencreport_output_element_object, zo));
#else
	return (php_opencreport_output_element_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_OUTPUT_ELEMENT_P(zv)  php_opencreport_output_element_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_OUTPUT_ELEMENT_P(zv) ((php_opencreport_output_element_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_line_element_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_line *line;
	ocrpt_line_element *elem;
	void *iter;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_line_element_object;

static inline php_opencreport_line_element_object *php_opencreport_line_element_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_line_element_object *)((char *)(obj) - XtOffsetOf(php_opencreport_line_element_object, zo));
#else
	return (php_opencreport_line_element_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_LINE_ELEMENT_P(zv)  php_opencreport_line_element_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_LINE_ELEMENT_P(zv) ((php_opencreport_line_element_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

extern zend_class_entry *opencreport_ce;
extern zend_class_entry *opencreport_ds_ce;
extern zend_class_entry *opencreport_query_ce;
extern zend_class_entry *opencreport_query_result_ce;
extern zend_class_entry *opencreport_expr_ce;
extern zend_class_entry *opencreport_result_ce;
extern zend_class_entry *opencreport_part_ce;
extern zend_class_entry *opencreport_row_ce;
extern zend_class_entry *opencreport_col_ce;
extern zend_class_entry *opencreport_report_ce;
extern zend_class_entry *opencreport_variable_ce;
extern zend_class_entry *opencreport_break_ce;
extern zend_class_entry *opencreport_output_ce;
extern zend_class_entry *opencreport_line_ce;
extern zend_class_entry *opencreport_hline_ce;
extern zend_class_entry *opencreport_image_ce;
extern zend_class_entry *opencreport_text_ce;
extern zend_class_entry *opencreport_barcode_ce;
extern zend_class_entry *opencreport_output_element_ce;
extern zend_class_entry *opencreport_line_element_ce;

extern const zend_function_entry opencreport_functions[];

bool opencreport_init(void);
void opencreport_object_deinit(php_opencreport_object *oo);

#if PHP_VERSION_ID >= 70000
OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_add_any_cb, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, callback, IS_STRING, 0)
ZEND_END_ARG_INFO()
#else
#define arginfo_opencreport_add_any_cb NULL
#endif

void register_opencreport_ce(void);
void register_opencreport_ds_ce(void);
void register_opencreport_query_ce(void);
void register_opencreport_query_result_ce(void);
void register_opencreport_expr_ce(void);
void register_opencreport_result_ce(void);
void register_opencreport_part_ce(void);
void register_opencreport_row_ce(void);
void register_opencreport_col_ce(void);
void register_opencreport_report_ce(void);
void register_opencreport_variable_ce(void);
void register_opencreport_break_ce(void);
void register_opencreport_output_ce(void);
void register_opencreport_line_ce(void);
void register_opencreport_hline_ce(void);
void register_opencreport_image_ce(void);
void register_opencreport_text_ce(void);
void register_opencreport_barcode_ce(void);
void register_opencreport_output_element_ce(void);
void register_opencreport_line_element_ce(void);

void opencreport_part_cb(opencreport *o, ocrpt_part *p, void *data);
void opencreport_report_cb(opencreport *o, ocrpt_report *r, void *data);

#endif /* PHP_OPENCREPORT_H */
