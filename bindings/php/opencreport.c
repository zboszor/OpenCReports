/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "zend_interfaces.h"
#include "php_opencreport.h"
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

/* {{{ zend_function_entry */
static const zend_function_entry opencreport_functions[] = {
#ifdef  PHP_FE_END
	PHP_FE_END
#else
	{NULL,NULL,NULL}
#endif
};
/* }}} */

/* {{{ function prototypes */
static PHP_MINIT_FUNCTION(opencreport);
static PHP_RINIT_FUNCTION(opencreport);
static PHP_RSHUTDOWN_FUNCTION(opencreport);
static PHP_MINFO_FUNCTION(opencreport);
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

/* Handlers */
static zend_object_handlers opencreport_object_handlers;
static zend_object_handlers opencreport_ds_object_handlers;
static zend_object_handlers opencreport_query_object_handlers;
static zend_object_handlers opencreport_expr_object_handlers;
static zend_object_handlers opencreport_result_object_handlers;

/* Class entries */
zend_class_entry *opencreport_ce;
zend_class_entry *opencreport_ds_ce;
zend_class_entry *opencreport_query_ce;
zend_class_entry *opencreport_expr_ce;
zend_class_entry *opencreport_result_ce;

static zend_object *opencreport_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_object *intern;

	intern = zend_object_alloc(sizeof(php_opencreport_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_object_handlers;

	intern->o = ocrpt_init();
	intern->assoc_objs = NULL;
	intern->assoc_objs_last = NULL;
	intern->funcnames = NULL;
	intern->funcnames_last = NULL;
	intern->in_dtor = false;

	return &intern->zo;
}
/* }}} */

static void opencreport_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_object *oo = php_opencreport_from_obj(object);
	ocrpt_list *l;

	if (!oo->o)
		return;

	oo->in_dtor = true;

	for (l = oo->assoc_objs; l; l = ocrpt_list_next(l)) {
		zend_object *obj = ocrpt_list_get_data(l);
		zval zobj;

		ZVAL_OBJ(&zobj, obj);
		zval_ptr_dtor(&zobj);
	}

	oo->in_dtor = false;

	for (l = oo->funcnames; l; l = ocrpt_list_next(l)) {
		char *s = ocrpt_list_get_data(l);
		ocrpt_strfree(s);
	}

	ocrpt_list_free(oo->assoc_objs);
	oo->assoc_objs = NULL;
	oo->assoc_objs_last = NULL;
	ocrpt_list_free(oo->funcnames);
	oo->funcnames = NULL;
	oo->funcnames_last = NULL;

	ocrpt_free(oo->o);
	oo->o = NULL;

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

static void opencreport_ds_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_ds_object *dso = php_opencreport_ds_from_obj(object);
	php_opencreport_object *oo = dso->oo;

	if (oo && !oo->in_dtor)
		oo->assoc_objs = ocrpt_list_remove(oo->assoc_objs, object);

	dso->oo = NULL;

	zend_object_std_dtor(object);
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

static void opencreport_query_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_query_object *qo = php_opencreport_query_from_obj(object);
	php_opencreport_object *oo = qo->oo;

	if (oo && !oo->in_dtor)
		oo->assoc_objs = ocrpt_list_remove(oo->assoc_objs, object);

	qo->oo = NULL;

	zend_object_std_dtor(object);
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

static void opencreport_expr_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_expr_object *eo = php_opencreport_expr_from_obj(object);
	php_opencreport_object *oo = eo->oo;

	if (oo && !oo->in_dtor)
		oo->assoc_objs = ocrpt_list_remove(oo->assoc_objs, object);

	if (!eo->freed_by_lib)
		ocrpt_expr_free(eo->e);
	eo->e = NULL;
	eo->oo = NULL;

	zend_object_std_dtor(object);
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
	php_opencreport_object *oo = ro->oo;

	if (oo && !oo->in_dtor)
		oo->assoc_objs = ocrpt_list_remove(oo->assoc_objs, object);

	ro->r = NULL;
	ro->oo = NULL;

	zend_object_std_dtor(object);
}
/* }}} */

