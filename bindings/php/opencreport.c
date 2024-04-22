/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "zend_interfaces.h"
#include "php_opencreport.h"
#include <errno.h>
#include <stdio.h>

#include <opencreport.h>

/* {{{ REGISTER_OPENCREPORT_CLASS_CONST_LONG */

/*
 * The below macro is open-coding zend_declare_class_constant_long()
 * with some changes. Since PHP 8.1, class constants can be final.
 * Use the ZEND_ACC_FINAL flag to disallow overriding our constants.
 */
#if PHP_VERSION_ID >= 80100
# define MY_FINAL_CONST (ZEND_ACC_FINAL)
#else
# define MY_FINAL_CONST (0)
#endif

#define REGISTER_OPENCREPORT_CLASS_CONST_LONG(const_name, value) { \
		zval constant; \
		ZVAL_LONG(&constant, (zend_long)value); \
		zend_string *key; \
		if (opencreport_ce->type == ZEND_INTERNAL_CLASS) { \
			key = zend_string_init_interned(const_name, sizeof(const_name)-1, 1); \
		} else { \
			key = zend_string_init(const_name, sizeof(const_name)-1, 0); \
		} \
		zend_declare_class_constant_ex(opencreport_ce, key, &constant, ZEND_ACC_PUBLIC | MY_FINAL_CONST, NULL); \
		zend_string_release(key); \
	}
/* }}} */

/* Handlers */
static zend_object_handlers opencreport_object_handlers;
static zend_object_handlers opencreport_ds_object_handlers;
static zend_object_handlers opencreport_query_object_handlers;
static zend_object_handlers opencreport_query_result_object_handlers;
static zend_object_handlers opencreport_expr_object_handlers;
static zend_object_handlers opencreport_result_object_handlers;
static zend_object_handlers opencreport_part_object_handlers;
static zend_object_handlers opencreport_row_object_handlers;
static zend_object_handlers opencreport_col_object_handlers;
static zend_object_handlers opencreport_report_object_handlers;
static zend_object_handlers opencreport_variable_object_handlers;
static zend_object_handlers opencreport_break_object_handlers;
static zend_object_handlers opencreport_output_object_handlers;
static zend_object_handlers opencreport_line_object_handlers;
static zend_object_handlers opencreport_hline_object_handlers;
static zend_object_handlers opencreport_image_object_handlers;
static zend_object_handlers opencreport_text_object_handlers;
static zend_object_handlers opencreport_barcode_object_handlers;

/* Class entries */
zend_class_entry *opencreport_ce;
zend_class_entry *opencreport_ds_ce;
zend_class_entry *opencreport_query_ce;
zend_class_entry *opencreport_query_result_ce;
zend_class_entry *opencreport_expr_ce;
zend_class_entry *opencreport_result_ce;
zend_class_entry *opencreport_part_ce;
zend_class_entry *opencreport_row_ce;
zend_class_entry *opencreport_col_ce;
zend_class_entry *opencreport_report_ce;
zend_class_entry *opencreport_variable_ce;
zend_class_entry *opencreport_break_ce;
zend_class_entry *opencreport_output_ce;
zend_class_entry *opencreport_line_ce;
zend_class_entry *opencreport_hline_ce;
zend_class_entry *opencreport_image_ce;
zend_class_entry *opencreport_text_ce;
zend_class_entry *opencreport_barcode_ce;

static zend_object *opencreport_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_object *intern = zend_object_alloc(sizeof(php_opencreport_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_object_handlers;

	intern->o = NULL;
	intern->expr_error = NULL;
	intern->free_me = false;

	return &intern->zo;
}
/* }}} */

static void opencreport_object_deinit(php_opencreport_object *oo) {
	ocrpt_strfree(oo->expr_error);
	oo->expr_error = NULL;
	if (oo->free_me)
		ocrpt_free(oo->o);
	oo->o = NULL;
}

static void opencreport_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_object *oo = php_opencreport_from_obj(object);

	if (!oo->o)
		return;

	opencreport_object_deinit(oo);

	zend_object_std_dtor(&oo->zo);
}
/* }}} */

static zend_object *opencreport_ds_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_ds_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_ds_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_ds_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_query_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_query_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_query_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_query_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_query_result_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_query_result_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_query_result_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_query_result_object_handlers;

	intern->qr = NULL;
	intern->cols = 0;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_expr_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_expr_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_expr_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_expr_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_result_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_result_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_result_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_result_object_handlers;

	return &intern->zo;
}
/* }}} */

static void opencreport_result_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_result_object *ro = php_opencreport_result_from_obj(object);

	if (ro->r && !ro->freed_by_lib)
		ocrpt_result_free(ro->r);

	ro->r = NULL;

	zend_object_std_dtor(object);
}
/* }}} */

static zend_object *opencreport_part_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_part_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_part_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_part_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_row_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_row_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_row_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_row_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_col_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_col_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_col_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_col_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_report_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_report_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_report_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_report_object_handlers;

	return &intern->zo;
}
/* }}} */

static void opencreport_report_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_report_object *pro = php_opencreport_report_from_obj(object);

	ocrpt_strfree(pro->expr_error);

	zend_object_std_dtor(&pro->zo);
}
/* }}} */

static zend_object *opencreport_variable_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_variable_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_variable_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_variable_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_break_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_break_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_break_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_break_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_output_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_output_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_output_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_output_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_line_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_line_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_line_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_line_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_hline_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_hline_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_hline_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_hline_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_image_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_image_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_image_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_image_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_text_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_text_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_text_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_text_object_handlers;

	return &intern->zo;
}
/* }}} */

static zend_object *opencreport_barcode_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_barcode_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_barcode_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_barcode_object_handlers;

	return &intern->zo;
}
/* }}} */

PHP_METHOD(opencreport, __construct) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	oo->o = ocrpt_init();
	oo->free_me = true;
}

PHP_METHOD(opencreport, parse_xml) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *filename;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(filename);
	ZEND_PARSE_PARAMETERS_END();

	RETURN_BOOL(ocrpt_parse_xml(oo->o, ZSTR_VAL(filename)));
}

PHP_METHOD(opencreport, parse_xml_from_buffer) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *buffer;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(buffer);
	ZEND_PARSE_PARAMETERS_END();

	RETURN_BOOL(ocrpt_parse_xml_from_buffer(oo->o, ZSTR_VAL(buffer), ZSTR_LEN(buffer)));
}

PHP_METHOD(opencreport, set_output_format) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_long format;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(format);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_output_format(oo->o, format);
}

PHP_METHOD(opencreport, set_output_parameter) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *param;
	zend_string *value;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_STR(param);
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_output_parameter(oo->o, ZSTR_VAL(param), ZSTR_VAL(value));
}

PHP_METHOD(opencreport, execute) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_execute(oo->o));
}

PHP_METHOD(opencreport, spool) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_spool(oo->o);
}

PHP_METHOD(opencreport, get_output) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	size_t sz = 0;
	const char *res = ocrpt_get_output(oo->o, &sz);

	if (!res)
		RETURN_FALSE;

	RETURN_STRINGL(res, sz);
}

PHP_METHOD(opencreport, get_content_type) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	const ocrpt_string **content_type = ocrpt_get_content_type(oo->o);

	if (!content_type)
		RETURN_FALSE;

	array_init(return_value);

	for (int32_t i = 0; content_type[i]; i++) {
		zval tmp;
		ZVAL_STRINGL(&tmp, content_type[i]->str, content_type[i]->len);
		zend_hash_next_index_insert_new(Z_ARRVAL_P(return_value), &tmp);
	}
}

PHP_METHOD(opencreport, version) {
	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_STRING(ocrpt_version());
}

PHP_METHOD(opencreport, set_numeric_precision_bits) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string = NULL;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_numeric_precision_bits(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport, set_rounding_mode) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string = NULL;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_rounding_mode(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport, bindtextdomain) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *domainname;
	zend_string *dirname;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_STR(domainname);
		Z_PARAM_STR(dirname);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_bindtextdomain(oo->o, ZSTR_VAL(domainname), ZSTR_VAL(dirname));
}

PHP_METHOD(opencreport, set_locale) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *locale;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(locale);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_locale(oo->o, ZSTR_VAL(locale));
}

PHP_METHOD(opencreport, datasource_add) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *source_type;
	HashTable *source_params = NULL;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;
	ocrpt_input_connect_parameter *conn_params = NULL;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR(source_type);
		Z_PARAM_OPTIONAL;
		Z_PARAM_ARRAY_HT_OR_NULL(source_params);
	ZEND_PARSE_PARAMETERS_END();

	if (source_params) {
		uint32_t n_elements = zend_hash_num_elements(source_params);

		if (n_elements) {
			HashPosition source_param_pos;
			uint32_t n_params = 0, pos = 0;

			conn_params = ocrpt_mem_malloc((n_elements + 1) * sizeof(ocrpt_input_connect_parameter));
			memset(conn_params, 0, (n_elements + 1) * sizeof(ocrpt_input_connect_parameter));

			for (zend_hash_internal_pointer_reset_ex(source_params, &source_param_pos); pos < n_elements; pos++, zend_hash_move_forward_ex(source_params, &source_param_pos)) {
				zval key, *value;

				zend_hash_get_current_key_zval_ex(source_params, &key, &source_param_pos);
				if (Z_TYPE(key) != IS_STRING)
					continue;

				value = zend_hash_get_current_data_ex(source_params, &source_param_pos);
				if (Z_TYPE_P(value) != IS_STRING)
					continue;

				conn_params[n_params].param_name = Z_STRVAL(key);
				conn_params[n_params].param_value = Z_STRVAL_P(value);
				n_params++;
			}
		}
	}

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), ZSTR_VAL(source_type), conn_params);

	ocrpt_mem_free(conn_params);

	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_get) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_get(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, query_get) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *query_name;
	ocrpt_query *q;
	php_opencreport_query_object *qo;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(query_name);
	ZEND_PARSE_PARAMETERS_END();

	q = ocrpt_query_get(oo->o, ZSTR_VAL(query_name));
	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;
}

PHP_METHOD(opencreport, query_refresh) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_query_refresh(oo->o);
}

PHP_METHOD(opencreport, expr_parse) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(expr_string);
	ZEND_PARSE_PARAMETERS_END();

	char *err = NULL;
	ocrpt_expr *e = ocrpt_expr_parse(oo->o, ZSTR_VAL(expr_string), &err);

	ocrpt_strfree(oo->expr_error);
	if (e) {
		object_init_ex(return_value, opencreport_expr_ce);
		php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
		eo->e = e;
		oo->expr_error = NULL;
	} else {
		oo->expr_error = err;
		RETURN_NULL();
	}
}

PHP_METHOD(opencreport, expr_error) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	if (oo->expr_error)
		RETURN_STRING(oo->expr_error);
	else
		RETURN_NULL();
}

OCRPT_STATIC_FUNCTION(opencreport_default_function) {
	zend_string *fname = user_data;
	zval zfname;

	ocrpt_expr_make_error_result(e, "not implemented");

	ZVAL_STRINGL(&zfname, ZSTR_VAL(fname), ZSTR_LEN(fname));

	zval retval;
	zval params[1];

	object_init_ex(&params[0], opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(&params[0]);
	eo->e = e;

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, 1, params) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 1, params, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_object_std_dtor(&eo->zo);
	OBJ_RELEASE(&eo->zo);

	zend_string_release(Z_STR(zfname));

	/*
	 * The function must be a real OpenCReports user function
	 * and called $e->set_{long|double|string}_value(...)
	 */
	if (Z_TYPE(retval) == IS_UNDEF || Z_TYPE(retval) == IS_NULL)
		return;
	else if (Z_TYPE(retval) == IS_FALSE)
		ocrpt_expr_set_long(e, 0);
	else if (Z_TYPE(retval) == IS_TRUE)
		ocrpt_expr_set_long(e, 1);
	else if (Z_TYPE(retval) == IS_LONG)
		ocrpt_expr_set_long(e, Z_LVAL(retval));
	else if (Z_TYPE(retval) == IS_DOUBLE)
		ocrpt_expr_set_double(e, Z_DVAL(retval));
	else if (Z_TYPE(retval) == IS_STRING)
		ocrpt_expr_set_string(e, Z_STRVAL(retval));
	else
		ocrpt_expr_make_error_result(e, "invalid return value");
}

PHP_METHOD(opencreport, part_new) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_part *p = ocrpt_part_new(oo->o);
	if (!p)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_part_ce);
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(return_value);
	po->o = NULL;
	po->p = p;
	po->iter = NULL;
	po->is_iterator = false;
}

PHP_METHOD(opencreport, part_get_first) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_part *p = ocrpt_part_get_next(oo->o, &iter);

	if (!p)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_part_ce);
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(return_value);
	po->o = oo->o;
	po->p = p;
	po->iter = iter;
	po->is_iterator = true;
}

PHP_METHOD(opencreport, function_add) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_func_name;
	zend_string *zend_func_name;
	zend_long n_ops;
	zend_bool commutative, associative, left_associative, dont_optimize;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 7, 7)
		Z_PARAM_STR(expr_func_name);
		Z_PARAM_STR(zend_func_name);
		Z_PARAM_LONG(n_ops);
		Z_PARAM_BOOL(commutative);
		Z_PARAM_BOOL(associative);
		Z_PARAM_BOOL(left_associative);
		Z_PARAM_BOOL(dont_optimize);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *zfunc = zend_string_init(ZSTR_VAL(zend_func_name), ZSTR_LEN(zend_func_name), false);
	bool ret = ocrpt_function_add(oo->o, ZSTR_VAL(expr_func_name), opencreport_default_function, zfunc, n_ops, commutative, associative, left_associative, dont_optimize);

	RETURN_BOOL(ret);
}

