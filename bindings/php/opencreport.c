/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
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
#include <unistd.h>
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

/* Global list of callback data */
static ocrpt_list *funcnames;
static ocrpt_list *funcnames_last;

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
	char *res = ocrpt_get_output(oo->o, &sz);

	RETURN_STRINGL(res, sz);
}

PHP_METHOD(opencreport, version) {
	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_STRING(ocrpt_version());
}

PHP_METHOD(opencreport, set_numeric_precision_bits) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_long prec;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(prec);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_numeric_precision_bits(oo->o, (mpfr_prec_t)prec);
}

PHP_METHOD(opencreport, set_rounding_mode) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_long mode;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(mode);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_rounding_mode(oo->o, (mpfr_rnd_t)mode);
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

PHP_METHOD(opencreport, datasource_add_array) {
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

	ds = ocrpt_datasource_add_array(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_csv) {
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

	ds = ocrpt_datasource_add_csv(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_json) {
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

	ds = ocrpt_datasource_add_json(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_xml) {
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

	ds = ocrpt_datasource_add_xml(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_postgresql) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *host = NULL, *port = NULL, *dbname = NULL;
	zend_string *user = NULL, *password = NULL;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 6, 6)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(host, 1, 0);
		Z_PARAM_STR_EX(port, 1, 0);
		Z_PARAM_STR_EX(dbname, 1, 0);
		Z_PARAM_STR_EX(user, 1, 0);
		Z_PARAM_STR_EX(password, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_postgresql(
				oo->o, ZSTR_VAL(source_name),
				host ? ZSTR_VAL(host) : NULL,
				port ? ZSTR_VAL(port) : NULL,
				dbname ? ZSTR_VAL(dbname) : NULL,
				user ? ZSTR_VAL(user) : NULL,
				password ? ZSTR_VAL(password) : NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_postgresql2) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *connection_info = NULL;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(connection_info, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_postgresql2(
				oo->o, ZSTR_VAL(source_name),
				connection_info ? ZSTR_VAL(connection_info) : NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_mariadb) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *host = NULL, *port = NULL, *dbname = NULL;
	zend_string *user = NULL, *password = NULL, *unix_socket = NULL;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 7, 7)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(host, 1, 0);
		Z_PARAM_STR_EX(port, 1, 0);
		Z_PARAM_STR_EX(dbname, 1, 0);
		Z_PARAM_STR_EX(user, 1, 0);
		Z_PARAM_STR_EX(password, 1, 0);
		Z_PARAM_STR_EX(unix_socket, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_mariadb(
				oo->o, ZSTR_VAL(source_name),
				host ? ZSTR_VAL(host) : NULL,
				port ? ZSTR_VAL(port) : NULL,
				dbname ? ZSTR_VAL(dbname) : NULL,
				user ? ZSTR_VAL(user) : NULL,
				password ? ZSTR_VAL(password) : NULL,
				unix_socket ? ZSTR_VAL(unix_socket) : NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_mariadb2) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *option_file = NULL, *group = NULL;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(option_file, 1, 0);
		Z_PARAM_STR_EX(group, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_mariadb2(
				oo->o, ZSTR_VAL(source_name),
				option_file ? ZSTR_VAL(option_file) : NULL,
				group ? ZSTR_VAL(group) : NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_odbc) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *dbname = NULL, *user = NULL, *password = NULL;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 4)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(dbname, 1, 0);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(user, 1, 0);
		Z_PARAM_STR_EX(password, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_odbc(
				oo->o, ZSTR_VAL(source_name),
				dbname ? ZSTR_VAL(dbname) : NULL,
				user ? ZSTR_VAL(user) : NULL,
				password ? ZSTR_VAL(password) : NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_add_odbc2) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *connection_info = NULL;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(connection_info, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_odbc2(
				oo->o, ZSTR_VAL(source_name),
				connection_info ? ZSTR_VAL(connection_info) : NULL);
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

	if (e) {
		object_init_ex(return_value, opencreport_expr_ce);
		php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
		eo->e = e;
		ocrpt_strfree(oo->expr_error);
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
	char *fname = user_data;
	zval zfname;

	ocrpt_expr_make_error_result(e, "not implemented");

	ZVAL_STRING(&zfname, fname);

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
		ocrpt_expr_set_long_value(e, 0);
	else if (Z_TYPE(retval) == IS_TRUE)
		ocrpt_expr_set_long_value(e, 1);
	else if (Z_TYPE(retval) == IS_LONG)
		ocrpt_expr_set_long_value(e, Z_LVAL(retval));
	else if (Z_TYPE(retval) == IS_DOUBLE)
		ocrpt_expr_set_double_value(e, Z_DVAL(retval));
	else if (Z_TYPE(retval) == IS_STRING)
		ocrpt_expr_set_string_value(e, Z_STRVAL(retval));
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

PHP_METHOD(opencreport, part_get_next) {
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

	char *zfunc = ocrpt_mem_strdup(ZSTR_VAL(zend_func_name));
	bool ret = ocrpt_function_add(oo->o, ZSTR_VAL(expr_func_name), opencreport_default_function, zfunc, n_ops, commutative, associative, left_associative, dont_optimize);

	if (!ret) {
		ocrpt_strfree(zfunc);
		RETURN_FALSE;
	}

	RETURN_TRUE;
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
	char *callback = data;
	zval zfname;

	ZVAL_STRING(&zfname, callback);

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
	char *callback = data;
	zval zfname;

	ZVAL_STRING(&zfname, callback);

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

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

	ocrpt_add_precalculation_done_cb(oo->o, opencreport_cb, cb_name);
}

PHP_METHOD(opencreport, add_part_added_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

	ocrpt_add_part_added_cb(oo->o, opencreport_part_cb, cb_name);
}

static void opencreport_report_cb(opencreport *o, ocrpt_report *r, void *data) {
	char *callback = data;
	zval zfname;

	ZVAL_STRING(&zfname, callback);

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

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

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

ZEND_BEGIN_ARG_INFO_EX(arginfo_opencreport___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_parse_xml, 0, 1, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_output_format, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, format, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_execute, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_spool, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_output, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_version, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_numeric_precision_bits, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, prec, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_rounding_mode, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, mode, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_bindtextdomain, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, domainname, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dirname, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_locale, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, locale, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_array, 0, 1, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_file, 0, 1, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_postgresql, 0, 6, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, port, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_postgresql2, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, connection_info, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_mariadb, 0, 7, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, port, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, unix_socket, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_mariadb2, 0, 3, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, option_file, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, group, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_odbc, 0, 4, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_odbc2, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, connection_info, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_get, 0, 1, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_query_get, 0, 1, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, query_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_parse, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_error, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_new, 0, 0, OpenCReport\\Part, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_next, 0, 0, OpenCReport\\Part, 1)
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

static const zend_function_entry opencreport_class_methods[] = {
	/*
	 * High level API
	 */
	PHP_ME(opencreport, __construct, arginfo_opencreport___construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, parse_xml, arginfo_opencreport_parse_xml, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_output_format, arginfo_opencreport_set_output_format, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, execute, arginfo_opencreport_execute, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, spool, arginfo_opencreport_spool, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_output, arginfo_opencreport_get_output, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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
	PHP_ME(opencreport, datasource_add_array, arginfo_opencreport_datasource_add_array, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_csv, arginfo_opencreport_datasource_add_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_json, arginfo_opencreport_datasource_add_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_xml, arginfo_opencreport_datasource_add_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_postgresql, arginfo_opencreport_datasource_add_postgresql, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_postgresql2, arginfo_opencreport_datasource_add_postgresql2, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_mariadb, arginfo_opencreport_datasource_add_mariadb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_mariadb2, arginfo_opencreport_datasource_add_mariadb2, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_odbc, arginfo_opencreport_datasource_add_odbc, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_add_odbc2, arginfo_opencreport_datasource_add_odbc2, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_get, arginfo_opencreport_datasource_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, query_get, arginfo_opencreport_query_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Expression related methods */
	PHP_ME(opencreport, expr_parse, arginfo_opencreport_expr_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, expr_error, arginfo_opencreport_expr_error, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Function related methods */
	PHP_ME(opencreport, function_add, arginfo_opencreport_function_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Report part related functions */
	PHP_ME(opencreport, part_new, arginfo_opencreport_part_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, part_get_next, arginfo_opencreport_part_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Callback related methods */
	PHP_ME(opencreport, add_precalculation_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, add_part_added_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, add_report_added_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Environment related methods */
	PHP_ME(opencreport, env_get, arginfo_opencreport_env_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Result related methods */
	PHP_ME(opencreport, result_new, arginfo_opencreport_result_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* File handling related methods */
	PHP_ME(opencreport, add_search_path, arginfo_opencreport_add_search_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, canonicalize_path, arginfo_opencreport_canonicalize_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	PHP_ME(opencreport, find_file, arginfo_opencreport_find_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Color related methods */
	PHP_ME(opencreport, get_color, arginfo_opencreport_get_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
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
	void *array_x, *types_x = NULL;
	int32_t rows, cols, types_cols = 0;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(name);
		Z_PARAM_STR(array_or_file_or_sql);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(coltypes, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	if (ocrpt_datasource_is_array(dso->ds)) {
		if (coltypes && ZSTR_LEN(coltypes) > 0) {
			ocrpt_query_discover_array(ZSTR_VAL(array_or_file_or_sql), &array_x, &rows, &cols, ZSTR_VAL(coltypes), &types_x, &types_cols);
		} else {
			ocrpt_query_discover_array(ZSTR_VAL(array_or_file_or_sql), &array_x, &rows, &cols, NULL, NULL, NULL);
		}

		q = ocrpt_query_add_array(dso->ds, ZSTR_VAL(name), array_x, rows, cols, types_x, types_cols);
	} else if (ocrpt_datasource_is_csv(dso->ds)) {
		if (coltypes && ZSTR_LEN(coltypes) > 0)
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(coltypes), &types_x, &types_cols);

		q = ocrpt_query_add_csv(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), types_x, types_cols);
	} else if (ocrpt_datasource_is_json(dso->ds)) {
		if (coltypes && ZSTR_LEN(coltypes) > 0)
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(coltypes), &types_x, &types_cols);

		q = ocrpt_query_add_json(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), types_x, types_cols);
	} else if (ocrpt_datasource_is_xml(dso->ds)) {
		if (coltypes && ZSTR_LEN(coltypes) > 0)
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(coltypes), &types_x, &types_cols);

		q = ocrpt_query_add_xml(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), types_x, types_cols);
	} else if (ocrpt_datasource_is_postgresql(dso->ds)) {
		q = ocrpt_query_add_postgresql(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	} else if (ocrpt_datasource_is_mariadb(dso->ds)) {
		q = ocrpt_query_add_mariadb(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	} else if (ocrpt_datasource_is_odbc(dso->ds)) {
		q = ocrpt_query_add_odbc(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	} else
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

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_add_follower, 0, 0, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, follower, OpenCReport\\Query, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_add_follower_n_to_1, 0, 0, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, follower, OpenCReport\\Query, 0)
ZEND_ARG_OBJ_INFO(0, match, OpenCReport\\Expr, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_query_class_methods[] = {
	PHP_ME(opencreport_query, get_result, arginfo_opencreport_query_get_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, navigate_start, arginfo_opencreport_query_navigate_start, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query, navigate_next, arginfo_opencreport_query_navigate_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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

PHP_METHOD(opencreport_expr, set_string_value) {
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

	ocrpt_expr_set_string_value(eo->e, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, set_long_value) {
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

	ocrpt_expr_set_long_value(eo->e, value);
}

PHP_METHOD(opencreport_expr, set_double_value) {
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

	ocrpt_expr_set_double_value(eo->e, value);
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

PHP_METHOD(opencreport_expr, set_nth_result_string_value) {
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

	ocrpt_expr_set_nth_result_string_value(eo->e, which, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, set_nth_result_long_value) {
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

	ocrpt_expr_set_nth_result_long_value(eo->e, which, value);
}

PHP_METHOD(opencreport_expr, set_nth_result_double_value) {
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

	ocrpt_expr_set_nth_result_double_value(eo->e, which, value);
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

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_string_value, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_long_value, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_double_value, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_num_operands, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_operand_get_result, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, opidx, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_cmp_results, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_init_results, 0, 0, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, result_type, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_string_value, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_long_value, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_double_value, 0, 2, IS_VOID, 0)
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
	PHP_ME(opencreport_expr, set_string_value, arginfo_opencreport_expr_set_string_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_long_value, arginfo_opencreport_expr_set_long_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_double_value, arginfo_opencreport_expr_set_double_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_num_operands, arginfo_opencreport_expr_get_num_operands, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, operand_get_result, arginfo_opencreport_expr_operand_get_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, cmp_results, arginfo_opencreport_expr_cmp_results, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, init_results, arginfo_opencreport_expr_init_results, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_string_value, arginfo_opencreport_expr_set_nth_result_string_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_long_value, arginfo_opencreport_expr_set_nth_result_long_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_double_value, arginfo_opencreport_expr_set_nth_result_double_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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
	size_t len = mpfr_snprintf(NULL, 0, fmt, number);

	char *retval = emalloc(len + 1);
	mpfr_snprintf(retval, len + 1, fmt, number);

	RETURN_STRINGL(retval, len);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_copy, 0, 0, IS_VOID, 0)
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

PHP_METHOD(opencreport_part, part_get_next) {
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

PHP_METHOD(opencreport_part, row_get_next) {
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

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

	ocrpt_part_add_iteration_cb(po->p, opencreport_part_cb, cb_name);
}

PHP_METHOD(opencreport_part, equals) {
	zval *object = ZEND_THIS;
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zval *report;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(report, opencreport_part_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_part_object *po1 = Z_OPENCREPORT_PART_P(report);

	RETURN_BOOL(po->p == po1->p);
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_part_get_next, 0, 0, OpenCReport\\Part, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_new, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_get_next, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_equals, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, report, OpenCReport\\Part, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_part_class_methods[] = {
	PHP_ME(opencreport_part, part_get_next, arginfo_opencreport_part_part_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, row_new, arginfo_opencreport_part_row_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, row_get_next, arginfo_opencreport_part_row_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, add_iteration_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, equals, arginfo_opencreport_part_equals, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_part_row, row_get_next) {
	zval *object = ZEND_THIS;
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	if (!pro->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Part\\Row object is not an iterator");
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

PHP_METHOD(opencreport_part_row, column_new) {
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

PHP_METHOD(opencreport_part_row, column_get_next) {
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

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_row_get_next, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_column_new, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_column_get_next, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_part_row_class_methods[] = {
	PHP_ME(opencreport_part_row, row_get_next, arginfo_opencreport_part_row_row_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_row, column_new, arginfo_opencreport_part_row_column_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_row, column_get_next, arginfo_opencreport_part_row_column_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_part_col, column_get_next) {
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

PHP_METHOD(opencreport_part_col, report_get_next) {
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

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_column_get_next, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_report_new, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_report_get_next, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_part_col_class_methods[] = {
	PHP_ME(opencreport_part_col, column_get_next, arginfo_opencreport_part_col_column_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, report_new, arginfo_opencreport_part_col_report_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, report_get_next, arginfo_opencreport_part_col_report_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_report, report_get_next) {
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

PHP_METHOD(opencreport_report, break_get_next) {
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

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

	ocrpt_report_add_start_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_done_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

	ocrpt_report_add_done_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_new_row_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

	ocrpt_report_add_new_row_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_iteration_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

	ocrpt_report_add_iteration_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_precalculation_done_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

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

	ocrpt_report_set_main_query_by_name(pro->r, ZSTR_VAL(query_name));
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_report_get_next, 0, 0, OpenCReport\\Report, 1)
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

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_break_new, 0, 1, OpenCReport\\Break, 1)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_break_get_next, 0, 0, OpenCReport\\Break, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_resolve_breaks, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_get_query_rownum, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_break_get, 0, 1, OpenCReport\\Break, 1)
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

static const zend_function_entry opencreport_report_class_methods[] = {
	PHP_ME(opencreport_report, report_get_next, arginfo_opencreport_report_report_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_main_query, arginfo_opencreport_report_set_main_query, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_main_query_by_name, arginfo_opencreport_report_set_main_query_by_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, variable_new, arginfo_opencreport_variable_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, variable_new_full, arginfo_opencreport_variable_new_full, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, expr_parse, arginfo_opencreport_report_expr_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, expr_error, arginfo_opencreport_report_expr_error, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, resolve_variables, arginfo_opencreport_report_resolve_variables, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, evaluate_variables, arginfo_opencreport_report_evaluate_variables, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_new, arginfo_opencreport_report_break_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_get_next, arginfo_opencreport_report_break_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, resolve_breaks, arginfo_opencreport_report_resolve_breaks, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_query_rownum, arginfo_opencreport_report_get_query_rownum, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_start_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_new_row_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_iteration_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_precalculation_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_get, arginfo_opencreport_report_break_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, equals, arginfo_opencreport_report_equals, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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
	zend_bool precalculate;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_BOOL(precalculate);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_variable_set_precalculate(vo->v, precalculate);
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
ZEND_ARG_TYPE_INFO(0, precalculate, _IS_BOOL, 0)
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

PHP_METHOD(opencreport_report_break, break_get_next) {
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

PHP_METHOD(opencreport_report_break, breakfield_add) {
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

PHP_METHOD(opencreport_report_break, check_fields) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_break_check_fields(bro->br));
}

PHP_METHOD(opencreport_report_break, reset_vars) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_break_reset_vars(bro->br);
}

static void opencreport_report_break_cb(opencreport *o, ocrpt_report *r, ocrpt_break *br, void *data) {
	char *callback = data;
	zval zfname;

	ZVAL_STRING(&zfname, callback);

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

PHP_METHOD(opencreport_report_break, add_trigger_cb) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	funcnames = ocrpt_list_end_append(funcnames, &funcnames_last, cb_name);

	ocrpt_break_add_trigger_cb(bro->br, opencreport_report_break_cb, cb_name);
}

PHP_METHOD(opencreport_report_break, name) {
	zval *object = ZEND_THIS;
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_STRING(ocrpt_break_get_name(bro->br));
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_break_break_get_next, 0, 0, OpenCReport\\Break, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_break_breakfield_add, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, breakfield_expr, OpenCReport\\Expr, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_break_check_fields, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_break_reset_vars, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_break_name, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_report_break_class_methods[] = {
	PHP_ME(opencreport_report_break, break_get_next, arginfo_opencreport_report_break_break_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report_break, breakfield_add, arginfo_opencreport_report_break_breakfield_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report_break, check_fields, arginfo_opencreport_report_break_check_fields, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report_break, reset_vars, arginfo_opencreport_report_break_reset_vars, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report_break, add_trigger_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report_break, name, arginfo_opencreport_report_break_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

static HashTable *php_opencreport_arrays;

struct php_opencreport_array {
	zend_string *arrayname;
	union {
		char **array;
		int32_t *types;
	} a;
	uint32_t rows;
	uint32_t cols;
	bool types;
};

static void php_opencreport_array_dtor(zval *data) {
	struct php_opencreport_array *arr = (struct php_opencreport_array *)Z_PTR_P(data);

	zend_string_release(arr->arrayname);
	ocrpt_mem_free(arr->a.array);
	ocrpt_mem_free(arr);
}

static zval *php_opencreport_get_array(const char *arrayname) {
	int unref_count;
	zval *zv = zend_hash_str_find(&EG(symbol_table), arrayname, strlen(arrayname));
	if (zv == NULL)
		return NULL;

	/* Prevent an infinite loop with unref_count */
	for (unref_count = 0; unref_count < 3 && Z_TYPE_P(zv) != IS_ARRAY; unref_count++) {
		if (EXPECTED(Z_TYPE_P(zv) == IS_INDIRECT))
			zv = Z_INDIRECT_P(zv);
		if (EXPECTED(Z_TYPE_P(zv) == IS_REFERENCE))
			zv = Z_REFVAL_P(zv);
	}

	if (UNEXPECTED(Z_TYPE_P(zv) != IS_ARRAY))
		return NULL;

	return zv;
}

static void php_opencreport_query_discover_array(const char *arrayname, void **array, int32_t *rows, int32_t *cols, const char *typesname, void **types, int32_t *types_cols) {
	if (arrayname && *arrayname) {
		zend_string *zarrayname = zend_string_init(arrayname, strlen(arrayname), 0);
		struct php_opencreport_array *arr = (struct php_opencreport_array *)zend_hash_find_ptr(php_opencreport_arrays, zarrayname);

		if (!arr) {
			zval *zv_array = php_opencreport_get_array(arrayname);

			if (zv_array) {
				HashTable *htab1;
				HashPosition htab1p;
				int32_t a_rows, a_cols;

				htab1 = Z_ARRVAL_P(zv_array);
				a_rows = zend_hash_num_elements(htab1);

				zend_hash_internal_pointer_reset_ex(htab1, &htab1p);
				zval *row_element = zend_hash_get_current_data_ex(htab1, &htab1p);

				if (EXPECTED(Z_TYPE_P(row_element) == IS_ARRAY)) {
					HashTable *htab2 = Z_ARRVAL_P(row_element);
					a_cols = zend_hash_num_elements(htab2);
				} else
					a_cols = 1;

				int32_t sz = a_rows * a_cols * sizeof(char *);
				if (sz > 0) {
					arr = ocrpt_mem_malloc(sizeof(struct php_opencreport_array));
					arr->arrayname = zarrayname;
					arr->a.array = ocrpt_mem_malloc(sz);
					memset(arr->a.array, 0, sz);
					arr->rows = a_rows - 1; /* discount the header row */
					arr->cols = a_cols;
					arr->types = false;

					int32_t row, col;

					for (row = 0, zend_hash_internal_pointer_reset_ex(htab1, &htab1p); row < a_rows; row++, zend_hash_move_forward_ex(htab1, &htab1p)) {
						zval *rowval = zend_hash_get_current_data_ex(htab1, &htab1p);
						if (UNEXPECTED(Z_TYPE_P(rowval) != IS_ARRAY)) {
							col = 0;
							continue;
						}

						HashTable *htab2 = Z_ARRVAL_P(rowval);
						HashPosition htab2p;

						for (col = 0, zend_hash_internal_pointer_reset_ex(htab2, &htab2p); col < a_cols; col++, zend_hash_move_forward_ex(htab2, &htab2p)) {
							zval *cell = zend_hash_get_current_data_ex(htab2, &htab2p);
							char *data_result;

							if (cell == NULL || Z_TYPE_P(cell) == IS_NULL)
								data_result = NULL;
							else {
								zval copy;

								ZVAL_STR(&copy, zval_get_string_func(cell));
								data_result = Z_STRVAL(copy);
							}

							arr->a.array[(row * a_cols) + col] = ocrpt_mem_strdup(data_result);
						}
					}

					zval zarr;
					ZVAL_PTR(&zarr, arr);

					zend_hash_add_new(php_opencreport_arrays, zarrayname, &zarr);
				}
			}
		}

		if (array)
			*array = arr ? arr->a.array : NULL;
		if (rows)
			*rows = arr ? arr->rows : 0;
		if (cols)
			*cols = arr ? arr->cols : 0;

		if (!arr)
			zend_string_release(zarrayname);
	} else {
		if (array)
			*array = NULL;
		if (rows)
			*rows = 0;
		if (cols)
			*cols = 0;
	}

	if (typesname && *typesname) {
		zend_string *ztypesname = zend_string_init(typesname, strlen(typesname), 0);
		struct php_opencreport_array *typ = (struct php_opencreport_array *)zend_hash_find_ptr(php_opencreport_arrays, ztypesname);

		if (!typ) {
			zval *zv_types = php_opencreport_get_array(typesname);

			if (zv_types) {
				HashTable *htab1;
				HashPosition htab1p;
				int32_t t_cols;

				htab1 = Z_ARRVAL_P(zv_types);
				t_cols = zend_hash_num_elements(htab1);

				int32_t sz = t_cols * sizeof(int32_t);
				if (sz > 0) {
					typ = ocrpt_mem_malloc(sizeof(struct php_opencreport_array));
					typ->arrayname = ztypesname;
					typ->a.types = ocrpt_mem_malloc(sz);
					memset(typ->a.types, 0, sz);
					typ->rows = 1;
					typ->cols = t_cols;
					typ->types = true;

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

						typ->a.types[col] = data_result;
					}

					zval ztyp;
					ZVAL_PTR(&ztyp, typ);

					zend_hash_add_new(php_opencreport_arrays, ztypesname, &ztyp);
				}
			}
		}

		if (types)
			*types = typ ? typ->a.types : NULL;
		if (types_cols)
			*types_cols = typ ? typ->cols : 0;

		if (!typ)
			zend_string_release(ztypesname);
	} else {
		if (types)
			*types = NULL;
		if (types_cols)
			*types_cols = 0;
	}
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

/* {{{ PHP_MINIT_FUNCTION */
static PHP_MINIT_FUNCTION(opencreport)
{
	zend_class_entry ce;

	ocrpt_query_set_discover_func(php_opencreport_query_discover_array);
	ocrpt_env_set_query_func(php_opencreport_env_query);
	ocrpt_set_printf_func(php_opencreport_std_printf);
	ocrpt_set_err_printf_func(php_opencreport_err_printf);

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
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Row", opencreport_part_row_class_methods);
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
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Break", opencreport_report_break_class_methods);
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

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("EXPR_RESULTS", OCRPT_EXPR_RESULTS);

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_UNSET", OCRPT_OUTPUT_UNSET);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_PDF", OCRPT_OUTPUT_PDF);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_HTML", OCRPT_OUTPUT_HTML);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_TXT", OCRPT_OUTPUT_TXT);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_CSV", OCRPT_OUTPUT_CSV);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_XML", OCRPT_OUTPUT_XML);

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
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_CUSTOM", OCRPT_VARIABLE_CUSTOM);

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("MPFR_RNDN", MPFR_RNDN);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("MPFR_RNDZ", MPFR_RNDZ);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("MPFR_RNDU", MPFR_RNDU);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("MPFR_RNDD", MPFR_RNDD);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("MPFR_RNDA", MPFR_RNDA);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("MPFR_RNDF", MPFR_RNDF);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("MPFR_RNDNA", MPFR_RNDNA);

	return SUCCESS;
}

/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(opencreport) {
	/* Accounting for (global) array queries */
	ALLOC_HASHTABLE(php_opencreport_arrays);
	zend_hash_init(php_opencreport_arrays, 16, NULL, php_opencreport_array_dtor, 0);

	funcnames = NULL;
	funcnames_last = NULL;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(opencreport) {
	zend_hash_destroy(php_opencreport_arrays);
	FREE_HASHTABLE(php_opencreport_arrays);
	php_opencreport_arrays = NULL;

	ocrpt_list_free_deep(funcnames, ocrpt_mem_free);
	funcnames = NULL;
	funcnames_last = NULL;

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

	ds = ocrpt_datasource_add_mariadb(
				oo->o, ZSTR_VAL(source_name),
				host ? ZSTR_VAL(host) : NULL,
				NULL /* port */,
				dbname ? ZSTR_VAL(dbname) : NULL,
				user ? ZSTR_VAL(user) : NULL,
				password ? ZSTR_VAL(password) : NULL,
				NULL /* unix_socket */);
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

	ds = ocrpt_datasource_add_mariadb2(
				oo->o, ZSTR_VAL(source_name),
				option_file ? ZSTR_VAL(option_file) : NULL,
				group ? ZSTR_VAL(group) : NULL);
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

	ds = ocrpt_datasource_add_postgresql2(
				oo->o, ZSTR_VAL(source_name),
				connection_info ? ZSTR_VAL(connection_info) : NULL);
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

	ds = ocrpt_datasource_add_odbc(
				oo->o, ZSTR_VAL(source_name),
				dbname ? ZSTR_VAL(dbname) : NULL,
				user ? ZSTR_VAL(user) : NULL,
				password ? ZSTR_VAL(password) : NULL);
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

	ds = ocrpt_datasource_add_array(oo->o, ZSTR_VAL(source_name));
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

	ds = ocrpt_datasource_add_xml(oo->o, ZSTR_VAL(source_name));
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

	ds = ocrpt_datasource_add_csv(oo->o, ZSTR_VAL(source_name));
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
	void *array_x;
	int32_t rows, cols;

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

	if (ocrpt_datasource_is_array(ds)) {
		ocrpt_query_discover_array(ZSTR_VAL(array_or_file_or_sql), &array_x, &rows, &cols, NULL, NULL, NULL);
		q = ocrpt_query_add_array(ds, ZSTR_VAL(name), array_x, rows, cols, NULL, 0);
	} else if (ocrpt_datasource_is_csv(ds))
		q = ocrpt_query_add_csv(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), NULL, 0);
	else if (ocrpt_datasource_is_json(ds))
		q = ocrpt_query_add_json(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), NULL, 0);
	else if (ocrpt_datasource_is_xml(ds))
		q = ocrpt_query_add_xml(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), NULL, 0);
	else if (ocrpt_datasource_is_postgresql(ds))
		q = ocrpt_query_add_postgresql(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	else if (ocrpt_datasource_is_mariadb(ds))
		q = ocrpt_query_add_mariadb(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	else if (ocrpt_datasource_is_odbc(ds))
		q = ocrpt_query_add_odbc(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	else
		RETURN_NULL();

	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;
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

/* {{{ zend_function_entry */
static const zend_function_entry opencreport_functions[] = {
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
#if 0
	ZEND_FE(rlib_graph_add_bg_region, arginfo_rlib_graph_add_bg_region)
	ZEND_FE(rlib_graph_clear_bg_region, arginfo_rlib_graph_clear_bg_region)
	ZEND_FE(rlib_graph_set_x_minor_tick, arginfo_rlib_graph_set_x_minor_tick)
	ZEND_FE(rlib_graph_set_x_minor_tick_by_location, arginfo_rlib_graph_set_x_minor_tick_by_location)
	ZEND_FE(rlib_add_resultset_follower, arginfo_rlib_add_resultset_follower)
	ZEND_FE(rlib_add_resultset_follower_n_to_1, arginfo_rlib_add_resultset_follower_n_to_1)
	ZEND_FE(rlib_add_report, arginfo_rlib_add_report)
	ZEND_FE(rlib_add_report_from_buffer, arginfo_rlib_add_report_from_buffer)
	ZEND_FE(rlib_query_refresh, arginfo_rlib_query_refresh)
	ZEND_FE(rlib_signal_connect, arginfo_rlib_signal_connect)
	ZEND_FE(rlib_add_function, arginfo_rlib_add_function)
	ZEND_FE(rlib_set_output_format_from_text, arginfo_rlib_set_output_format_from_text)
	ZEND_FE(rlib_execute, arginfo_rlib_execute)
	ZEND_FE(rlib_spool, arginfo_rlib_spool)
	ZEND_FE(rlib_get_content_type, arginfo_rlib_get_content_type)
	ZEND_FE(rlib_add_parameter, arginfo_rlib_add_parameter)
	ZEND_FE(rlib_set_locale, arginfo_rlib_set_locale)
	ZEND_FE(rlib_bindtextdomain, arginfo_rlib_bindtextdomain)
	ZEND_FE(rlib_set_radix_character, arginfo_rlib_set_radix_character)
	ZEND_FE(rlib_version, arginfo_rlib_version)
	ZEND_FE(rlib_set_output_parameter, arginfo_rlib_set_output_parameter)
	ZEND_FE(rlib_set_datasource_encoding, arginfo_rlib_set_datasource_encoding)
	ZEND_FE(rlib_set_output_encoding, arginfo_rlib_set_output_encoding)
	ZEND_FE(rlib_compile_infix, arginfo_rlib_compile_infix)
	ZEND_FE(rlib_add_search_path, arginfo_rlib_add_search_path)
#endif
#ifdef PHP_FE_END
	PHP_FE_END
#else
	{NULL,NULL,NULL}
#endif
};
/* }}} */

/* {{{ opencreport_module_entry
 */
zend_module_entry opencreport_module_entry = {
	STANDARD_MODULE_HEADER,
	"opencreport",
	opencreport_functions,
	PHP_MINIT(opencreport),
	NULL,
	PHP_RINIT(opencreport),
	PHP_RSHUTDOWN(opencreport),
	PHP_MINFO(opencreport),
	PHP_OPENCREPORT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

ZEND_GET_MODULE(opencreport)
