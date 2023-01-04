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

/* Handlers */
static zend_object_handlers opencreport_object_handlers;
static zend_object_handlers opencreport_ds_object_handlers;
static zend_object_handlers opencreport_query_object_handlers;
static zend_object_handlers opencreport_query_result_object_handlers;
static zend_object_handlers opencreport_expr_object_handlers;
static zend_object_handlers opencreport_result_object_handlers;

/* Class entries */
zend_class_entry *opencreport_ce;
zend_class_entry *opencreport_ds_ce;
zend_class_entry *opencreport_query_ce;
zend_class_entry *opencreport_query_result_ce;
zend_class_entry *opencreport_expr_ce;
zend_class_entry *opencreport_result_ce;

static bool pointer_good(void *ptr) {
#if defined(_WIN32) || defined (_WIN64)
	MEMORY_BASIC_INFORMATION mbi = {0};
	if (VirtualQuery(p, &mbi, sizeof(mbi))) {
		DWORD mask = (PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY);
		bool g = (mbi.Protect & mask);
		/* check the page is not a guard page */
		if (mbi.Protect & (PAGE_GUARD|PAGE_NOACCESS))
			g = false;

		return g;
	}
	return false;
#else
	if (!ptr || (access(ptr, F_OK) == -1 &&  errno == EFAULT))
		return false;
	return true;
#endif
}

static zend_object *opencreport_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_object *intern = zend_object_alloc(sizeof(php_opencreport_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_object_handlers;

	intern->o = ocrpt_init();
	intern->assoc_objs = NULL;
	intern->assoc_objs_last = NULL;
	intern->funcnames = NULL;
	intern->funcnames_last = NULL;

	return &intern->zo;
}
/* }}} */

static void php_opencreport_kill_assoc_refs(zend_object *obj) {
	if (!pointer_good(obj->ce))
		return;

	if (instanceof_function(obj->ce, opencreport_ds_ce)) {
		php_opencreport_ds_object *dso = php_opencreport_ds_from_obj(obj);
		dso->oo = NULL;
	} else if (instanceof_function(obj->ce, opencreport_query_ce)) {
		php_opencreport_query_object *qo = php_opencreport_query_from_obj(obj);
		qo->oo = NULL;
	} else if (instanceof_function(obj->ce, opencreport_query_result_ce)) {
		php_opencreport_query_result_object *qro = php_opencreport_query_result_from_obj(obj);
		qro->oo = NULL;
	} else if (instanceof_function(obj->ce, opencreport_expr_ce)) {
		php_opencreport_expr_object *eo = php_opencreport_expr_from_obj(obj);
		eo->oo = NULL;
	} else if (instanceof_function(obj->ce, opencreport_result_ce)) {
		php_opencreport_result_object *ro = php_opencreport_result_from_obj(obj);
		ro->oo = NULL;
	}
}

static void opencreport_object_deinit(php_opencreport_object *oo) {
	ocrpt_list_free_deep(oo->assoc_objs, (ocrpt_mem_free_t)php_opencreport_kill_assoc_refs);
	oo->assoc_objs = NULL;
	oo->assoc_objs_last = NULL;

	ocrpt_list_free_deep(oo->funcnames, (ocrpt_mem_free_t)ocrpt_strfree);
	oo->funcnames = NULL;
	oo->funcnames_last = NULL;

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

	intern->assoc_objs = NULL;
	intern->assoc_objs_last = NULL;

	return &intern->zo;
}
/* }}} */

static void opencreport_ds_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_ds_object *dso = php_opencreport_ds_from_obj(object);
	php_opencreport_object *oo = dso->oo;

	if (oo)
		oo->assoc_objs = ocrpt_list_end_remove(oo->assoc_objs, &oo->assoc_objs_last, object);

	ocrpt_list_free_deep(dso->assoc_objs, (ocrpt_mem_free_t)php_opencreport_kill_assoc_refs);
	dso->assoc_objs = NULL;
	dso->assoc_objs_last = NULL;

	dso->ds = NULL;
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

static void php_opencreport_query_kill_assoc_refs(zend_object *obj) {
	if (instanceof_function(obj->ce, opencreport_query_result_ce)) {
		php_opencreport_query_result_object *qro = php_opencreport_query_result_from_obj(obj);
		qro->qo = NULL;
	}
}

static void opencreport_query_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_query_object *qo = php_opencreport_query_from_obj(object);

	php_opencreport_object *oo = qo->oo;
	if (oo)
		oo->assoc_objs = ocrpt_list_end_remove(oo->assoc_objs, &oo->assoc_objs_last, object);

	qo->q = NULL;
	qo->oo = NULL;

	ocrpt_list_free_deep(qo->assoc_objs, (ocrpt_mem_free_t)php_opencreport_query_kill_assoc_refs);
	qo->assoc_objs = NULL;
	qo->assoc_objs_last = NULL;

	zend_object_std_dtor(object);
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
	intern->qo = NULL;
	intern->oo = NULL;
	intern->cols = 0;
	intern->assoc_objs = NULL;
	intern->assoc_objs_last = NULL;

	return &intern->zo;
}
/* }}} */