static ocrpt_result *php_opencreport_env_query(opencreport *o, const char *env) {
	if (!env)
		return NULL;

	ocrpt_result *result = ocrpt_result_new(o);
	if (!result)
		return NULL;

	bool found = false;
	zval *var = zend_hash_str_find(&EG(symbol_table), env, strlen(env));
	if (var) {
		for (int unref_count = 0; unref_count < 3 && (Z_TYPE_P(var) == IS_INDIRECT || Z_TYPE_P(var) == IS_REFERENCE); unref_count++) {
			if (EXPECTED(Z_TYPE_P(var) == IS_INDIRECT))
				var = Z_INDIRECT_P(var);
			if (EXPECTED(Z_TYPE_P(var) == IS_REFERENCE))
				var = Z_REFVAL_P(var);
		}

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

static void opencreport_cb(opencreport *o, void *data) {
	zend_string *callback = data;
	zval zfname;

	ZVAL_STRINGL(&zfname, ZSTR_VAL(callback), ZSTR_LEN(callback));

	zval retval;
	zval params[1];

	object_init_ex(&params[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(&params[0]);
	oo->o = o;

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, 1, params) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 1, params, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_object_std_dtor(&oo->zo);
	OBJ_RELEASE(&oo->zo);

	zend_string_release(Z_STR(zfname));
}

static void opencreport_part_cb(opencreport *o, ocrpt_part *p, void *data) {
	zend_string *callback = data;
	zval zfname;

	ZVAL_STRINGL(&zfname, ZSTR_VAL(callback), ZSTR_LEN(callback));

	zval retval;
	zval params[2];

	object_init_ex(&params[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(&params[0]);
	oo->o = o;

	object_init_ex(&params[1], opencreport_part_ce);
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(&params[1]);
	po->p = p;

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, 2, params) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 2, params, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_object_std_dtor(&oo->zo);
	OBJ_RELEASE(&oo->zo);
	zend_object_std_dtor(&po->zo);
	OBJ_RELEASE(&po->zo);

	zend_string_release(Z_STR(zfname));
}

PHP_METHOD(opencreport, add_precalculation_done_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_add_precalculation_done_cb(oo->o, opencreport_cb, cb_name);
}

PHP_METHOD(opencreport, add_part_added_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_add_part_added_cb(oo->o, opencreport_part_cb, cb_name);
}

static void opencreport_report_cb(opencreport *o, ocrpt_report *r, void *data) {
	zend_string *callback = data;
	zval zfname;

	ZVAL_STRINGL(&zfname, ZSTR_VAL(callback), ZSTR_LEN(callback));

	zval retval;
	zval params[2];

	object_init_ex(&params[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(&params[0]);
	oo->o = o;

	object_init_ex(&params[1], opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(&params[1]);
	pro->r = r;

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, 2, params) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 2, params, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_object_std_dtor(&oo->zo);
	OBJ_RELEASE(&oo->zo);
	zend_object_std_dtor(&pro->zo);
	OBJ_RELEASE(&pro->zo);

	zend_string_release(Z_STR(zfname));
}

PHP_METHOD(opencreport, add_report_added_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_add_report_added_cb(oo->o, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport, env_get) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *var_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(var_name);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_result *r = ocrpt_env_get(oo->o, ZSTR_VAL(var_name));
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->o = oo->o;
	ro->r = r;
	ro->freed_by_lib = false;
}

PHP_METHOD(opencreport, result_new) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result *r = ocrpt_result_new(oo->o);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->o = oo->o;
	ro->r = r;
	ro->freed_by_lib = false;
}

PHP_METHOD(opencreport, set_mvariable) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *name;
	zend_string *value = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 2)
		Z_PARAM_STR(name);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(value, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_mvariable(oo->o, ZSTR_VAL(name), value ? ZSTR_VAL(value) : NULL);
}

PHP_METHOD(opencreport, add_search_path) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *path;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(path);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_add_search_path(oo->o, ZSTR_VAL(path));
}

PHP_METHOD(opencreport, canonicalize_path) {
	zend_string *path;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(path);
	ZEND_PARSE_PARAMETERS_END();

	RETURN_STRING(ocrpt_canonicalize_path(ZSTR_VAL(path)));
}

PHP_METHOD(opencreport, find_file) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *path;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(path);
	ZEND_PARSE_PARAMETERS_END();

	char *file = ocrpt_find_file(oo->o, ZSTR_VAL(path));

	if (!file)
		RETURN_NULL();

	RETVAL_STRING(file);
	ocrpt_mem_free(file);
}

PHP_METHOD(opencreport, get_color) {
	zend_string *color_name = NULL;
	zend_bool bgcolor = false;
	ocrpt_color c;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 2)
		Z_PARAM_STR_EX(color_name, 1, 0);
		Z_PARAM_OPTIONAL;
		Z_PARAM_BOOL(bgcolor);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_get_color(color_name ? ZSTR_VAL(color_name) : NULL, &c, bgcolor);

	array_init(return_value);

	zval tmp;

	ZVAL_DOUBLE(&tmp, c.r);
	zend_hash_str_add_new(Z_ARRVAL_P(return_value), "r", 1, &tmp);

	ZVAL_DOUBLE(&tmp, c.g);
	zend_hash_str_add_new(Z_ARRVAL_P(return_value), "g", 1, &tmp);

	ZVAL_DOUBLE(&tmp, c.b);
	zend_hash_str_add_new(Z_ARRVAL_P(return_value), "b", 1, &tmp);
}

PHP_METHOD(opencreport, set_paper) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *paper;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(paper);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_paper_by_name(oo->o, ZSTR_VAL(paper));
}

PHP_METHOD(opencreport, set_size_unit) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_size_unit(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport, set_noquery_show_nodata) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_noquery_show_nodata(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport, set_report_height_after_last) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_report_height_after_last(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport, set_follower_match_single) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_follower_match_single(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport, set_follower_match_single_direct) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_bool value = false;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_BOOL(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_follower_match_single_direct(oo->o, value);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_opencreport___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_parse_xml, 0, 1, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_parse_xml_from_buffer, 0, 1, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, buffer, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_output_format, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, format, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_output_parameter, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, param, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_execute, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_spool, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_output, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_content_type, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_version, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_numeric_precision_bits, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_rounding_mode, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_bindtextdomain, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, domainname, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dirname, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_locale, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, locale, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, source_type, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, source_params, IS_ARRAY, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_get, 0, 1, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_query_get, 0, 1, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, query_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_array_query_refresh, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_parse, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_error, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_new, 0, 0, OpenCReport\\Part, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_first, 0, 0, OpenCReport\\Part, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_function_add, 0, 7, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, expr_func_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, zend_func_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, n_ops, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, commutative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, associative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, left_associative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, dont_optimize, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_add_any_cb, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, callback, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_env_get, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, var_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_result_new, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, var_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_mvariable, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, name, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_add_search_path, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_canonicalize_path, 0, 1, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_find_file, 0, 1, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_color, 0, 0, IS_ARRAY, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, color_name, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, bgcolor, _IS_BOOL, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_paper, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, paper, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_size_unit, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_noquery_show_nodata, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_report_height_after_last, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_follower_match_single, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_follower_match_single_direct, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_class_methods[] = {
	/*
	 * High level API
	 */
	PHP_ME(opencreport, __construct, arginfo_opencreport___construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, parse_xml, arginfo_opencreport_parse_xml, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, parse_xml_from_buffer, arginfo_opencreport_parse_xml_from_buffer, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_output_format, arginfo_opencreport_set_output_format, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_output_parameter, arginfo_opencreport_set_output_parameter, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, execute, arginfo_opencreport_execute, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, spool, arginfo_opencreport_spool, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_output, arginfo_opencreport_get_output, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_content_type, arginfo_opencreport_get_content_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, version, arginfo_opencreport_version, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	/*
	 * Low level API
	 */
	/* Numeric behavior related methods */
	PHP_ME(opencreport, set_numeric_precision_bits, arginfo_opencreport_set_numeric_precision_bits, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_rounding_mode, arginfo_opencreport_set_rounding_mode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Locale related methods */
	PHP_ME(opencreport, bindtextdomain, arginfo_opencreport_bindtextdomain, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_locale, arginfo_opencreport_set_locale, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Datasource and query related methods */
	PHP_ME(opencreport, datasource_add, arginfo_opencreport_datasource_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_get, arginfo_opencreport_datasource_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, query_get, arginfo_opencreport_query_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, query_refresh, arginfo_array_query_refresh, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Expression related methods */
	PHP_ME(opencreport, expr_parse, arginfo_opencreport_expr_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, expr_error, arginfo_opencreport_expr_error, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Function related methods */
	PHP_ME(opencreport, function_add, arginfo_opencreport_function_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Report part related functions */
	PHP_ME(opencreport, part_new, arginfo_opencreport_part_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, part_get_first, arginfo_opencreport_part_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Callback related methods */
	PHP_ME(opencreport, add_precalculation_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, add_part_added_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, add_report_added_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Environment related methods */
	PHP_ME(opencreport, env_get, arginfo_opencreport_env_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_mvariable, arginfo_opencreport_set_mvariable, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Result related methods */
	PHP_ME(opencreport, result_new, arginfo_opencreport_result_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* File handling related methods */
	PHP_ME(opencreport, add_search_path, arginfo_opencreport_add_search_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, canonicalize_path, arginfo_opencreport_canonicalize_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	PHP_ME(opencreport, find_file, arginfo_opencreport_find_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Color related methods */
	PHP_ME(opencreport, get_color, arginfo_opencreport_get_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	/* Paper related methods */
	PHP_ME(opencreport, set_paper, arginfo_opencreport_set_paper, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Layout related methods */
	PHP_ME(opencreport, set_size_unit, arginfo_opencreport_set_size_unit, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_noquery_show_nodata, arginfo_opencreport_set_noquery_show_nodata, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_report_height_after_last, arginfo_opencreport_set_report_height_after_last, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_follower_match_single, arginfo_opencreport_set_follower_match_single, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_follower_match_single_direct, arginfo_opencreport_set_follower_match_single_direct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_ds, free) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_datasource_free(dso->ds);
	dso->ds = NULL;
}

PHP_METHOD(opencreport_ds, query_add) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(object);
	zend_string *name, *array_or_file_or_sql, *coltypes = NULL;
	ocrpt_query *q;
	php_opencreport_query_object *qo;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(name);
		Z_PARAM_STR(array_or_file_or_sql);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(coltypes, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	if (ocrpt_datasource_is_sql(dso->ds)) {
		q = ocrpt_query_add_sql(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	} else if (ocrpt_datasource_is_file(dso->ds)) {
		void *types_x = NULL;
		int32_t types_cols = 0;
		bool free_types = false;

		if (coltypes && ZSTR_LEN(coltypes) > 0)
			ocrpt_query_discover_data(NULL, NULL, NULL, NULL, ZSTR_VAL(coltypes), &types_x, &types_cols, &free_types);

		q = ocrpt_query_add_file(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), types_x, types_cols);

		if (free_types)
			ocrpt_mem_free(types_x);
	} else if (ocrpt_datasource_is_symbolic_data(dso->ds))
		q = ocrpt_query_add_symbolic_data(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), 0, 0, coltypes ? ZSTR_VAL(coltypes) : NULL, 0);
	else
		RETURN_NULL();

	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;
}

PHP_METHOD(opencreport_ds, set_encoding) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(object);
	zend_string *encoding;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(encoding);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_datasource_set_encoding(dso->ds, ZSTR_VAL(encoding));
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_ds_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_ds_query_add, 0, 2, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, array_or_file_or_sql, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, types, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_ds_set_encoding, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, encoding, IS_STRING, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_ds_class_methods[] = {
	PHP_ME(opencreport_ds, free, arginfo_opencreport_ds_free, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_ds, query_add, arginfo_opencreport_ds_query_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_ds, set_encoding, arginfo_opencreport_ds_set_encoding, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_query, get_result) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	int32_t cols;
	ocrpt_query_result *qr = ocrpt_query_get_result(qo->q, &cols);

	if (!qr)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_result_ce);
	php_opencreport_query_result_object *qro = Z_OPENCREPORT_QUERY_RESULT_P(return_value);
	qro->cols = cols;
	qro->qr = qr;
}

PHP_METHOD(opencreport_query, navigate_start) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_navigate_start(qo->q);
}

PHP_METHOD(opencreport_query, navigate_next) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_query_navigate_next(qo->q));
}

PHP_METHOD(opencreport_query, navigate_use_prev_row) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_navigate_use_prev_row(qo->q);
}

PHP_METHOD(opencreport_query, navigate_use_next_row) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_navigate_use_next_row(qo->q);
}

PHP_METHOD(opencreport_query, add_follower) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);
	zval *fobj;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(fobj, opencreport_query_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_query_object *fo = Z_OPENCREPORT_QUERY_P(fobj);

	RETURN_BOOL(ocrpt_query_add_follower(qo->q, fo->q));
}

PHP_METHOD(opencreport_query, add_follower_n_to_1) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);
	zval *fobj;
	zval *mobj;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(fobj, opencreport_query_ce);
		Z_PARAM_OBJECT_OF_CLASS(mobj, opencreport_expr_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_query_object *fo = Z_OPENCREPORT_QUERY_P(fobj);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(mobj);

	bool retval = ocrpt_query_add_follower_n_to_1(qo->q, fo->q, eo->e);

	/* The expression's ownership was taken over or it was freed. */
	eo->e = NULL;

	RETURN_BOOL(retval);
}

PHP_METHOD(opencreport_query, free) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	if (!qo->q) {
		zend_throw_error(NULL, "OpenCReport\\Query object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_free(qo->q);
	qo->q = NULL;
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_query_get_result, 0, 0, OpenCReport\\QueryResult, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_navigate_start, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_navigate_next, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_navigate_use_prev_row, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_navigate_use_next_row, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_add_follower, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, follower, OpenCReport\\Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_add_follower_n_to_1, 0, 2, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, follower, OpenCReport\\Query, 0)
ZEND_ARG_OBJ_INFO(0, match, OpenCReport\\Expr, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_query_class_methods[] = {
	PHP_ME(opencreport_query, get_result, arginfo_opencreport_query_get_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, navigate_start, arginfo_opencreport_query_navigate_start, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, navigate_next, arginfo_opencreport_query_navigate_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, navigate_use_prev_row, arginfo_opencreport_query_navigate_use_prev_row, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, navigate_use_next_row, arginfo_opencreport_query_navigate_use_next_row, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, add_follower, arginfo_opencreport_query_add_follower, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, add_follower_n_to_1, arginfo_opencreport_query_add_follower_n_to_1, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, free, arginfo_opencreport_query_free, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_query_result, columns) {
	zval *object = ZEND_THIS;
	php_opencreport_query_result_object *qr = Z_OPENCREPORT_QUERY_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(qr->cols);
}

PHP_METHOD(opencreport_query_result, column_name) {
	zval *object = ZEND_THIS;
	php_opencreport_query_result_object *qr = Z_OPENCREPORT_QUERY_RESULT_P(object);
	zend_long index;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(index);
	ZEND_PARSE_PARAMETERS_END();

	if (index < 0 || index >= qr->cols)
		RETURN_NULL();

	RETURN_STRING(ocrpt_query_result_column_name(qr->qr, index));
}

PHP_METHOD(opencreport_query_result, column_result) {
	zval *object = ZEND_THIS;
	php_opencreport_query_result_object *qr = Z_OPENCREPORT_QUERY_RESULT_P(object);
	zend_long index;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(index);
	ZEND_PARSE_PARAMETERS_END();

	if (index < 0 || index >= qr->cols)
		RETURN_NULL();

	ocrpt_result *r = ocrpt_query_result_column_result(qr->qr, index);

	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->r = r;
	ro->freed_by_lib = true;
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_result_columns, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_result_column_name, 0, 1, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_query_result_column_result, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_query_result_class_methods[] = {
	PHP_ME(opencreport_query_result, columns, arginfo_opencreport_query_result_columns, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query_result, column_name, arginfo_opencreport_query_result_column_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query_result, column_result, arginfo_opencreport_query_result_column_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_expr, free) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_free(eo->e);
	eo->e = NULL;
}

PHP_METHOD(opencreport_expr, print) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_print(eo->e);
}

PHP_METHOD(opencreport_expr, nodes) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_nodes(eo->e));
}

PHP_METHOD(opencreport_expr, optimize) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_optimize(eo->e);
}

PHP_METHOD(opencreport_expr, resolve) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_resolve(eo->e);
}

PHP_METHOD(opencreport_expr, eval) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	php_opencreport_result_object *ro;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result *r = ocrpt_expr_eval(eo->e);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->r = r;
	ro->freed_by_lib = true;
}

PHP_METHOD(opencreport_expr, get_result) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result *r = ocrpt_expr_get_result(eo->e);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->o = NULL;
	ro->r = r;
	ro->freed_by_lib = true;
}

PHP_METHOD(opencreport_expr, set_string) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_string *value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_string(eo->e, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, set_long) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_long value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_long(eo->e, value);
}

PHP_METHOD(opencreport_expr, set_double) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	double value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_DOUBLE(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_double(eo->e, value);
}

PHP_METHOD(opencreport_expr, set_number) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_string *value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_number_from_string(eo->e, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, get_num_operands) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_get_num_operands(eo->e));
}