PHP_METHOD(opencreport, parse_xml) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *filename;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(filename);
	ZEND_PARSE_PARAMETERS_END();

	RETURN_BOOL(ocrpt_parse_xml(oo->o, ZSTR_VAL(filename)));
}

PHP_METHOD(opencreport, set_output_format) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_long format;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(format);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_output_format(oo->o, format);
}

PHP_METHOD(opencreport, execute) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_execute(oo->o));
}

PHP_METHOD(opencreport, spool) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_spool(oo->o);
}

PHP_METHOD(opencreport, get_output) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

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

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(prec);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_set_numeric_precision_bits(oo->o, (mpfr_prec_t)prec);
}

PHP_METHOD(opencreport, set_rounding_mode) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_long mode;

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

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_array(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_csv) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_csv(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_json) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_json(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_xml) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_xml(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, expr_parse) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(expr_string);
	ZEND_PARSE_PARAMETERS_END();

	char *err = NULL;
	ocrpt_expr *e = ocrpt_expr_parse(oo->o, ZSTR_VAL(expr_string), &err);

	array_init(return_value);

	zval tmp;

	if (e) {
		object_init_ex(&tmp, opencreport_expr_ce);
		php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(&tmp);
		eo->e = e;
		eo->oo = oo;
		eo->freed_by_lib = false;

		oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &eo->zo);
	} else {
		ZVAL_NULL(&tmp);
	}

	zend_hash_index_add_new(Z_ARRVAL_P(return_value), 0, &tmp);

	if (err) {
		zend_string *errs = zend_string_init(err, strlen(err), 0);
		ZVAL_STR(&tmp, errs);
		ocrpt_strfree(err);
	} else {
		ZVAL_NULL(&tmp);
	}

	zend_hash_index_add_new(Z_ARRVAL_P(return_value), 1, &tmp);
}

OCRPT_STATIC_FUNCTION(opencreport_default_function) {
	char *fname = user_data;
	zval zfname;

	ocrpt_expr_make_error_result(e, "not implemented");

	ZVAL_STRING(&zfname, fname);

	zval retval;
	zval *params = emalloc(sizeof(zval));

	object_init_ex(&params[0], opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(&params[0]);
	eo->e = e;
	eo->oo = NULL;
	eo->freed_by_lib = true;

#if PHP_MAJOR_VERSION >= 8
	if (call_user_function(CG(function_table), NULL, &zfname, &retval, 1, params) == FAILURE)
#else
	if (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 1, params, 0, NULL TSRMLS_CC) == FAILURE)
#endif
		return;

	zend_object_std_dtor(&eo->zo);

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

PHP_METHOD(opencreport, function_add) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_func_name;
	zend_string *zend_func_name;
	zend_long n_ops;
	zend_bool commutative, associative, left_associative, dont_optimize;

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

PHP_METHOD(opencreport, add_search_path) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *path;

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

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_add_search_path, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_canonicalize_path, 0, 1, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_parse, 0, 1, IS_ARRAY, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_function_add, 7, 7, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, expr_func_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, zend_func_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, n_ops, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, commutative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, associative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, left_associative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, dont_optimize, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_class_methods[] = {
	/*
	 * High level API
	 */
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
	/* Expression related methods */
	PHP_ME(opencreport, expr_parse, arginfo_opencreport_expr_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Function related methods */
	PHP_ME(opencreport, function_add, arginfo_opencreport_function_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* File handling related methods */
	PHP_ME(opencreport, add_search_path, arginfo_opencreport_add_search_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, canonicalize_path, arginfo_opencreport_canonicalize_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	PHP_FE_END
};

PHP_METHOD(opencreport_ds, query_add_array) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *ds = Z_OPENCREPORT_DS_P(object);
	zend_string *name, *array, *types;
	ocrpt_query *q;
	php_opencreport_query_object *qo;
	void *array_x, *types_x = NULL;
	int32_t rows, cols, types_cols = 0;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(name);
		Z_PARAM_STR(array);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_OR_NULL(types);
	ZEND_PARSE_PARAMETERS_END();

	if (types && ZSTR_LEN(types) > 0) {
		ocrpt_query_discover_array(ZSTR_VAL(array), &array_x, &rows, &cols, ZSTR_VAL(types), &types_x, &types_cols);
	} else {
		ocrpt_query_discover_array(ZSTR_VAL(array), &array_x, &rows, &cols, NULL, NULL, NULL);
	}

	q = ocrpt_query_add_array(ds->ds, ZSTR_VAL(name), array_x, rows, cols, types_x, types_cols);
	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;

	php_opencreport_object *oo = ds->oo;
	qo->oo = ds->oo;
	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &qo->zo);
}

PHP_METHOD(opencreport_ds, query_add_csv) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *ds = Z_OPENCREPORT_DS_P(object);
	zend_string *name, *filename, *types;
	ocrpt_query *q;
	php_opencreport_query_object *qo;
	void *types_x = NULL;
	int32_t types_cols = 0;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(name);
		Z_PARAM_STR(filename);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_OR_NULL(types);
	ZEND_PARSE_PARAMETERS_END();

	if (types && ZSTR_LEN(types) > 0)
		ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(types), &types_x, &types_cols);

	q = ocrpt_query_add_csv(ds->ds, ZSTR_VAL(name), ZSTR_VAL(filename), types_x, types_cols);
	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;

	php_opencreport_object *oo = ds->oo;
	qo->oo = ds->oo;
	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &qo->zo);
}