static void php_opencreport_query_result_kill_assoc_refs(zend_object *obj) {
	if (instanceof_function(obj->ce, opencreport_result_ce)) {
		php_opencreport_result_object *ro = php_opencreport_result_from_obj(obj);
		ro->qro = NULL;
	}
}

static void opencreport_query_result_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_query_result_object *qro = php_opencreport_query_result_from_obj(object);

	php_opencreport_object *oo = qro->oo;
	if (pointer_good(oo))
		oo->assoc_objs = ocrpt_list_end_remove(oo->assoc_objs, &oo->assoc_objs_last, object);

	php_opencreport_query_object *qo = qro->qo;
	if (pointer_good(qo))
		qo->assoc_objs = ocrpt_list_end_remove(qo->assoc_objs, &qo->assoc_objs_last, object);

	qro->qr = NULL;
	qro->qo = NULL;
	qro->oo = NULL;
	qro->cols = 0;

	ocrpt_list_free_deep(qro->assoc_objs, (ocrpt_mem_free_t)php_opencreport_query_result_kill_assoc_refs);
	qro->assoc_objs = NULL;
	qro->assoc_objs_last = NULL;

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

	if (oo)
		oo->assoc_objs = ocrpt_list_end_remove(oo->assoc_objs, &oo->assoc_objs_last, object);

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
	if (oo)
		oo->assoc_objs = ocrpt_list_end_remove(oo->assoc_objs, &oo->assoc_objs_last, object);

	php_opencreport_query_result_object *qro = ro->qro;
	if (ro->has_query && qro)
		qro->assoc_objs = ocrpt_list_end_remove(qro->assoc_objs, &qro->assoc_objs_last, object);

	if (ro->r && !ro->freed_by_lib)
		ocrpt_result_free(ro->r);

	ro->r = NULL;
	ro->oo = NULL;
	ro->qro = NULL;

	zend_object_std_dtor(object);
}
/* }}} */

PHP_METHOD(opencreport, parse_xml) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *filename;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_execute(oo->o));
}

PHP_METHOD(opencreport, spool) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_spool(oo->o);
}

PHP_METHOD(opencreport, get_output) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_csv) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_json) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_xml) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_postgresql) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *host, *port, *dbname;
	zend_string *user, *password;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_postgresql2) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *connection_info;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_mariadb) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *host, *port, *dbname;
	zend_string *user, *password, *unix_socket;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_mariadb2) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *option_file, *group;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_odbc) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *dbname, *user, *password;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_add_odbc2) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	zend_string *connection_info;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR(connection_info);
	ZEND_PARSE_PARAMETERS_END();

	ds = ocrpt_datasource_add_odbc2(
				oo->o, ZSTR_VAL(source_name),
				ZSTR_VAL(connection_info));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, datasource_get) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *source_name;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