PHP_METHOD(opencreport_expr, operand_get_result) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_long opidx;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(opidx);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_result *r = ocrpt_expr_operand_get_result(eo->e, opidx);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->r = r;
	ro->freed_by_lib = true;
}

PHP_METHOD(opencreport_expr, cmp_results) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_expr_cmp_results(eo->e));
}

PHP_METHOD(opencreport_expr, init_results) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_long result_type;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(result_type);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_init_results(eo->e, result_type);
}

PHP_METHOD(opencreport_expr, get_string) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	const ocrpt_string *s = ocrpt_expr_get_string(eo->e);

	if (!s)
		RETURN_NULL();

	RETURN_STRINGL(s->str, s->len);
}

PHP_METHOD(opencreport_expr, get_long) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_get_long(eo->e));
}

PHP_METHOD(opencreport_expr, get_double) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_DOUBLE(ocrpt_expr_get_double(eo->e));
}

PHP_METHOD(opencreport_expr, get_number) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_string *format = NULL;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR(format);
	ZEND_PARSE_PARAMETERS_END();

	char *fmt = format ? ZSTR_VAL(format) : "%RF";
	mpfr_ptr number = ocrpt_expr_get_number(eo->e);
	if (!number)
		RETURN_NULL();

	size_t len = mpfr_snprintf(NULL, 0, fmt, number);
	char *retval = emalloc(len + 1);
	mpfr_snprintf(retval, len + 1, fmt, number);

	RETURN_STRINGL(retval, len);
}

PHP_METHOD(opencreport_expr, set_nth_result_string) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_long which;
	zend_string *value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_LONG(which);
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_nth_result_string(eo->e, which, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, set_nth_result_long) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_long which;
	zend_long value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_LONG(which);
		Z_PARAM_LONG(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_nth_result_long(eo->e, which, value);
}

PHP_METHOD(opencreport_expr, set_nth_result_double) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_long which;
	double value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_LONG(which);
		Z_PARAM_DOUBLE(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_nth_result_double(eo->e, which, value);
}

PHP_METHOD(opencreport_expr, set_iterative_start_value) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_bool value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_BOOL(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_iterative_start_value(eo->e, value);
}

PHP_METHOD(opencreport_expr, set_delayed) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_bool value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_BOOL(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_delayed(eo->e, value);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_print, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_nodes, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_optimize, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_resolve, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_eval, 0, 0, OpenCReport\\Result, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_get_result, 0, 0, OpenCReport\\Result, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_string, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_long, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_double, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_number, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_num_operands, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_operand_get_result, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, opidx, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_cmp_results, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_init_results, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, result_type, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_string, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_long, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_double, 0, 0, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_number, 0, 0, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, format, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_string, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_long, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_double, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_iterative_start_value, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_delayed, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_expr_class_methods[] = {
	PHP_ME(opencreport_expr, free, arginfo_opencreport_expr_free, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, print, arginfo_opencreport_expr_print, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, nodes, arginfo_opencreport_expr_nodes, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, optimize, arginfo_opencreport_expr_optimize, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, resolve, arginfo_opencreport_expr_resolve, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, eval, arginfo_opencreport_expr_eval, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_result, arginfo_opencreport_expr_get_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_string, arginfo_opencreport_expr_set_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_long, arginfo_opencreport_expr_set_long, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_double, arginfo_opencreport_expr_set_double, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_number, arginfo_opencreport_expr_set_number, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_num_operands, arginfo_opencreport_expr_get_num_operands, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, operand_get_result, arginfo_opencreport_expr_operand_get_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, cmp_results, arginfo_opencreport_expr_cmp_results, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, init_results, arginfo_opencreport_expr_init_results, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_string, arginfo_opencreport_expr_get_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_long, arginfo_opencreport_expr_get_long, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_double, arginfo_opencreport_expr_get_double, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_number, arginfo_opencreport_expr_get_number, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_string, arginfo_opencreport_expr_set_nth_result_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_long, arginfo_opencreport_expr_set_nth_result_long, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_double, arginfo_opencreport_expr_set_nth_result_double, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_iterative_start_value, arginfo_opencreport_expr_set_iterative_start_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_delayed, arginfo_opencreport_expr_set_delayed, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_result, free) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ro->freed_by_lib)
		ocrpt_result_free(ro->r);

	ro->r = NULL;
}

PHP_METHOD(opencreport_result, copy) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);
	zval *src_result;

	if (!ro->r) {
		zend_throw_error(NULL, "OpenCReport\\Result object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(src_result, opencreport_result_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_result_object *src_ro = Z_OPENCREPORT_RESULT_P(src_result);
	if (!src_ro->r) {
		zend_throw_error(NULL, "OpenCReport\\Result source object was freed");
		RETURN_THROWS();
	}

	ocrpt_result_copy(ro->r, src_ro->r);
}

PHP_METHOD(opencreport_result, print) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result_print(ro->r);
}

PHP_METHOD(opencreport_result, get_type) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_result_get_type(ro->r));
}

PHP_METHOD(opencreport_result, is_null) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isnull(ro->r));
}

PHP_METHOD(opencreport_result, is_string) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isstring(ro->r));
}

PHP_METHOD(opencreport_result, is_number) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isnumber(ro->r));
}

PHP_METHOD(opencreport_result, get_string) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_string *s = ocrpt_result_get_string(ro->r);
	if (!s)
		RETURN_NULL();

	RETURN_STRINGL(s->str, s->len);
}

PHP_METHOD(opencreport_result, get_number) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);
	zend_string *format = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(format, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	char *fmt = format ? ZSTR_VAL(format) : "%RF";
	mpfr_ptr number = ocrpt_result_get_number(ro->r);
	if (!number)
		RETURN_NULL();

	size_t len = mpfr_snprintf(NULL, 0, fmt, number);
	char *retval = emalloc(len + 1);
	mpfr_snprintf(retval, len + 1, fmt, number);

	RETURN_STRINGL(retval, len);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_copy, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, src_result, OpenCReport\\Result, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_print, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_get_type, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_is_null, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_is_string, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_is_number, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_get_string, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_get_number, 0, 0, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, format, IS_STRING, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_result_class_methods[] = {
	PHP_ME(opencreport_result, free, arginfo_opencreport_result_free, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, copy, arginfo_opencreport_result_copy, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, print, arginfo_opencreport_result_print, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, get_type, arginfo_opencreport_result_get_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, is_null, arginfo_opencreport_result_is_null, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, is_string, arginfo_opencreport_result_is_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, is_number, arginfo_opencreport_result_is_number, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, get_string, arginfo_opencreport_result_get_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, get_number, arginfo_opencreport_result_get_number, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_part, get_next) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	if (!po->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Part object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = po->iter;
	ocrpt_part *p = ocrpt_part_get_next(po->o, &iter);

	if (!p)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_part_ce);
	php_opencreport_part_object *po1 = Z_OPENCREPORT_PART_P(return_value);

	po1->o = po->o;
	po1->p = p;
	po1->iter = iter;
	po1->is_iterator = true;
}

PHP_METHOD(opencreport_part, row_new) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_row_ce);
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(return_value);
	pro->pr = ocrpt_part_new_row(po->p);
}

PHP_METHOD(opencreport_part, row_get_first) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_part_row *pr = ocrpt_part_row_get_next(po->p, &iter);

	if (!pr)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_row_ce);
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(return_value);

	pro->p = po->p;
	pro->pr = pr;
	pro->iter = iter;
	pro->is_iterator = true;
}

PHP_METHOD(opencreport_part, add_iteration_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_part_add_iteration_cb(po->p, opencreport_part_cb, cb_name);
}

PHP_METHOD(opencreport_part, equals) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zval *part;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(part, opencreport_part_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_part_object *po1 = Z_OPENCREPORT_PART_P(part);

	RETURN_BOOL(po->p == po1->p);
}

PHP_METHOD(opencreport_part, set_iterations) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_iterations(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_font_name) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_font_name(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_font_size) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_font_size(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_paper) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_paper_by_name(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_orientation) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_orientation(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_top_margin) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_top_margin(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_bottom_margin) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_bottom_margin(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_left_margin) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_left_margin(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_right_margin) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_right_margin(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_suppress(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, set_suppress_pageheader_firstpage) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_set_suppress_pageheader_firstpage(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part, page_header) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_part_page_header(po->p);
}

PHP_METHOD(opencreport_part, page_header_set_report) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zval *report = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(report, opencreport_report_ce, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_layout_part_page_header_set_report(po->p, report ? Z_OPENCREPORT_REPORT_P(report)->r : NULL);
}

PHP_METHOD(opencreport_part, page_footer) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_part_page_footer(po->p);
}

PHP_METHOD(opencreport_part, page_footer_set_report) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zval *report = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(report, opencreport_report_ce, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_layout_part_page_footer_set_report(po->p, report ? Z_OPENCREPORT_REPORT_P(report)->r : NULL);
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_next, 0, 0, OpenCReport\\Part, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_new, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_get_first, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_equals, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, part, OpenCReport\\Part, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_iterations, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_font_name, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_font_size, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_paper, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_orientation, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_top_margin, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_bottom_margin, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_left_margin, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_right_margin, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_set_suppress_pageheader_firstpage, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_page_header, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_page_header_set_report, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, report, OpenCReport\\Report, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_page_footer, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_page_footer_set_report, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, report, OpenCReport\\Report, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_part_class_methods[] = {
	PHP_ME(opencreport_part, get_next, arginfo_opencreport_part_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, row_new, arginfo_opencreport_part_row_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, row_get_first, arginfo_opencreport_part_row_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, add_iteration_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, equals, arginfo_opencreport_part_equals, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_iterations, arginfo_opencreport_part_set_iterations, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_font_name, arginfo_opencreport_part_set_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_font_size, arginfo_opencreport_part_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_paper, arginfo_opencreport_part_set_paper, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_orientation, arginfo_opencreport_part_set_orientation, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_top_margin, arginfo_opencreport_part_set_top_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_bottom_margin, arginfo_opencreport_part_set_bottom_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_left_margin, arginfo_opencreport_part_set_left_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_right_margin, arginfo_opencreport_part_set_right_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_suppress, arginfo_opencreport_part_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_suppress_pageheader_firstpage, arginfo_opencreport_part_set_suppress_pageheader_firstpage, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, page_header, arginfo_opencreport_part_page_header, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, page_header_set_report, arginfo_opencreport_part_page_header_set_report, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, page_footer, arginfo_opencreport_part_page_footer, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, page_footer_set_report, arginfo_opencreport_part_page_footer_set_report, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_row, get_next) {
	zval *object = ZEND_THIS;
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	if (!pro->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Row object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = pro->iter;
	ocrpt_part_row *pr = ocrpt_part_row_get_next(pro->p, &iter);

	if (!pr)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_row_ce);
	php_opencreport_row_object *pro1 = Z_OPENCREPORT_ROW_P(return_value);

	pro1->p = pro->p;
	pro1->pr = pr;
	pro1->iter = iter;
	pro1->is_iterator = true;
}

PHP_METHOD(opencreport_row, column_new) {
	zval *object = ZEND_THIS;
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_col_ce);
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(return_value);
	pco->pc = ocrpt_part_row_new_column(pro->pr);
	pco->pr = NULL;
	pco->iter = NULL;
	pco->is_iterator = false;
}

PHP_METHOD(opencreport_row, column_get_first) {
	zval *object = ZEND_THIS;
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_part_column *pc = ocrpt_part_column_get_next(pro->pr, &iter);
	if (!pc)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_col_ce);
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(return_value);

	pco->pr = pro->pr;
	pco->pc = pc;
	pco->iter = iter;
	pco->is_iterator = true;
}