PHP_METHOD(opencreport_ds, query_add_json) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *ds = Z_OPENCREPORT_DS_P(object);
	zend_string *name, *filename, *types;
	ocrpt_query *q;
	php_opencreport_query_object *qo;
	void *types_x = NULL;
	int32_t types_cols = 0;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(name);
		Z_PARAM_STR(filename);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_OR_NULL(types);
	ZEND_PARSE_PARAMETERS_END();

	if (types && ZSTR_LEN(types) > 0)
		ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(types), &types_x, &types_cols);

	q = ocrpt_query_add_json(ds->ds, ZSTR_VAL(name), ZSTR_VAL(filename), types_x, types_cols);
	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;

	php_opencreport_object *oo = ds->oo;
	qo->oo = ds->oo;
	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &qo->zo);
}

PHP_METHOD(opencreport_ds, query_add_xml) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *ds = Z_OPENCREPORT_DS_P(object);
	zend_string *name, *filename, *types;
	ocrpt_query *q;
	php_opencreport_query_object *qo;
	void *types_x = NULL;
	int32_t types_cols = 0;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(name);
		Z_PARAM_STR(filename);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_OR_NULL(types);
	ZEND_PARSE_PARAMETERS_END();

	if (types && ZSTR_LEN(types) > 0)
		ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(types), &types_x, &types_cols);

	q = ocrpt_query_add_xml(ds->ds, ZSTR_VAL(name), ZSTR_VAL(filename), types_x, types_cols);
	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;

	php_opencreport_object *oo = ds->oo;
	qo->oo = ds->oo;
	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &qo->zo);
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_ds_query_add_array, 0, 3, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, array, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, types, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_ds_query_add_file, 0, 3, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, types, IS_STRING, 1)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_ds_class_methods[] = {
	PHP_ME(opencreport_ds, query_add_array, arginfo_opencreport_ds_query_add_array, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_ds, query_add_csv, arginfo_opencreport_ds_query_add_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_ds, query_add_json, arginfo_opencreport_ds_query_add_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_ds, query_add_xml, arginfo_opencreport_ds_query_add_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

static const zend_function_entry opencreport_query_class_methods[] = {
	PHP_FE_END
};

PHP_METHOD(opencreport_expr, print) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_print(e->e);
}

PHP_METHOD(opencreport_expr, nodes) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_nodes(e->e));
}

PHP_METHOD(opencreport_expr, optimize) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_optimize(e->e);
}

PHP_METHOD(opencreport_expr, resolve) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_resolve(e->e);
}

PHP_METHOD(opencreport_expr, eval) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);
	php_opencreport_result_object *ro;

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result *r = ocrpt_expr_eval(e->e);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->r = r;

	php_opencreport_object *oo = e->oo;
	ro->oo = e->oo;
	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &ro->zo);
}