PHP_METHOD(opencreport, query_get) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *query_name;
	ocrpt_query *q;
	php_opencreport_query_object *qo;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	qo->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &qo->zo);
}

PHP_METHOD(opencreport, expr_parse) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *expr_string;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

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
		eo->has_parent = true;
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
	eo->has_parent = false;

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

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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

static ocrpt_result *php_opencreport_env_query(const char *env) {
	if (!env)
		return NULL;

	ocrpt_result *result = ocrpt_result_new();
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

PHP_METHOD(opencreport, env_get) {
	php_opencreport_result_object *ro;
	zend_string *var_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(var_name);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_result *r = ocrpt_env_get(ZSTR_VAL(var_name));
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->oo = NULL;
	ro->qro = NULL;
	ro->r = r;
	ro->has_parent = false;
	ro->has_query = false;
	ro->freed_by_lib = false;
}

PHP_METHOD(opencreport, add_search_path) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_string *path;

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, port, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_postgresql2, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, connection_info, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_mariadb, 0, 7, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, port, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, unix_socket, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_mariadb2, 0, 3, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, option_file, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, group, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_odbc, 0, 4, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add_odbc2, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, connection_info, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_get, 0, 1, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_query_get, 0, 1, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, query_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_parse, 0, 1, IS_ARRAY, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
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

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_env_get, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, var_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_add_search_path, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_canonicalize_path, 0, 1, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
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
	/* Function related methods */
	PHP_ME(opencreport, function_add, arginfo_opencreport_function_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Environment related methods */
	PHP_ME(opencreport, env_get, arginfo_opencreport_env_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	/* File handling related methods */
	PHP_ME(opencreport, add_search_path, arginfo_opencreport_add_search_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, canonicalize_path, arginfo_opencreport_canonicalize_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	PHP_FE_END
};

PHP_METHOD(opencreport_ds, query_add) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *ds = Z_OPENCREPORT_DS_P(object);
	zend_string *name, *array_or_file_or_sql, *coltypes;
	ocrpt_query *q;
	php_opencreport_query_object *qo;
	void *array_x, *types_x = NULL;
	int32_t rows, cols, types_cols = 0;

	if (!ds->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(name);
		Z_PARAM_STR(array_or_file_or_sql);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(coltypes, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	if (ocrpt_datasource_is_array(ds->ds)) {
		if (coltypes && ZSTR_LEN(coltypes) > 0) {
			ocrpt_query_discover_array(ZSTR_VAL(array_or_file_or_sql), &array_x, &rows, &cols, ZSTR_VAL(coltypes), &types_x, &types_cols);
		} else {
			ocrpt_query_discover_array(ZSTR_VAL(array_or_file_or_sql), &array_x, &rows, &cols, NULL, NULL, NULL);
		}

		q = ocrpt_query_add_array(ds->ds, ZSTR_VAL(name), array_x, rows, cols, types_x, types_cols);
	} else if (ocrpt_datasource_is_csv(ds->ds)) {
		if (coltypes && ZSTR_LEN(coltypes) > 0)
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(coltypes), &types_x, &types_cols);

		q = ocrpt_query_add_csv(ds->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), types_x, types_cols);
	} else if (ocrpt_datasource_is_json(ds->ds)) {
		if (coltypes && ZSTR_LEN(coltypes) > 0)
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(coltypes), &types_x, &types_cols);

		q = ocrpt_query_add_json(ds->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), types_x, types_cols);
	} else if (ocrpt_datasource_is_xml(ds->ds)) {
		if (coltypes && ZSTR_LEN(coltypes) > 0)
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, ZSTR_VAL(coltypes), &types_x, &types_cols);

		q = ocrpt_query_add_xml(ds->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), types_x, types_cols);
	} else if (ocrpt_datasource_is_postgresql(ds->ds)) {
		q = ocrpt_query_add_postgresql(ds->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	} else if (ocrpt_datasource_is_mariadb(ds->ds)) {
		q = ocrpt_query_add_mariadb(ds->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	} else if (ocrpt_datasource_is_odbc(ds->ds)) {
		q = ocrpt_query_add_odbc(ds->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	} else
		RETURN_NULL();

	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;

	php_opencreport_object *oo = ds->oo;
	qo->oo = ds->oo;
	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &qo->zo);
}

PHP_METHOD(opencreport_ds, set_encoding) {
	zval *object = ZEND_THIS;
	php_opencreport_ds_object *ds = Z_OPENCREPORT_DS_P(object);
	zend_string *encoding;

	if (!ds->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(encoding);
	ZEND_PARSE_PARAMETERS_END();

	ocrpt_datasource_set_encoding(ds->ds, ZSTR_VAL(encoding));
}

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_ds_query_add, 0, 3, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, array_or_file_or_sql, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, types, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_ds_set_encoding, 0, 1, IS_VOID, 1)
ZEND_ARG_TYPE_INFO(0, encoding, IS_STRING, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_ds_class_methods[] = {
	PHP_ME(opencreport_ds, query_add, arginfo_opencreport_ds_query_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_ds, set_encoding, arginfo_opencreport_ds_set_encoding, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_query, get_result) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	if (!qo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qo->q) {
		zend_throw_error(NULL, "This OpenCReport\\Query object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	int32_t cols;
	ocrpt_query_result *qr = ocrpt_query_get_result(qo->q, &cols);

	if (!qr)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_result_ce);
	php_opencreport_query_result_object *qro = Z_OPENCREPORT_QUERY_RESULT_P(return_value);
	qro->qo = qo;
	qro->cols = cols;
	qro->qr = qr;
	qro->assoc_objs = NULL;
	qro->assoc_objs_last = NULL;

	php_opencreport_object *oo = qo->oo;
	qro->oo = qo->oo;
	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &qro->zo);
	qo->assoc_objs = ocrpt_list_end_append(qo->assoc_objs, &qo->assoc_objs_last, &qro->zo);
}

PHP_METHOD(opencreport_query, navigate_start) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	if (!qo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qo->q) {
		zend_throw_error(NULL, "This OpenCReport\\Query object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	if (!qo->q)
		RETURN_NULL();

	ocrpt_query_navigate_start(qo->q);
}

PHP_METHOD(opencreport_query, navigate_next) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	if (!qo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qo->q) {
		zend_throw_error(NULL, "This OpenCReport\\Query object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	if (!qo->q)
		RETURN_NULL();

	RETURN_BOOL(ocrpt_query_navigate_next(qo->q));
}

PHP_METHOD(opencreport_query, add_follower) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);
	zval *fobj;

	if (!qo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qo->q) {
		zend_throw_error(NULL, "This OpenCReport\\Query object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(fobj, opencreport_query_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_query_object *fo = Z_OPENCREPORT_QUERY_P(fobj);
	if (!fo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qo->q || !fo->q)
		RETURN_NULL();

	RETURN_BOOL(ocrpt_query_add_follower(qo->q, fo->q));
}

PHP_METHOD(opencreport_query, add_follower_n_to_1) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);
	zval *fobj;
	zval *mobj;

	if (!qo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qo->q) {
		zend_throw_error(NULL, "This OpenCReport\\Query object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(fobj, opencreport_query_ce);
		Z_PARAM_OBJECT_OF_CLASS(mobj, opencreport_expr_ce);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_query_object *fo = Z_OPENCREPORT_QUERY_P(fobj);
	if (!fo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(mobj);
	if (!eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qo->q || !fo->q || !eo->e)
		RETURN_NULL();

	RETURN_BOOL(ocrpt_query_add_follower_n_to_1(qo->q, fo->q, eo->e));
}

PHP_METHOD(opencreport_query, free) {
	zval *object = ZEND_THIS;
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	if (!qo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qo->q) {
		zend_throw_error(NULL, "This OpenCReport\\Query object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_free(qo->q);
	qo->q = NULL;

	php_opencreport_object *oo = qo->oo;
	if (pointer_good(oo))
		oo->assoc_objs = ocrpt_list_end_remove(oo->assoc_objs, &oo->assoc_objs_last, object);

	qo->oo = NULL;

	ocrpt_list_free_deep(qo->assoc_objs, (ocrpt_mem_free_t)php_opencreport_query_kill_assoc_refs);
	qo->assoc_objs = NULL;
	qo->assoc_objs_last = NULL;
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

	if (!qr->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qr->qo) {
		zend_throw_error(NULL, "Parent OpenCReport\\Query object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(qr->cols);
}

PHP_METHOD(opencreport_query_result, column_name) {
	zval *object = ZEND_THIS;
	php_opencreport_query_result_object *qr = Z_OPENCREPORT_QUERY_RESULT_P(object);
	zend_long index;

	if (!qr->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qr->qo) {
		zend_throw_error(NULL, "Parent OpenCReport\\Query object was destroyed");
		RETURN_THROWS();
	}

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

	if (!qr->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!qr->qo) {
		zend_throw_error(NULL, "Parent OpenCReport\\Query object was destroyed");
		RETURN_THROWS();
	}

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
	php_opencreport_object *oo = qr->oo;
	ro->r = r;
	ro->qro = qr;
	ro->oo = oo;
	ro->has_parent = !!oo;
	ro->has_query = true;
	ro->freed_by_lib = true;

	qr->assoc_objs = ocrpt_list_end_append(qr->assoc_objs, &qr->assoc_objs_last, &ro->zo);
}

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_result_columns, 0, 0, IS_LONG, 1)
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

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (!eo->e) {
		zend_throw_error(NULL, "This OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	php_opencreport_object *oo = eo->oo;
	if (pointer_good(oo))
		oo->assoc_objs = ocrpt_list_end_remove(oo->assoc_objs, &oo->assoc_objs_last, object);

	ocrpt_expr_free(eo->e);
	eo->e = NULL;
	eo->oo = NULL;
}

PHP_METHOD(opencreport_expr, print) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_print(eo->e);
}

PHP_METHOD(opencreport_expr, nodes) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_nodes(eo->e));
}

PHP_METHOD(opencreport_expr, optimize) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_optimize(eo->e);
}

PHP_METHOD(opencreport_expr, resolve) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_resolve(eo->e);
}

PHP_METHOD(opencreport_expr, eval) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	php_opencreport_result_object *ro;

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result *r = ocrpt_expr_eval(eo->e);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	ro = Z_OPENCREPORT_RESULT_P(return_value);
	php_opencreport_object *oo = eo->oo;
	ro->r = r;
	ro->oo = oo;
	ro->has_parent = !!oo;
	ro->freed_by_lib = true;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &ro->zo);
}

PHP_METHOD(opencreport_expr, set_string_value) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_string *value;

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_get_num_operands(eo->e));
}