PHP_METHOD(opencreport_row, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_row_set_suppress(pro->pr, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_row, set_newpage) {
	zval *object = ZEND_THIS;
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_row_set_newpage(pro->pr, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_row, set_layout) {
	zval *object = ZEND_THIS;
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_row_set_layout(pro->pr, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_get_next, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_column_new, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_column_get_first, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_row_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_row_set_newpage, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_row_set_layout, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_row_class_methods[] = {
	PHP_ME(opencreport_row, get_next, arginfo_opencreport_row_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, column_new, arginfo_opencreport_row_column_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, column_get_first, arginfo_opencreport_row_column_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, set_suppress, arginfo_opencreport_row_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, set_newpage, arginfo_opencreport_row_set_newpage, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, set_layout, arginfo_opencreport_row_set_layout, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_part_col, get_next) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	if (!pco->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Column object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = pco->iter;
	ocrpt_part_column *pc = ocrpt_part_column_get_next(pco->pr, &iter);

	if (!pc)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_col_ce);
	php_opencreport_col_object *pco1 = Z_OPENCREPORT_COL_P(return_value);

	pco1->pr = pco->pr;
	pco1->pc = pc;
	pco1->iter = iter;
	pco1->is_iterator = true;
}

PHP_METHOD(opencreport_part_col, report_new) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_report *r = ocrpt_part_column_new_report(pco->pc);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(return_value);

	pro->pc = NULL;
	pro->r = r;
	pro->iter = NULL;
	pro->is_iterator = false;
}

PHP_METHOD(opencreport_part_col, report_get_first) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_report *r = ocrpt_report_get_next(pco->pc, &iter);

	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(return_value);

	pro->pc = pco->pc;
	pro->r = r;
	pro->iter = iter;
	pro->is_iterator = true;
}

PHP_METHOD(opencreport_part_col, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_column_set_suppress(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part_col, set_width) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_column_set_width(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part_col, set_height) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_column_set_height(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part_col, set_border_width) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_column_set_border_width(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part_col, set_border_color) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_column_set_border_color(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part_col, set_detail_columns) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_column_set_detail_columns(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_part_col, set_column_padding) {
	zval *object = ZEND_THIS;
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_part_column_set_column_padding(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_next, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_report_new, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_report_get_first, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_col_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_col_set_width, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_col_set_height, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_col_set_border_width, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_col_set_border_color, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_col_set_detail_columns, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_col_set_column_padding, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_part_col_class_methods[] = {
	PHP_ME(opencreport_part_col, get_next, arginfo_opencreport_part_col_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, report_new, arginfo_opencreport_part_col_report_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, report_get_first, arginfo_opencreport_part_col_report_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_suppress, arginfo_opencreport_part_col_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_width, arginfo_opencreport_part_col_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_height, arginfo_opencreport_part_col_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_border_width, arginfo_opencreport_part_col_set_border_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_border_color, arginfo_opencreport_part_col_set_border_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_detail_columns, arginfo_opencreport_part_col_set_detail_columns, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_column_padding, arginfo_opencreport_part_col_set_column_padding, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_report, get_next) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	if (!pro->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Report object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = pro->iter;
	ocrpt_report *r = ocrpt_report_get_next(pro->pc, &iter);

	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_report_ce);
	php_opencreport_report_object *pro1 = Z_OPENCREPORT_REPORT_P(return_value);

	pro1->pc = pro->pc;
	pro1->r = r;
	pro1->iter = iter;
	pro1->is_iterator = true;
}

PHP_METHOD(opencreport_report, variable_new) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_long variable_type;
	zend_string *name;
	zend_string *expr;
	zend_string *reset_on_break_name = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3,4)
		Z_PARAM_LONG(variable_type);
		Z_PARAM_STR(name);
		Z_PARAM_STR(expr);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(reset_on_break_name, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_var *v = ocrpt_variable_new(pro->r, variable_type, ZSTR_VAL(name), ZSTR_VAL(expr), reset_on_break_name ? ZSTR_VAL(reset_on_break_name) : NULL);

	if (!v)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_variable_ce);
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(return_value);
	vo->v = v;
}

PHP_METHOD(opencreport_report, variable_new_full) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_long result_type;
	zend_string *name;
	zend_string *baseexpr = NULL;
	zend_string *intermedexpr = NULL;
	zend_string *intermed2expr = NULL;
	zend_string *resultexpr = NULL;
	zend_string *reset_on_break_name = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 7)
		Z_PARAM_LONG(result_type);
		Z_PARAM_STR(name);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(baseexpr, 1, 0);
		Z_PARAM_STR_EX(intermedexpr, 1, 0);
		Z_PARAM_STR_EX(intermed2expr, 1, 0);
		Z_PARAM_STR_EX(resultexpr, 1, 0);
		Z_PARAM_STR_EX(reset_on_break_name, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_var *v = ocrpt_variable_new_full(pro->r, result_type, ZSTR_VAL(name),
												baseexpr ? ZSTR_VAL(baseexpr) : NULL,
												intermedexpr ? ZSTR_VAL(intermedexpr) : NULL,
												intermed2expr ? ZSTR_VAL(intermed2expr) : NULL,
												resultexpr ? ZSTR_VAL(resultexpr) : NULL,
												reset_on_break_name ? ZSTR_VAL(reset_on_break_name) : NULL);

	if (!v)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_variable_ce);
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(return_value);
	vo->v = v;
}

PHP_METHOD(opencreport_report, expr_parse) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *expr_string;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(expr_string);
	ZEND_PARSE_PARAMETERS_END();

	char *err = NULL;
	ocrpt_expr *e = ocrpt_report_expr_parse(pro->r, ZSTR_VAL(expr_string), &err);

	if (e) {
		object_init_ex(return_value, opencreport_expr_ce);
		php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
		eo->e = e;
		ocrpt_strfree(pro->expr_error);
		pro->expr_error = NULL;
	} else {
		pro->expr_error = err;
		RETURN_NULL();
	}
}

PHP_METHOD(opencreport_report, expr_error) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (pro->expr_error)
		RETURN_STRING(pro->expr_error);
	else
		RETURN_NULL();
}

PHP_METHOD(opencreport_report, resolve_variables) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_report_resolve_variables(pro->r);
}

PHP_METHOD(opencreport_report, evaluate_variables) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_report_evaluate_variables(pro->r);
}

PHP_METHOD(opencreport_report, break_new) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(name);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_break *br = ocrpt_break_new(pro->r, ZSTR_VAL(name));
	if (!br)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(return_value);
	bro->br = br;
}

PHP_METHOD(opencreport_report, break_get_first) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_break *br = ocrpt_break_get_next(pro->r, &iter);
	if (!br)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(return_value);
	bro->r = pro->r;
	bro->br = br;
	bro->iter = iter;
	bro->is_iterator = true;
}

PHP_METHOD(opencreport_report, resolve_breaks) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_report_resolve_breaks(pro->r);
}

PHP_METHOD(opencreport_report, get_query_rownum) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_report_get_query_rownum(pro->r));
}

PHP_METHOD(opencreport_report, add_start_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_report_add_start_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_done_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_report_add_done_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_new_row_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_report_add_new_row_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_iteration_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_report_add_iteration_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_precalculation_done_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_report_add_precalculation_done_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, break_get) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *break_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(break_name);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_break *br = ocrpt_break_get(pro->r, ZSTR_VAL(break_name));

	if (!br)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(return_value);

	bro->r = pro->r;
	bro->br = br;
	bro->iter = NULL;
	bro->is_iterator = false;
}

PHP_METHOD(opencreport_report, equals) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zval *report;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(report, opencreport_report_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_report_object *pro1 = Z_OPENCREPORT_REPORT_P(report);

	RETURN_BOOL(pro->r == pro1->r);
}

PHP_METHOD(opencreport_report, set_main_query) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zval *query;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(query, opencreport_query_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(query);

	ocrpt_report_set_main_query(pro->r, qo->q);
}

PHP_METHOD(opencreport_report, set_main_query_by_name) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *query_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(query_name);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_report_set_main_query_from_expr(pro->r, ZSTR_VAL(query_name));
}

PHP_METHOD(opencreport_report, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_report_set_suppress(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_report, set_iterations) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_report_set_iterations(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_report, set_font_name) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_report_set_font_name(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_report, set_font_size) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_report_set_font_size(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_report, set_height) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_report_set_height(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_report, set_fieldheader_priority) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_report_set_fieldheader_priority(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_report, nodata) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_nodata(ro->r);
}

PHP_METHOD(opencreport_report, header) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_header(ro->r);
}

PHP_METHOD(opencreport_report, footer) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_footer(ro->r);
}

PHP_METHOD(opencreport_report, field_header) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_field_header(ro->r);
}

PHP_METHOD(opencreport_report, field_details) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_field_details(ro->r);
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_get_next, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_new, 0, 3, OpenCReport\\Variable, 1)
ZEND_ARG_TYPE_INFO(0, variable_type, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, expr, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, reset_on_break_name, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_new_full, 0, 2, OpenCReport\\Variable, 1)
ZEND_ARG_TYPE_INFO(0, result_type, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, baseexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, intermedexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, intermed2expr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, resultexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, reset_on_break_name, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_expr_parse, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_expr_error, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_resolve_variables, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_evaluate_variables, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_new, 0, 1, OpenCReport\\Break, 1)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_get_first, 0, 0, OpenCReport\\Break, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_resolve_breaks, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_get_query_rownum, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_get, 0, 1, OpenCReport\\Break, 1)
ZEND_ARG_TYPE_INFO(0, break_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_equals, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, report, OpenCReport\\Report, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_main_query, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, query, OpenCReport\\Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_main_query_by_name, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, query_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_iterations, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_font_name, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_font_size, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_height, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_fieldheader_priority, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_nodata, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_header, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_footer, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_field_header, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_field_details, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_report_class_methods[] = {
	PHP_ME(opencreport_report, get_next, arginfo_opencreport_report_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_main_query, arginfo_opencreport_report_set_main_query, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_main_query_by_name, arginfo_opencreport_report_set_main_query_by_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, variable_new, arginfo_opencreport_variable_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, variable_new_full, arginfo_opencreport_variable_new_full, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, expr_parse, arginfo_opencreport_report_expr_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, expr_error, arginfo_opencreport_report_expr_error, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, resolve_variables, arginfo_opencreport_report_resolve_variables, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, evaluate_variables, arginfo_opencreport_report_evaluate_variables, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_new, arginfo_opencreport_break_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_get_first, arginfo_opencreport_break_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, resolve_breaks, arginfo_opencreport_report_resolve_breaks, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_query_rownum, arginfo_opencreport_report_get_query_rownum, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_start_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_new_row_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_iteration_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_precalculation_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_get, arginfo_opencreport_break_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, equals, arginfo_opencreport_report_equals, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_suppress, arginfo_opencreport_report_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_iterations, arginfo_opencreport_report_set_iterations, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_font_name, arginfo_opencreport_report_set_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_font_size, arginfo_opencreport_report_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_height, arginfo_opencreport_report_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_fieldheader_priority, arginfo_opencreport_report_set_fieldheader_priority, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, nodata, arginfo_opencreport_report_nodata, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, header, arginfo_opencreport_report_header, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, footer, arginfo_opencreport_report_footer, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, field_header, arginfo_opencreport_report_field_header, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, field_details, arginfo_opencreport_report_field_details, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_variable, baseexpr) {
	zval *object = ZEND_THIS;
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_baseexpr(vo->v);

	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_variable, intermedexpr) {
	zval *object = ZEND_THIS;
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_intermedexpr(vo->v);

	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_variable, intermed2expr) {
	zval *object = ZEND_THIS;
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_intermed2expr(vo->v);

	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_variable, resultexpr) {
	zval *object = ZEND_THIS;
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_resultexpr(vo->v);

	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_variable, set_precalculate) {
	zval *object = ZEND_THIS;
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_variable_set_precalculate(vo->v, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_variable, resolve) {
	zval *object = ZEND_THIS;
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_variable_resolve(vo->v);
}

PHP_METHOD(opencreport_variable, eval) {
	zval *object = ZEND_THIS;
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_variable_evaluate(vo->v);
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_baseexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_intermedexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_intermed2expr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_resultexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_set_precalculate, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_resolve, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_eval, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_variable_class_methods[] = {
	PHP_ME(opencreport_variable, baseexpr, arginfo_opencreport_variable_baseexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, intermedexpr, arginfo_opencreport_variable_intermedexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, intermed2expr, arginfo_opencreport_variable_intermed2expr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, resultexpr, arginfo_opencreport_variable_resultexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, set_precalculate, arginfo_opencreport_variable_set_precalculate, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, resolve, arginfo_opencreport_variable_resolve, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, eval, arginfo_opencreport_variable_eval, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_break, get_next) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	if (!bro->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Break object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = bro->iter;
	ocrpt_break *br = ocrpt_break_get_next(bro->r, &iter);
	if (!br)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_break_ce);
	php_opencreport_break_object *bro1 = Z_OPENCREPORT_BREAK_P(return_value);
	bro1->r = bro->r;
	bro1->br = br;
	bro1->iter = iter;
	bro1->is_iterator = true;
}

PHP_METHOD(opencreport_break, breakfield_add) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);
	zval *breakfield_expr;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(breakfield_expr, opencreport_expr_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(breakfield_expr);

	bool retval = ocrpt_break_add_breakfield(bro->br, eo->e);

	/* The expression's ownership was taken over or it was freed */
	eo->e = NULL;

	RETURN_BOOL(retval);
}

PHP_METHOD(opencreport_break, check_fields) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_break_check_fields(bro->br));
}

PHP_METHOD(opencreport_break, reset_vars) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_break_reset_vars(bro->br);
}