PHP_METHOD(opencreport_expr, set_string_value) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);
	zend_string *value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_string_value(e->e, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, set_long_value) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);
	zend_long value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_long_value(e->e, value);
}

PHP_METHOD(opencreport_expr, set_double_value) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);
	double value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_DOUBLE(value);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_expr_set_double_value(e->e, value);
}

PHP_METHOD(opencreport_expr, get_num_operands) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_get_num_operands(e->e));
}

PHP_METHOD(opencreport_expr, operand_get_result) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *e = Z_OPENCREPORT_EXPR_P(object);
	zend_long opidx;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(opidx);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_result *r = ocrpt_expr_operand_get_result(e->e, opidx);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->r = r;

	php_opencreport_object *oo = e->oo;
	ro->oo = e->oo;
	if (oo)
		oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &ro->zo);
}

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

static const zend_function_entry opencreport_expr_class_methods[] = {
	PHP_ME(opencreport_expr, print, arginfo_opencreport_expr_print, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, nodes, arginfo_opencreport_expr_nodes, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, optimize, arginfo_opencreport_expr_optimize, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, resolve, arginfo_opencreport_expr_resolve, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, eval, arginfo_opencreport_expr_eval, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_string_value, arginfo_opencreport_expr_set_string_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_long_value, arginfo_opencreport_expr_set_long_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_double_value, arginfo_opencreport_expr_set_double_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_num_operands, arginfo_opencreport_expr_get_num_operands, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, operand_get_result, arginfo_opencreport_expr_operand_get_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_result, print) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *r = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result_print(r->r);
}

PHP_METHOD(opencreport_result, get_type) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *r = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_result_get_type(r->r));
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_print, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_get_type, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_result_class_methods[] = {
	PHP_ME(opencreport_result, print, arginfo_opencreport_result_print, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, get_type, arginfo_opencreport_result_get_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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
	}
}

/* {{{ PHP_MINIT_FUNCTION */
static PHP_MINIT_FUNCTION(opencreport)
{
	zend_class_entry ce;

	ocrpt_query_set_discover_func(php_opencreport_query_discover_array);

	memcpy(&opencreport_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_CLASS_ENTRY(ce, "OpenCReport", opencreport_class_methods);
	ce.create_object = opencreport_object_new;
	opencreport_object_handlers.offset = XtOffsetOf(php_opencreport_object, zo);
	opencreport_object_handlers.clone_obj = NULL;
	opencreport_object_handlers.free_obj = opencreport_object_free;
	opencreport_ce = zend_register_internal_class(&ce);
#if PHP_VERSION_ID >= 80100
	opencreport_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_ce->serialize = zend_class_serialize_deny;
	opencreport_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_ds_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "DataSource", opencreport_ds_class_methods);
	ce.create_object = opencreport_ds_object_new;
	opencreport_ds_object_handlers.offset = XtOffsetOf(php_opencreport_ds_object, zo);
	opencreport_ds_object_handlers.clone_obj = NULL;
	opencreport_ds_object_handlers.free_obj = opencreport_ds_object_free;
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
	opencreport_query_object_handlers.free_obj = opencreport_query_object_free;
	opencreport_query_ce = zend_register_internal_class(&ce);
	opencreport_query_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_query_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_query_ce->serialize = zend_class_serialize_deny;
	opencreport_query_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_expr_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Expr", opencreport_expr_class_methods);
	ce.create_object = opencreport_expr_object_new;
	opencreport_expr_object_handlers.offset = XtOffsetOf(php_opencreport_expr_object, zo);
	opencreport_expr_object_handlers.clone_obj = NULL;
	opencreport_expr_object_handlers.free_obj = opencreport_expr_object_free;
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
	opencreport_result_ce = zend_register_internal_class(&ce);
	opencreport_result_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_result_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_result_ce->serialize = zend_class_serialize_deny;
	opencreport_result_ce->unserialize = zend_class_unserialize_deny;
#endif

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

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(opencreport) {
	zend_hash_destroy(php_opencreport_arrays);
	FREE_HASHTABLE(php_opencreport_arrays);
	php_opencreport_arrays = NULL;

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