PHP_METHOD(opencreport_expr, operand_get_result) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_long opidx;

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	php_opencreport_object *oo = eo->oo;
	ro->r = r;
	ro->oo = oo;
	ro->has_parent = !!oo;
	ro->freed_by_lib = true;

	if (oo)
		oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &ro->zo);
}

PHP_METHOD(opencreport_expr, cmp_results) {
	zval *object = ZEND_THIS;
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (eo->has_parent && !eo->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_expr_cmp_results(eo->e));
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

static const zend_function_entry opencreport_expr_class_methods[] = {
	PHP_ME(opencreport_expr, free, arginfo_opencreport_expr_free, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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
	PHP_ME(opencreport_expr, cmp_results, arginfo_opencreport_expr_cmp_results, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_result, print) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	if (ro->has_parent && !ro->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (ro->has_query && !ro->qro) {
		zend_throw_error(NULL, "Parent OpenCReport\\QueryResult object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result_print(ro->r);
}

PHP_METHOD(opencreport_result, get_type) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	if (ro->has_parent && !ro->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (ro->has_query && !ro->qro) {
		zend_throw_error(NULL, "Parent OpenCReport\\QueryResult object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_result_get_type(ro->r));
}

PHP_METHOD(opencreport_result, is_null) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	if (ro->has_parent && !ro->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (ro->has_query && !ro->qro) {
		zend_throw_error(NULL, "Parent OpenCReport\\QueryResult object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isnull(ro->r));
}

PHP_METHOD(opencreport_result, is_string) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	if (ro->has_parent && !ro->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (ro->has_query && !ro->qro) {
		zend_throw_error(NULL, "Parent OpenCReport\\QueryResult object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isstring(ro->r));
}

PHP_METHOD(opencreport_result, is_number) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	if (ro->has_parent && !ro->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (ro->has_query && !ro->qro) {
		zend_throw_error(NULL, "Parent OpenCReport\\QueryResult object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isnumber(ro->r));
}

PHP_METHOD(opencreport_result, get_string) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	if (ro->has_parent && !ro->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (ro->has_query && !ro->qro) {
		zend_throw_error(NULL, "Parent OpenCReport\\QueryResult object was destroyed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ocrpt_result_isstring(ro->r))
		RETURN_NULL();

	ocrpt_string *s = ocrpt_result_get_string(ro->r);
	RETURN_STRINGL(s->str, s->len);
}

PHP_METHOD(opencreport_result, get_number) {
	zval *object = ZEND_THIS;
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);
	zend_string *format = NULL;

	if (ro->has_parent && !ro->oo) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	if (ro->has_query && !ro->qro) {
		zend_throw_error(NULL, "Parent OpenCReport\\QueryResult object was destroyed");
		RETURN_THROWS();
	}

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
	PHP_ME(opencreport_result, print, arginfo_opencreport_result_print, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, get_type, arginfo_opencreport_result_get_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, is_null, arginfo_opencreport_result_is_null, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, is_string, arginfo_opencreport_result_is_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, is_number, arginfo_opencreport_result_is_number, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, get_string, arginfo_opencreport_result_get_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_result, get_number, arginfo_opencreport_result_get_number, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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

	php_error_docref(NULL, E_ERROR, "%s", s->str);

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
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "DataSource", opencreport_ds_class_methods);
	ce.create_object = opencreport_ds_object_new;
	opencreport_ds_object_handlers.offset = XtOffsetOf(php_opencreport_ds_object, zo);
	opencreport_ds_object_handlers.clone_obj = NULL;
	opencreport_ds_object_handlers.free_obj = opencreport_ds_object_free;
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
	opencreport_query_object_handlers.free_obj = opencreport_query_object_free;
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
	opencreport_query_result_object_handlers.free_obj = opencreport_query_result_object_free;
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
	opencreport_expr_object_handlers.free_obj = opencreport_expr_object_free;
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

ZEND_FUNCTION(rlib_init) {
	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_ce);
}

ZEND_FUNCTION(rlib_free) {
	zval *obj;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(obj, opencreport_ce)
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(obj);

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(host, 1, 0);
		Z_PARAM_STR_EX(user, 1, 0);
		Z_PARAM_STR_EX(password, 1, 0);
		Z_PARAM_STR_EX(dbname, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

ZEND_FUNCTION(rlib_add_datasource_mysql_from_group) {
	zval *object;
	zend_string *source_name;
	zend_string *option_file, *group;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 4)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(group, 1, 0);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(option_file, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
}

ZEND_FUNCTION(rlib_add_datasource_postgres) {
	zval *object;
	zend_string *source_name;
	zend_string *connection_info;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(connection_info, 1, 0);
	ZEND_PARSE_PARAMETERS_END();

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ds = ocrpt_datasource_add_array(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ds = ocrpt_datasource_add_xml(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
		RETURN_THROWS();
	}

	ds = ocrpt_datasource_add_csv(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
	dso->oo = oo;

	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &dso->zo);
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
		zend_throw_error(NULL, "Parent OpenCReport object was destroyed");
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

	qo->oo = oo;
	oo->assoc_objs = ocrpt_list_end_append(oo->assoc_objs, &oo->assoc_objs_last, &qo->zo);
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