static void opencreport_break_cb(opencreport *o, ocrpt_report *r, ocrpt_break *br, void *data) {
	zend_string *callback = data;
	zval zfname;

	ZVAL_STRINGL(&zfname, ZSTR_VAL(callback), ZSTR_LEN(callback));

	zval retval;
	zval params[3];

	object_init_ex(&params[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(&params[0]);
	oo->o = o;

	object_init_ex(&params[1], opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(&params[1]);
	pro->r = r;

	object_init_ex(&params[2], opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(&params[2]);
	bro->r = r;
	bro->br = br;

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, 3, params) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 3, params, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_object_std_dtor(&oo->zo);
	OBJ_RELEASE(&oo->zo);
	zend_object_std_dtor(&pro->zo);
	OBJ_RELEASE(&pro->zo);
	zend_object_std_dtor(&bro->zo);
	OBJ_RELEASE(&bro->zo);

	zend_string_release(Z_STR(zfname));
}

PHP_METHOD(opencreport_break, add_trigger_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	zend_string *cb_name = zend_string_init(ZSTR_VAL(callback), ZSTR_LEN(callback), false);

	ocrpt_break_add_trigger_cb(bro->br, opencreport_break_cb, cb_name);
}

PHP_METHOD(opencreport_break, name) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_STRING(ocrpt_break_get_name(bro->br));
}

PHP_METHOD(opencreport_break, header) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_break_get_header(bro->br);
}

PHP_METHOD(opencreport_break, footer) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_break_get_footer(bro->br);
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_get_next, 0, 0, OpenCReport\\Break, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_break_breakfield_add, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, breakfield_expr, OpenCReport\\Expr, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_break_check_fields, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_break_reset_vars, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_break_name, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_header, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_footer, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_break_class_methods[] = {
	PHP_ME(opencreport_break, get_next, arginfo_opencreport_break_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, breakfield_add, arginfo_opencreport_break_breakfield_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, check_fields, arginfo_opencreport_break_check_fields, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, reset_vars, arginfo_opencreport_break_reset_vars, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, add_trigger_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, name, arginfo_opencreport_break_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, header, arginfo_opencreport_break_header, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, footer, arginfo_opencreport_break_footer, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_output, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_output_set_suppress(oo->output, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_output, add_line) {
	zval *object = ZEND_THIS;
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	ocrpt_line *l;
	php_opencreport_line_object *lo;

	ZEND_PARSE_PARAMETERS_NONE();

	l = ocrpt_output_add_line(oo->output);
	if (!l)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_line_ce);
	lo = Z_OPENCREPORT_LINE_P(return_value);
	lo->line = l;
}

PHP_METHOD(opencreport_output, add_hline) {
	zval *object = ZEND_THIS;
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	ocrpt_hline *hl;
	php_opencreport_hline_object *hlo;

	ZEND_PARSE_PARAMETERS_NONE();

	hl = ocrpt_output_add_hline(oo->output);
	if (!hl)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_hline_ce);
	hlo = Z_OPENCREPORT_HLINE_P(return_value);
	hlo->hline = hl;
}

PHP_METHOD(opencreport_output, add_image) {
	zval *object = ZEND_THIS;
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	ocrpt_image *im;
	php_opencreport_image_object *imo;

	ZEND_PARSE_PARAMETERS_NONE();

	im = ocrpt_output_add_image(oo->output);
	if (!im)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_image_ce);
	imo = Z_OPENCREPORT_IMAGE_P(return_value);
	imo->image = im;
}

PHP_METHOD(opencreport_output, add_image_end) {
	zval *object = ZEND_THIS;
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_output_add_image_end(oo->output);
}

PHP_METHOD(opencreport_output, add_barcode) {
	zval *object = ZEND_THIS;
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	ocrpt_barcode *bc;
	php_opencreport_barcode_object *bco;

	ZEND_PARSE_PARAMETERS_NONE();

	bc = ocrpt_output_add_barcode(oo->output);
	if (!bc)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_barcode_ce);
	bco = Z_OPENCREPORT_BARCODE_P(return_value);
	bco->bc = bc;
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_output_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_add_line, 0, 0, OpenCReport\\Line, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_add_hline, 0, 0, OpenCReport\\HorizontalLine, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_add_image, 0, 0, OpenCReport\\Image, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_output_add_image_end, 0, 0, IS_VOID, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_output_class_methods[] = {
	PHP_ME(opencreport_output, set_suppress, arginfo_opencreport_output_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_line, arginfo_opencreport_output_add_line, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_hline, arginfo_opencreport_output_add_hline, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_image, arginfo_opencreport_output_add_image, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_image_end, arginfo_opencreport_output_add_image_end, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_line, set_font_name) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_line_set_font_name(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_line, set_font_size) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_line_set_font_size(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_line, set_bold) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_line_set_bold(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_line, set_italic) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_line_set_italic(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_line, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_line_set_suppress(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_line, set_color) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_line_set_color(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_line, set_bgcolor) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_line_set_bgcolor(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_line, add_text) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	ocrpt_text *t;
	php_opencreport_text_object *to;

	ZEND_PARSE_PARAMETERS_NONE();

	t = ocrpt_line_add_text(lo->line);
	if (!t)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_text_ce);
	to = Z_OPENCREPORT_TEXT_P(return_value);
	to->text = t;
}

PHP_METHOD(opencreport_line, add_image) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	ocrpt_image *im;
	php_opencreport_image_object *imo;

	ZEND_PARSE_PARAMETERS_NONE();

	im = ocrpt_line_add_image(lo->line);
	if (!im)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_image_ce);
	imo = Z_OPENCREPORT_IMAGE_P(return_value);
	imo->image = im;
}

PHP_METHOD(opencreport_line, add_barcode) {
	zval *object = ZEND_THIS;
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
	ocrpt_barcode *bc;
	php_opencreport_barcode_object *bco;

	ZEND_PARSE_PARAMETERS_NONE();

	bc = ocrpt_line_add_barcode(lo->line);
	if (!bc)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_barcode_ce);
	bco = Z_OPENCREPORT_BARCODE_P(return_value);
	bco->bc = bc;
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_set_font_name, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_set_font_size, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_set_bold, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_set_italic, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_set_color, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_set_bgcolor, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_add_text, 0, 0, OpenCReport\\Text, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_add_image, 0, 0, OpenCReport\\Image, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_add_barcode, 0, 0, OpenCReport\\Barcode, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_line_class_methods[] = {
	PHP_ME(opencreport_line, set_font_name, arginfo_opencreport_line_set_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_font_size, arginfo_opencreport_line_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_bold, arginfo_opencreport_line_set_bold, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_italic, arginfo_opencreport_line_set_italic, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_suppress, arginfo_opencreport_line_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_color, arginfo_opencreport_line_set_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_bgcolor, arginfo_opencreport_line_set_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, add_text, arginfo_opencreport_line_add_text, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, add_image, arginfo_opencreport_line_add_image, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, add_barcode, arginfo_opencreport_line_add_barcode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_hline, set_size) {
	zval *object = ZEND_THIS;
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_hline_set_size(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_hline, set_indent) {
	zval *object = ZEND_THIS;
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_hline_set_indent(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_hline, set_length) {
	zval *object = ZEND_THIS;
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_hline_set_length(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_hline, set_font_size) {
	zval *object = ZEND_THIS;
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_hline_set_font_size(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_hline, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_hline_set_suppress(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_hline, set_color) {
	zval *object = ZEND_THIS;
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_hline_set_color(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_hline_set_size, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_hline_set_indent, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_hline_set_length, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_hline_set_font_size, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_hline_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_hline_set_color, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_hline_class_methods[] = {
	PHP_ME(opencreport_hline, set_size, arginfo_opencreport_hline_set_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_indent, arginfo_opencreport_hline_set_indent, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_length, arginfo_opencreport_hline_set_length, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_font_size, arginfo_opencreport_hline_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_suppress, arginfo_opencreport_hline_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_color, arginfo_opencreport_hline_set_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_image, set_value) {
	zval *object = ZEND_THIS;
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_image_set_value(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_image, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_image_set_suppress(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_image, set_type) {
	zval *object = ZEND_THIS;
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_image_set_type(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_image, set_width) {
	zval *object = ZEND_THIS;
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_image_set_width(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_image, set_height) {
	zval *object = ZEND_THIS;
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_image_set_height(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_image, set_alignment) {
	zval *object = ZEND_THIS;
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_image_set_alignment(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_image, set_bgcolor) {
	zval *object = ZEND_THIS;
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_image_set_bgcolor(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_image, set_text_width) {
	zval *object = ZEND_THIS;
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_image_set_text_width(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_image_set_value, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_image_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_image_set_type, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_image_set_width, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_image_set_height, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_image_set_alignment, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_image_set_bgcolor, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_image_set_text_width, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_image_class_methods[] = {
	PHP_ME(opencreport_image, set_value, arginfo_opencreport_image_set_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_suppress, arginfo_opencreport_image_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_type, arginfo_opencreport_image_set_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_width, arginfo_opencreport_image_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_height, arginfo_opencreport_image_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_alignment, arginfo_opencreport_image_set_alignment, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_bgcolor, arginfo_opencreport_image_set_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_text_width, arginfo_opencreport_image_set_text_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_text, set_value_string) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_value_string(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_value_expr) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_value_expr(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_value_delayed) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_value_delayed(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_format) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_format(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_translate) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_translate(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_width) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_width(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_alignment) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_alignment(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_color) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_color(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_bgcolor) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_bgcolor(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_font_name) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_font_name(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_font_size) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_font_size(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_bold) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_bold(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_italic) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_italic(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_link) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_link(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_memo) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_memo(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_memo_wrap_chars) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_memo_wrap_chars(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_text, set_memo_max_lines) {
	zval *object = ZEND_THIS;
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_text_set_memo_max_lines(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_value_string, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_value_expr, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_value_delayed, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_format, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_translate, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_width, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_alignment, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_color, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_bgcolor, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_font_name, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_font_size, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_bold, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_italic, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_link, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_memo, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_memo_wrap_chars, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_text_set_memo_max_lines, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_text_class_methods[] = {
	PHP_ME(opencreport_text, set_value_string, arginfo_opencreport_text_set_value_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_value_expr, arginfo_opencreport_text_set_value_expr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_value_delayed, arginfo_opencreport_text_set_value_delayed, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_format, arginfo_opencreport_text_set_format, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_translate, arginfo_opencreport_text_set_translate, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_width, arginfo_opencreport_text_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_alignment, arginfo_opencreport_text_set_alignment, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_color, arginfo_opencreport_text_set_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_bgcolor, arginfo_opencreport_text_set_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_font_name, arginfo_opencreport_text_set_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_font_size, arginfo_opencreport_text_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_bold, arginfo_opencreport_text_set_bold, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_italic, arginfo_opencreport_text_set_italic, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_link, arginfo_opencreport_text_set_link, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_memo, arginfo_opencreport_text_set_memo, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_memo_wrap_chars, arginfo_opencreport_text_set_memo_wrap_chars, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_memo_max_lines, arginfo_opencreport_text_set_memo_max_lines, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_barcode, set_value) {
	zval *object = ZEND_THIS;
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_barcode_set_value(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_barcode, set_value_delayed) {
	zval *object = ZEND_THIS;
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_barcode_set_value_delayed(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_barcode, set_suppress) {
	zval *object = ZEND_THIS;
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_barcode_set_suppress(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_barcode, set_type) {
	zval *object = ZEND_THIS;
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_barcode_set_type(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_barcode, set_width) {
	zval *object = ZEND_THIS;
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_barcode_set_width(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_barcode, set_height) {
	zval *object = ZEND_THIS;
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_barcode_set_height(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_barcode, set_color) {
	zval *object = ZEND_THIS;
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_barcode_set_color(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_barcode, set_bgcolor) {
	zval *object = ZEND_THIS;
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_barcode_set_bgcolor(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_barcode_set_value, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_barcode_set_value_delayed, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_barcode_set_suppress, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_barcode_set_type, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_barcode_set_width, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_barcode_set_height, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_barcode_set_color, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_barcode_set_bgcolor, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_barcode_class_methods[] = {
	PHP_ME(opencreport_barcode, set_value, arginfo_opencreport_barcode_set_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_value_delayed, arginfo_opencreport_barcode_set_value_delayed, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_suppress, arginfo_opencreport_barcode_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_type, arginfo_opencreport_barcode_set_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_width, arginfo_opencreport_barcode_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_height, arginfo_opencreport_barcode_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_color, arginfo_opencreport_barcode_set_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_bgcolor, arginfo_opencreport_barcode_set_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

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

static zval *php_opencreport_get_zval_of_type_raw(zval *zv, int expected_type) {
	zv = php_opencreport_get_zval_direct(zv);

	return (zv && (Z_TYPE_P(zv) == expected_type)) ? zv : NULL;
}

static zval *php_opencreport_get_zval_of_type(const char *name, int expected_type) {
	if (!name || !*name)
		return NULL;

	zval *zv = zend_hash_str_find(&EG(symbol_table), name, strlen(name));

	return php_opencreport_get_zval_of_type_raw(zv, expected_type);
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
		zval *cell = zend_hash_get_current_data_ex(htab1, &htab1p);
		int32_t data_result;

		if (Z_TYPE_P(cell) == IS_LONG) {
			zend_long l = Z_LVAL_P(cell);
			if (l >= OCRPT_RESULT_ERROR && l <= OCRPT_RESULT_DATETIME)
				data_result = l;
			else
				data_result = OCRPT_RESULT_STRING;
		} else if (Z_TYPE_P(cell) == IS_STRING) {
			char *s = Z_STRVAL_P(cell);

			if (strcasecmp(s, "number") == 0 || strcasecmp(s, "numeric") == 0)
				data_result = OCRPT_RESULT_NUMBER;
			else if (strcasecmp(s, "datetime") == 0)
				data_result = OCRPT_RESULT_DATETIME;
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
	zval *array;
	ocrpt_query_result *result;
	ocrpt_string *converted;
	HashTable *ahash;
	HashPosition ahashpos;
	int32_t cols;
	bool atstart:1;
	bool isdone:1;
};

static bool php_opencreport_array_connect(ocrpt_datasource *ds, const ocrpt_input_connect_parameter *conn_params) {
	ocrpt_datasource_set_private(ds, (iconv_t)-1);
	return true;
}

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
	zval *row = zend_hash_get_current_data_ex(ahash, &ahashpos);
	row = php_opencreport_get_zval_of_type_raw(row, IS_ARRAY);

	if (!row)
		return NULL;

	HashTable *rowhash = Z_ARRVAL_P(row);
	HashPosition rowhashpos;
	int32_t a_cols = 0, array_cols = zend_hash_num_elements(rowhash);
	zval *cell = NULL;

	zend_hash_internal_pointer_reset_ex(rowhash, &rowhashpos);
	for (a_cols = 0; a_cols < array_cols; a_cols++, zend_hash_move_forward_ex(rowhash, &rowhashpos)) {
		cell = zend_hash_get_current_data_ex(rowhash, &rowhashpos);
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

	memset(result, 0, sizeof(struct php_opencreport_array_results));
	result->array_name = ocrpt_mem_strdup(array_name);
	result->types_name = ocrpt_mem_strdup(types_name);
	result->array = array;
	result->ahash = ahash;
	zend_hash_internal_pointer_reset_ex(result->ahash, &result->ahashpos);
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

		result->ahash = Z_ARRVAL_P(result->array);

		zend_hash_internal_pointer_reset_ex(result->ahash, &result->ahashpos);
		zval *row = zend_hash_get_current_data_ex(result->ahash, &result->ahashpos);
		row = php_opencreport_get_zval_of_type_raw(row, IS_ARRAY);

		if (!row)
			goto out_error;

		HashTable *rowhash = Z_ARRVAL_P(row);
		HashPosition rowhashpos;
		int32_t i;

		if (result->cols != zend_hash_num_elements(rowhash))
			goto out_error;

		for (i = 0, zend_hash_internal_pointer_reset_ex(rowhash, &rowhashpos); i < result->cols; i++, zend_hash_move_forward_ex(rowhash, &rowhashpos)) {
			zval *cell = zend_hash_get_current_data_ex(rowhash, &rowhashpos);
			cell = php_opencreport_get_zval_direct(cell);

			char *data_result;
			zval copy;

			ZVAL_STR(&copy, zval_get_string_func(cell));
			data_result = Z_STRVAL(copy);

			for (int32_t j = 0; j < OCRPT_EXPR_RESULTS; j++) {
				qr[j * result->cols + i].result.o = o;
				qr[j * result->cols + i].name = (j == 0) ? ocrpt_mem_strdup(data_result) : qr[i].name;
				qr[j * result->cols + i].name_allocated = (j == 0);
			}
		}

		i = 0;

		zval *types = (result->types_name && *result->types_name) ? php_opencreport_get_zval_of_type(result->types_name, IS_ARRAY) : NULL;
		if (types) {
			HashTable *typeshash = Z_ARRVAL_P(types);
			HashPosition typeshashpos;
			int types_cols = zend_hash_num_elements(typeshash);

			for (zend_hash_internal_pointer_reset_ex(typeshash, &typeshashpos); i < types_cols && i < result->cols; i++, zend_hash_move_forward_ex(typeshash, &typeshashpos)) {
				zval *cell = zend_hash_get_current_data_ex(typeshash, &typeshashpos);
				cell = php_opencreport_get_zval_of_type_raw(cell, IS_LONG);

				enum ocrpt_result_type type = OCRPT_RESULT_ERROR;
				if (cell) {
					zend_long l = Z_LVAL_P(cell);

					if (l >= OCRPT_RESULT_ERROR && l <= OCRPT_RESULT_DATETIME)
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

	result->array = array;
	result->ahash = Z_ARRVAL_P(array);
	zend_hash_internal_pointer_reset_ex(result->ahash, &result->ahashpos);

	return true;
}

static void php_opencreport_array_rewind(ocrpt_query *query) {
	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);

	if (result == NULL)
		return;

	result->ahash = Z_ARRVAL_P(result->array);
	zend_hash_internal_pointer_reset_ex(result->ahash, &result->ahashpos);

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

	zval *row = zend_hash_get_current_data_ex(result->ahash, &result->ahashpos);
	row = php_opencreport_get_zval_of_type_raw(row, IS_ARRAY);

	HashTable *rowhash = Z_ARRVAL_P(row);
	HashPosition rowhashpos;
	int32_t i;

	for (i = 0, zend_hash_internal_pointer_reset_ex(rowhash, &rowhashpos); i < result->cols; i++, zend_hash_move_forward_ex(rowhash, &rowhashpos)) {
		zval *cell = zend_hash_get_current_data_ex(rowhash, &rowhashpos);
		cell = php_opencreport_get_zval_direct(cell);

		char *str = NULL;
		int32_t len = 0;

		if (cell && Z_TYPE_P(cell) != IS_NULL) {
			zval copy;

			ZVAL_STR(&copy, zval_get_string_func(cell));
			str = Z_STRVAL(copy);
			len = Z_STRLEN(copy);
		}

		ocrpt_query_result_set_value(query, i, (str == NULL), ocrpt_datasource_get_private(source), str, len);
	}

	return true;
}

static bool php_opencreport_array_next(ocrpt_query *query) {
	struct php_opencreport_array_results *result = ocrpt_query_get_private(query);

	if (result == NULL)
		return false;

	result->atstart = false;
	zend_result newrow = zend_hash_move_forward_ex(result->ahash, &result->ahashpos);
	result->isdone = (newrow != SUCCESS || result->ahashpos >= zend_hash_num_elements(result->ahash));

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

/* {{{ PHP_MINIT_FUNCTION */
static PHP_MINIT_FUNCTION(opencreport)
{
	zend_class_entry ce;

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
		return FAILURE;

	ocrpt_query_set_discover_func(php_opencreport_query_discover_array);
	ocrpt_env_set_query_func(php_opencreport_env_query);
	ocrpt_set_printf_func(php_opencreport_std_printf);
	ocrpt_set_err_printf_func(php_opencreport_err_printf);
	ocrpt_mem_set_alloc_funcs(opencreport_emalloc, opencreport_erealloc, NULL, opencreport_efree, opencreport_estrdup, opencreport_estrndup);

	memcpy(&opencreport_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_CLASS_ENTRY(ce, "OpenCReport", opencreport_class_methods);
	ce.create_object = opencreport_object_new;
	opencreport_object_handlers.offset = XtOffsetOf(php_opencreport_object, zo);
	opencreport_object_handlers.clone_obj = NULL;
	opencreport_object_handlers.free_obj = opencreport_object_free;
	opencreport_object_handlers.compare = zend_objects_not_comparable;
	opencreport_ce = zend_register_internal_class(&ce);
#if PHP_VERSION_ID >= 80100
	opencreport_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_ce->serialize = zend_class_serialize_deny;
	opencreport_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_ds_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Datasource", opencreport_ds_class_methods);
	ce.create_object = opencreport_ds_object_new;
	opencreport_ds_object_handlers.offset = XtOffsetOf(php_opencreport_ds_object, zo);
	opencreport_ds_object_handlers.clone_obj = NULL;
	opencreport_ds_object_handlers.compare = zend_objects_not_comparable;
	opencreport_ds_ce = zend_register_internal_class(&ce);
	opencreport_ds_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_ds_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_ds_ce->serialize = zend_class_serialize_deny;
	opencreport_ds_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_query_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Query", opencreport_query_class_methods);
	ce.create_object = opencreport_query_object_new;
	opencreport_query_object_handlers.offset = XtOffsetOf(php_opencreport_query_object, zo);
	opencreport_query_object_handlers.clone_obj = NULL;
	opencreport_query_object_handlers.compare = zend_objects_not_comparable;
	opencreport_query_ce = zend_register_internal_class(&ce);
	opencreport_query_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_query_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_query_ce->serialize = zend_class_serialize_deny;
	opencreport_query_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_query_result_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "QueryResult", opencreport_query_result_class_methods);
	ce.create_object = opencreport_query_result_object_new;
	opencreport_query_result_object_handlers.offset = XtOffsetOf(php_opencreport_query_result_object, zo);
	opencreport_query_result_object_handlers.clone_obj = NULL;
	opencreport_query_result_object_handlers.compare = zend_objects_not_comparable;
	opencreport_query_result_ce = zend_register_internal_class(&ce);
	opencreport_query_result_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_query_result_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_query_result_ce->serialize = zend_class_serialize_deny;
	opencreport_query_result_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_expr_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Expr", opencreport_expr_class_methods);
	ce.create_object = opencreport_expr_object_new;
	opencreport_expr_object_handlers.offset = XtOffsetOf(php_opencreport_expr_object, zo);
	opencreport_expr_object_handlers.clone_obj = NULL;
	opencreport_expr_object_handlers.compare = zend_objects_not_comparable;
	opencreport_expr_ce = zend_register_internal_class(&ce);
	opencreport_expr_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_expr_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_expr_ce->serialize = zend_class_serialize_deny;
	opencreport_expr_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_result_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Result", opencreport_result_class_methods);
	ce.create_object = opencreport_result_object_new;
	opencreport_result_object_handlers.offset = XtOffsetOf(php_opencreport_result_object, zo);
	opencreport_result_object_handlers.clone_obj = NULL;
	opencreport_result_object_handlers.free_obj = opencreport_result_object_free;
	opencreport_result_object_handlers.compare = zend_objects_not_comparable;
	opencreport_result_ce = zend_register_internal_class(&ce);
	opencreport_result_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_result_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_result_ce->serialize = zend_class_serialize_deny;
	opencreport_result_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_part_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Part", opencreport_part_class_methods);
	ce.create_object = opencreport_part_object_new;
	opencreport_part_object_handlers.offset = XtOffsetOf(php_opencreport_part_object, zo);
	opencreport_part_object_handlers.clone_obj = NULL;
	opencreport_part_object_handlers.compare = zend_objects_not_comparable;
	opencreport_part_ce = zend_register_internal_class(&ce);
	opencreport_part_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_part_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_part_ce->serialize = zend_class_serialize_deny;
	opencreport_part_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_row_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Row", opencreport_row_class_methods);
	ce.create_object = opencreport_row_object_new;
	opencreport_row_object_handlers.offset = XtOffsetOf(php_opencreport_row_object, zo);
	opencreport_row_object_handlers.clone_obj = NULL;
	opencreport_row_object_handlers.compare = zend_objects_not_comparable;
	opencreport_row_ce = zend_register_internal_class(&ce);
	opencreport_row_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_row_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_row_ce->serialize = zend_class_serialize_deny;
	opencreport_row_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_col_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Column", opencreport_part_col_class_methods);
	ce.create_object = opencreport_col_object_new;
	opencreport_col_object_handlers.offset = XtOffsetOf(php_opencreport_col_object, zo);
	opencreport_col_object_handlers.clone_obj = NULL;
	opencreport_col_object_handlers.compare = zend_objects_not_comparable;
	opencreport_col_ce = zend_register_internal_class(&ce);
	opencreport_col_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_col_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_col_ce->serialize = zend_class_serialize_deny;
	opencreport_col_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_report_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Report", opencreport_report_class_methods);
	ce.create_object = opencreport_report_object_new;
	opencreport_report_object_handlers.offset = XtOffsetOf(php_opencreport_report_object, zo);
	opencreport_report_object_handlers.clone_obj = NULL;
	opencreport_report_object_handlers.free_obj = opencreport_report_object_free;
	opencreport_report_object_handlers.compare = zend_objects_not_comparable;
	opencreport_report_ce = zend_register_internal_class(&ce);
	opencreport_report_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_report_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_report_ce->serialize = zend_class_serialize_deny;
	opencreport_report_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_variable_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Variable", opencreport_variable_class_methods);
	ce.create_object = opencreport_variable_object_new;
	opencreport_variable_object_handlers.offset = XtOffsetOf(php_opencreport_variable_object, zo);
	opencreport_variable_object_handlers.clone_obj = NULL;
	opencreport_variable_object_handlers.compare = zend_objects_not_comparable;
	opencreport_variable_ce = zend_register_internal_class(&ce);
	opencreport_variable_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_variable_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_variable_ce->serialize = zend_class_serialize_deny;
	opencreport_variable_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_break_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Break", opencreport_break_class_methods);
	ce.create_object = opencreport_break_object_new;
	opencreport_break_object_handlers.offset = XtOffsetOf(php_opencreport_break_object, zo);
	opencreport_break_object_handlers.clone_obj = NULL;
	opencreport_break_object_handlers.compare = zend_objects_not_comparable;
	opencreport_break_ce = zend_register_internal_class(&ce);
	opencreport_break_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_break_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_break_ce->serialize = zend_class_serialize_deny;
	opencreport_break_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_output_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Output", opencreport_output_class_methods);
	ce.create_object = opencreport_output_object_new;
	opencreport_output_object_handlers.offset = XtOffsetOf(php_opencreport_output_object, zo);
	opencreport_output_object_handlers.clone_obj = NULL;
	opencreport_output_object_handlers.compare = zend_objects_not_comparable;
	opencreport_output_ce = zend_register_internal_class(&ce);
	opencreport_output_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_output_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_output_ce->serialize = zend_class_serialize_deny;
	opencreport_output_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_line_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Line", opencreport_line_class_methods);
	ce.create_object = opencreport_line_object_new;
	opencreport_line_object_handlers.offset = XtOffsetOf(php_opencreport_line_object, zo);
	opencreport_line_object_handlers.clone_obj = NULL;
	opencreport_line_object_handlers.compare = zend_objects_not_comparable;
	opencreport_line_ce = zend_register_internal_class(&ce);
	opencreport_line_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_line_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_line_ce->serialize = zend_class_serialize_deny;
	opencreport_line_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_hline_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "HorizontalLine", opencreport_hline_class_methods);
	ce.create_object = opencreport_hline_object_new;
	opencreport_hline_object_handlers.offset = XtOffsetOf(php_opencreport_hline_object, zo);
	opencreport_hline_object_handlers.clone_obj = NULL;
	opencreport_hline_object_handlers.compare = zend_objects_not_comparable;
	opencreport_hline_ce = zend_register_internal_class(&ce);
	opencreport_hline_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_hline_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_hline_ce->serialize = zend_class_serialize_deny;
	opencreport_hline_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_image_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Image", opencreport_image_class_methods);
	ce.create_object = opencreport_image_object_new;
	opencreport_image_object_handlers.offset = XtOffsetOf(php_opencreport_image_object, zo);
	opencreport_image_object_handlers.clone_obj = NULL;
	opencreport_image_object_handlers.compare = zend_objects_not_comparable;
	opencreport_image_ce = zend_register_internal_class(&ce);
	opencreport_image_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_image_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_image_ce->serialize = zend_class_serialize_deny;
	opencreport_image_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_text_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Text", opencreport_text_class_methods);
	ce.create_object = opencreport_text_object_new;
	opencreport_text_object_handlers.offset = XtOffsetOf(php_opencreport_text_object, zo);
	opencreport_text_object_handlers.clone_obj = NULL;
	opencreport_text_object_handlers.compare = zend_objects_not_comparable;
	opencreport_text_ce = zend_register_internal_class(&ce);
	opencreport_text_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_text_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_text_ce->serialize = zend_class_serialize_deny;
	opencreport_text_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_barcode_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Barcode", opencreport_barcode_class_methods);
	ce.create_object = opencreport_barcode_object_new;
	opencreport_barcode_object_handlers.offset = XtOffsetOf(php_opencreport_barcode_object, zo);
	opencreport_barcode_object_handlers.clone_obj = NULL;
	opencreport_barcode_object_handlers.compare = zend_objects_not_comparable;
	opencreport_barcode_ce = zend_register_internal_class(&ce);
	opencreport_barcode_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_barcode_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_barcode_ce->serialize = zend_class_serialize_deny;
	opencreport_barcode_ce->unserialize = zend_class_unserialize_deny;
#endif

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("EXPR_RESULTS", OCRPT_EXPR_RESULTS);

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_PDF", OCRPT_OUTPUT_PDF);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_HTML", OCRPT_OUTPUT_HTML);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_TXT", OCRPT_OUTPUT_TXT);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_CSV", OCRPT_OUTPUT_CSV);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_XML", OCRPT_OUTPUT_XML);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_JSON", OCRPT_OUTPUT_JSON);

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("RESULT_ERROR", OCRPT_RESULT_ERROR);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("RESULT_STRING", OCRPT_RESULT_STRING);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("RESULT_NUMBER", OCRPT_RESULT_NUMBER);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("RESULT_DATETIME", OCRPT_RESULT_DATETIME);

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_EXPRESSION", OCRPT_VARIABLE_EXPRESSION);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_COUNT", OCRPT_VARIABLE_COUNT);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_COUNTALL", OCRPT_VARIABLE_COUNTALL);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_SUM", OCRPT_VARIABLE_SUM);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_AVERAGE", OCRPT_VARIABLE_AVERAGE);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_AVERAGEALL", OCRPT_VARIABLE_AVERAGEALL);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_LOWEST", OCRPT_VARIABLE_LOWEST);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_HIGHEST", OCRPT_VARIABLE_HIGHEST);

	return SUCCESS;
}

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(opencreport) {
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(opencreport) {
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(opencreport) {
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(opencreport)
{
	php_info_print_table_start();

	php_info_print_table_row(2, "OpenCReport", "enabled");
	php_info_print_table_row(2, "OpenCReport module version", PHP_OPENCREPORT_VERSION);
	php_info_print_table_row(2, "OpenCReport library version", ocrpt_version());

	php_info_print_table_end();
}
/* }}} */

ZEND_FUNCTION(rlib_version) {
	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_STRING(ocrpt_version());
}

ZEND_FUNCTION(rlib_init) {
	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(return_value);

	oo->o = ocrpt_init();
	oo->free_me = true;
}

ZEND_FUNCTION(rlib_free) {
	zval *obj;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(obj, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(obj);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	opencreport_object_deinit(oo);
}

ZEND_FUNCTION(rlib_add_datasource_mysql) {
	zval *object;
	zend_string *source_name;
	zend_string *host, *dbname;
	zend_string *user, *password;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 6, 6)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(host, 1, 0);
		Z_PARAM_STR_EX(user, 1, 0);
		Z_PARAM_STR_EX(password, 1, 0);
		Z_PARAM_STR_EX(dbname, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "host", .param_value = host ? ZSTR_VAL(host) : NULL },
		{ .param_name = "dbname", .param_value = dbname ? ZSTR_VAL(dbname) : NULL },
		{ .param_name = "user", .param_value = user ? ZSTR_VAL(user) : NULL },
		{ .param_name = "password", .param_value = password ? ZSTR_VAL(password) : NULL },
		{ NULL }
	};

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "mariadb", conn_params);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_mysql_from_group) {
	zval *object;
	zend_string *source_name;
	zend_string *option_file, *group;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 4)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(group, 1, 0);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(option_file, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "optionfile", .param_value = option_file ? ZSTR_VAL(option_file) : NULL },
		{ .param_name = "group", .param_value = group ? ZSTR_VAL(group) : NULL },
		{ NULL }
	};

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "mariadb", conn_params);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_postgres) {
	zval *object;
	zend_string *source_name;
	zend_string *connection_info;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(connection_info, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "connstr", .param_value = connection_info ? ZSTR_VAL(connection_info) : NULL },
		{ NULL }
	};

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "postgresql", conn_params);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_odbc) {
	zval *object;
	zend_string *source_name;
	zend_string *dbname, *user, *password;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 5)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(dbname, 1, 0);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(user, 1, 0);
		Z_PARAM_STR_EX(password, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "dbname", .param_value = dbname ? ZSTR_VAL(dbname) : NULL },
		{ .param_name = "user", .param_value = user ? ZSTR_VAL(user) : NULL },
		{ .param_name = "password", .param_value = password ? ZSTR_VAL(password) : NULL },
		{ NULL }
	};

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "odbc", conn_params);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_array) {
	zval *object;
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "array", NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_xml) {
	zval *object;
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "xml", NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_csv) {
	zval *object;
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "csv", NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_query_as) {
	zval *object;
	zend_string *source_name, *array_or_file_or_sql, *name;
	ocrpt_query *q;
	php_opencreport_query_object *qo;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 4, 4)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR(array_or_file_or_sql);
		Z_PARAM_STR(name);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_datasource *ds = ocrpt_datasource_get(oo->o, ZSTR_VAL(source_name));

	if (ocrpt_datasource_is_sql(ds))
		q = ocrpt_query_add_sql(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	else if (ocrpt_datasource_is_file(ds))
		q = ocrpt_query_add_file(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), NULL, 0);
	else if (ocrpt_datasource_is_symbolic_data(ds))
		q = ocrpt_query_add_symbolic_data(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), 0, 0, NULL, 0);
	else
		RETURN_NULL();

	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;
}

ZEND_FUNCTION(rlib_add_resultset_follower) {
	zval *object;
	zend_string *leader, *follower;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(leader);
		Z_PARAM_STR(follower);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_query *leader_query = ocrpt_query_get(oo->o, ZSTR_VAL(leader));
	ocrpt_query *follower_query = ocrpt_query_get(oo->o, ZSTR_VAL(follower));

	if (!leader_query || !follower_query)
		RETURN_FALSE;

	RETURN_BOOL(ocrpt_query_add_follower(leader_query, follower_query));
}

ZEND_FUNCTION(rlib_add_resultset_follower_n_to_1) {
	zval *object;
	zend_string *leader, *leader_field, *follower, *follower_field;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 5, 5)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(leader);
		Z_PARAM_STR(leader_field);
		Z_PARAM_STR(follower);
		Z_PARAM_STR(follower_field);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_query *leader_query = ocrpt_query_get(oo->o, ZSTR_VAL(leader));
	ocrpt_query *follower_query = ocrpt_query_get(oo->o, ZSTR_VAL(follower));

	if (!leader_query || !follower_query)
		RETURN_FALSE;

	ocrpt_string *match = ocrpt_mem_string_new_printf("(%s) = (%s)", ZSTR_VAL(leader_field), ZSTR_VAL(follower_field));
	char *err = NULL;
	ocrpt_expr *match_expr = ocrpt_expr_parse(oo->o, match->str, &err);

	if (!match_expr) {
		if (err) {
			zend_throw_error(NULL, "rlib_add_resultset_follower_n_to_1: expression parse error: %s\nrlib_add_resultset_follower_n_to_1: expression was: %s\n", err, match->str);
			ocrpt_strfree(err);
			ocrpt_mem_string_free(match, true);
		}
		RETURN_FALSE;
	}

	ocrpt_mem_string_free(match, true);

	RETURN_BOOL(ocrpt_query_add_follower_n_to_1(leader_query, follower_query, match_expr));
}

ZEND_FUNCTION(rlib_set_datasource_encoding) {
	zval *object;
	zend_string *name, *encoding;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(name);
		Z_PARAM_STR(encoding);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_datasource *ds = ocrpt_datasource_get(oo->o, ZSTR_VAL(name));

	if (!ds)
		return;

	ocrpt_datasource_set_encoding(ds, ZSTR_VAL(encoding));
}

ZEND_FUNCTION(rlib_add_report) {
	zval *object;
	zend_string *filename;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(filename);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	RETURN_BOOL(ocrpt_parse_xml(oo->o, ZSTR_VAL(filename)));
}

ZEND_FUNCTION(rlib_add_report_from_buffer) {
	zval *object;
	zend_string *buffer;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(buffer);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	RETURN_BOOL(ocrpt_parse_xml_from_buffer(oo->o, ZSTR_VAL(buffer), ZSTR_LEN(buffer)));
}

ZEND_FUNCTION(rlib_add_search_path) {
	zval *object;
	zend_string *path;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(path);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_add_search_path(oo->o, ZSTR_VAL(path));
}

ZEND_FUNCTION(rlib_set_locale) {
	zval *object;
	zend_string *locale;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(locale);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_set_locale(oo->o, ZSTR_VAL(locale));
}

ZEND_FUNCTION(rlib_bindtextdomain) {
	zval *object;
	zend_string *domainname;
	zend_string *dirname;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(domainname);
		Z_PARAM_STR(dirname);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_bindtextdomain(oo->o, ZSTR_VAL(domainname), ZSTR_VAL(dirname));
}

ZEND_FUNCTION(rlib_set_output_format_from_text) {
	zval *object;
	zend_string *format;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(format);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_format_type type = OCRPT_OUTPUT_PDF;
	const char *fmt = ZSTR_VAL(format);

	if (strcasecmp(fmt, "pdf") == 0)
		type = OCRPT_OUTPUT_PDF;
	else if (strcasecmp(fmt, "html") == 0)
		type = OCRPT_OUTPUT_HTML;
	else if (strcasecmp(fmt, "txt") == 0 || strcasecmp(fmt, "text") == 0)
		type = OCRPT_OUTPUT_TXT;
	else if (strcasecmp(fmt, "csv") == 0)
		type = OCRPT_OUTPUT_CSV;
	else if (strcasecmp(fmt, "xml") == 0)
		type = OCRPT_OUTPUT_XML;
	else if (strcasecmp(fmt, "json") == 0)
		type = OCRPT_OUTPUT_JSON;

	ocrpt_set_output_format(oo->o, type);
}

OCRPT_STATIC_FUNCTION(rlib_default_function) {
	ocrpt_expr_make_error_result(e, "not implemented");

	int32_t n_ops = ocrpt_expr_get_num_operands(e);

	zval retval;
	zval params[n_ops];

	mpfr_ptr mval;
	ocrpt_string *sval;

	for (int32_t i = 0; i < n_ops; i++) {
		ocrpt_result *rs = ocrpt_expr_operand_get_result(e, i);
		switch (ocrpt_result_get_type(rs)) {
		case OCRPT_RESULT_ERROR:
			sval = ocrpt_result_get_string(rs);
			ocrpt_expr_make_error_result(e, sval->str);
			return;
		case OCRPT_RESULT_STRING:
			sval = ocrpt_result_get_string(rs);
			ZVAL_STRINGL(&params[i], sval->str, sval->len);
			break;
		case OCRPT_RESULT_NUMBER:
			/*
			 * Naive representation.
			 * MPFR number may exceed double precision.
			 */
			mval = ocrpt_result_get_number(rs);
			ZVAL_DOUBLE(&params[i], mpfr_get_d(mval, MPFR_RNDN));
			break;
		case OCRPT_RESULT_DATETIME:
			/* Not implemented yet */
			ZVAL_NULL(&params[i]);
			break;
		}
	}

	char *fname = user_data;
	zval zfname;

	ZVAL_STRING(&zfname, fname);

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, n_ops, params) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, n_ops, params, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_string_release(Z_STR(zfname));

	for (int32_t i = 0; i < n_ops; i++) {
		ocrpt_result *rs = ocrpt_expr_operand_get_result(e, i);
		switch (ocrpt_result_get_type(rs)) {
		case OCRPT_RESULT_ERROR:
			break;
		case OCRPT_RESULT_STRING:
			zend_string_release(Z_STR(params[i]));
			break;
		case OCRPT_RESULT_NUMBER:
			break;
		case OCRPT_RESULT_DATETIME:
			break;
		}
	}

	/*
	 * The function must be a real OpenCReports user function
	 * and called $e->set_{long|double|string}_value(...)
	 */
	if (Z_TYPE(retval) == IS_UNDEF || Z_TYPE(retval) == IS_NULL)
		return;
	else if (Z_TYPE(retval) == IS_FALSE)
		ocrpt_expr_set_long(e, 0);
	else if (Z_TYPE(retval) == IS_TRUE)
		ocrpt_expr_set_long(e, 1);
	else if (Z_TYPE(retval) == IS_LONG)
		ocrpt_expr_set_long(e, Z_LVAL(retval));
	else if (Z_TYPE(retval) == IS_DOUBLE)
		ocrpt_expr_set_double(e, Z_DVAL(retval));
	else if (Z_TYPE(retval) == IS_STRING)
		ocrpt_expr_set_string(e, Z_STRVAL(retval));
	else
		ocrpt_expr_make_error_result(e, "invalid return value");
}

ZEND_FUNCTION(rlib_add_function) {
	zval *object;
	zend_string *name, *function;
	zend_long params;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 4, 4)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(name);
		Z_PARAM_STR(function);
		Z_PARAM_LONG(params);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	char *zfunc = ocrpt_mem_strdup(ZSTR_VAL(function));
	ocrpt_function_add(oo->o, ZSTR_VAL(name), rlib_default_function, zfunc, params, false, false, false, true);
}

ZEND_FUNCTION(rlib_set_output_encoding) {
	zval *object;
	zend_string *encoding;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(encoding);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Do nothing. OpenCReports is UTF-8 only. */
}

ZEND_FUNCTION(rlib_add_parameter) {
	zval *object;
	zend_string *param;
	zend_string *value = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(param);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(value, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_set_mvariable(oo->o, ZSTR_VAL(param), value ? ZSTR_VAL(value) : NULL);
}

ZEND_FUNCTION(rlib_set_output_parameter) {
	zval *object;
	zend_string *param, *value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(param);
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_set_output_parameter(oo->o, ZSTR_VAL(param), ZSTR_VAL(value));
}

ZEND_FUNCTION(rlib_query_refresh) {
	zval *object;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_query_refresh(oo->o);
}

static void rlib_report_cb(opencreport *o, ocrpt_report *r, void *data) {
	zend_string *callback = data;
	zval zfname;

	ZVAL_STRINGL(&zfname, ZSTR_VAL(callback), ZSTR_LEN(callback));

	zval retval;

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, 0, NULL) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 0, NULL, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_string_release(Z_STR(zfname));
}

static void rlib_part_cb(opencreport *o, ocrpt_part *p, void *data) {
	zend_string *callback = data;
	zval zfname;

	ZVAL_STRINGL(&zfname, ZSTR_VAL(callback), ZSTR_LEN(callback));

	zval retval;

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, 0, NULL) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 0, NULL, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_string_release(Z_STR(zfname));
}

ZEND_FUNCTION(rlib_signal_connect) {
	zval *object;
	zend_string *signal, *function;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(signal);
		Z_PARAM_STR(function);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	zend_string *func = zend_string_init(ZSTR_VAL(function), ZSTR_LEN(function), false);

	char *signal_name = ZSTR_VAL(signal);
	if (!strcasecmp(signal_name, "row_change"))
		ocrpt_report_add_new_row_cb2(oo->o, rlib_report_cb, func);
	else if (!strcasecmp(signal_name, "report_done"))
		ocrpt_report_add_done_cb2(oo->o, rlib_report_cb, func);
	else if (!strcasecmp(signal_name, "report_start"))
		ocrpt_report_add_start_cb2(oo->o, rlib_report_cb, func);
	else if (!strcasecmp(signal_name, "report_iteration"))
		ocrpt_report_add_iteration_cb2(oo->o, rlib_report_cb, func);
	else if (!strcasecmp(signal_name, "part_iteration"))
		ocrpt_part_add_iteration_cb2(oo->o, rlib_part_cb, func);
	else if (!strcasecmp(signal_name, "precalculation_done"))
		ocrpt_report_add_precalculation_done_cb2(oo->o, rlib_report_cb, func);
}

ZEND_FUNCTION(rlib_execute) {
	zval *object;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_execute(oo->o);
}

ZEND_FUNCTION(rlib_spool) {
	zval *object;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_spool(oo->o);
}

ZEND_FUNCTION(rlib_get_content_type) {
	zval *object;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	const ocrpt_string **content_type = ocrpt_get_content_type(oo->o);

	if (!content_type)
		RETURN_FALSE;

	ocrpt_string *retval = ocrpt_mem_string_new("", true);

	for (int32_t i = 0; content_type[i]; i++)
		ocrpt_mem_string_append_printf(retval, "%s\n", content_type[i]->str);

	RETVAL_STRINGL(retval->str, retval->len);

	ocrpt_mem_string_free(retval, true);
}

ZEND_FUNCTION(rlib_set_radix_character) {
	zval *object;
	zend_string *radix;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(radix);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. The radix character depends on the locale. */
}

ZEND_FUNCTION(rlib_compile_infix) {
	zend_string *expr_string;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(expr_string);
	ZEND_PARSE_PARAMETERS_END();

	opencreport *o = ocrpt_init();

	char *err = NULL;
	ocrpt_expr *e = ocrpt_expr_parse(o, ZSTR_VAL(expr_string), &err);

	if (!e) {
		RETVAL_STRING(err);
		ocrpt_strfree(err);
		ocrpt_free(o);
		return;
	}

	ocrpt_expr_optimize(e);
	ocrpt_expr_eval(e);

	mpfr_ptr mval;
	ocrpt_string *sval;
	ocrpt_result *rs = ocrpt_expr_get_result(e);
	switch (ocrpt_result_get_type(rs)) {
	case OCRPT_RESULT_ERROR:
	case OCRPT_RESULT_STRING:
		sval = ocrpt_result_get_string(rs);
		RETVAL_STRINGL(sval->str, sval->len);
		break;
	case OCRPT_RESULT_NUMBER:
		/*
		 * Naive representation.
		 * MPFR number may exceed double precision.
		 */
		mval = ocrpt_result_get_number(rs);
		RETVAL_DOUBLE(mpfr_get_d(mval, MPFR_RNDN));
		break;
	case OCRPT_RESULT_DATETIME:
		/* Not implemented yet */
		RETVAL_NULL();
		break;
	}

	ocrpt_free(o);
}

ZEND_FUNCTION(rlib_graph_add_bg_region) {
	zval *object;
	zend_string *graph, *region, *color;
	double start, end;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 6, 6)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(graph);
		Z_PARAM_STR(region);
		Z_PARAM_STR(color);
		Z_PARAM_DOUBLE(start);
		Z_PARAM_DOUBLE(end);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. Not implemented yet. */
}

ZEND_FUNCTION(rlib_graph_clear_bg_region) {
	zval *object;
	zend_string *graph;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(graph);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. Not implemented yet. */
}

ZEND_FUNCTION(rlib_graph_set_x_minor_tick) {
	zval *object;
	zend_string *graph, *x;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(graph);
		Z_PARAM_STR(x);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. Not implemented yet. */
}

ZEND_FUNCTION(rlib_graph_set_x_minor_tick_by_location) {
	zval *object;
	zend_string *graph;
	double location;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(graph);
		Z_PARAM_DOUBLE(location);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. Not implemented yet. */
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_init, 0, 0, OpenCReport, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_free, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_mysql, 0, 6, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_mysql_from_group, 0, 3, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, group, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, option_file, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_postgres, 0, 3, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, connection_info, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_odbc, 0, 3, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dsn, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, user, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, password, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_array, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_xml, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_csv, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_query_as, 0, 4, OpenCReport\\Query, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, array_or_file_or_sql, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, query_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_resultset_follower, 0, 3, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, leader, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, follower, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_resultset_follower_n_to_1, 0, 5, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, leader, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, leader_field, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, follower, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, follower_field, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_datasource_encoding, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, encoding, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_report, 0, 2, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_report_from_buffer, 0, 2, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, buffer, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_search_path, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_version, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_locale, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, locale, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_bindtextdomain, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, domainname, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dirname, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_output_format_from_text, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, format, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_function, 0, 4, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, function, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, params, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_output_encoding, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, encoding, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_parameter, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, param, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, value, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_output_parameter, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, param, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_query_refresh, 0, 0, IS_VOID, 0)
ZEND_ARG_VARIADIC_OBJ_INFO(0, r, OpenCReport, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_signal_connect, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, signal, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, function, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_execute, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_spool, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_get_content_type, 0, 1, IS_STRING, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_radix_character, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, radix, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* The return type may be string, double or false */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_compile_infix, 0, 1, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_graph_add_bg_region, 0, 6, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, graph, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, region, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, color, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, start, IS_DOUBLE, 0)
ZEND_ARG_TYPE_INFO(0, end, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_graph_clear_bg_region, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, graph, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_graph_set_x_minor_tick, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, graph, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, x, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_graph_set_x_minor_tick_by_location, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, graph, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, location, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

/* {{{ zend_function_entry */
static const zend_function_entry opencreport_functions[] = {
	ZEND_FE(rlib_version, arginfo_rlib_version)
	ZEND_FE(rlib_init, arginfo_rlib_init)
	ZEND_FE(rlib_free, arginfo_rlib_free)
	ZEND_FE(rlib_add_datasource_mysql, arginfo_rlib_add_datasource_mysql)
	ZEND_FE(rlib_add_datasource_mysql_from_group, arginfo_rlib_add_datasource_mysql_from_group)
	ZEND_FE(rlib_add_datasource_postgres, arginfo_rlib_add_datasource_postgres)
	ZEND_FE(rlib_add_datasource_odbc, arginfo_rlib_add_datasource_odbc)
	ZEND_FE(rlib_add_datasource_array, arginfo_rlib_add_datasource_array)
	ZEND_FE(rlib_add_datasource_xml, arginfo_rlib_add_datasource_xml)
	ZEND_FE(rlib_add_datasource_csv, arginfo_rlib_add_datasource_csv)
	ZEND_FE(rlib_add_query_as, arginfo_rlib_add_query_as)
	ZEND_FE(rlib_add_resultset_follower, arginfo_rlib_add_resultset_follower)
	ZEND_FE(rlib_add_resultset_follower_n_to_1, arginfo_rlib_add_resultset_follower_n_to_1)
	ZEND_FE(rlib_set_datasource_encoding, arginfo_rlib_set_datasource_encoding)
	ZEND_FE(rlib_add_report, arginfo_rlib_add_report)
	ZEND_FE(rlib_add_report_from_buffer, arginfo_rlib_add_report_from_buffer)
	ZEND_FE(rlib_add_search_path, arginfo_rlib_add_search_path)
	ZEND_FE(rlib_set_locale, arginfo_rlib_set_locale)
	ZEND_FE(rlib_bindtextdomain, arginfo_rlib_bindtextdomain)
	ZEND_FE(rlib_set_output_format_from_text, arginfo_rlib_set_output_format_from_text)
	ZEND_FE(rlib_add_function, arginfo_rlib_add_function)
	ZEND_FE(rlib_set_output_encoding, arginfo_rlib_set_output_encoding)
	ZEND_FE(rlib_add_parameter, arginfo_rlib_add_parameter)
	ZEND_FE(rlib_set_output_parameter, arginfo_rlib_set_output_parameter)
	ZEND_FE(rlib_query_refresh, arginfo_rlib_query_refresh)
	ZEND_FE(rlib_signal_connect, arginfo_rlib_signal_connect)
	ZEND_FE(rlib_execute, arginfo_rlib_execute)
	ZEND_FE(rlib_spool, arginfo_rlib_spool)
	ZEND_FE(rlib_get_content_type, arginfo_rlib_get_content_type)
	ZEND_FE(rlib_set_radix_character, arginfo_rlib_set_radix_character)
	ZEND_FE(rlib_compile_infix, arginfo_rlib_compile_infix)
	ZEND_FE(rlib_graph_add_bg_region, arginfo_rlib_graph_add_bg_region)
	ZEND_FE(rlib_graph_clear_bg_region, arginfo_rlib_graph_clear_bg_region)
	ZEND_FE(rlib_graph_set_x_minor_tick, arginfo_rlib_graph_set_x_minor_tick)
	ZEND_FE(rlib_graph_set_x_minor_tick_by_location, arginfo_rlib_graph_set_x_minor_tick_by_location)
	PHP_FE_END
};
/* }}} */

/* {{{ opencreport_module_entry
 */
zend_module_entry opencreport_module_entry = {
	STANDARD_MODULE_HEADER,
	"opencreport",
	opencreport_functions,
	PHP_MINIT(opencreport),
	PHP_MSHUTDOWN(opencreport),
	PHP_RINIT(opencreport),
	PHP_RSHUTDOWN(opencreport),
	PHP_MINFO(opencreport),
	PHP_OPENCREPORT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

ZEND_GET_MODULE(opencreport)
