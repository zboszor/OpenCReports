/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_result_object_handlers;

zend_class_entry *opencreport_result_ce;

static void opencreport_result_object_free(zend_object *object);

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_result_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_result_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_result_object *intern = zend_object_alloc(sizeof(php_opencreport_result_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) opencreport_result_object_free, NULL TSRMLS_CC);
	retval.handlers = &opencreport_result_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_result_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

static void opencreport_result_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_result_object *ro = php_opencreport_result_from_obj(object);

	if (ro->r && !ro->freed_by_lib)
		ocrpt_result_free(ro->r);

	ro->r = NULL;

	zend_object_std_dtor(object);
#if PHP_VERSION_ID < 70000
	efree(ro);
#endif
}
/* }}} */

PHP_METHOD(opencreport_result, free) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ro->freed_by_lib)
		ocrpt_result_free(ro->r);

	ro->r = NULL;
}

PHP_METHOD(opencreport_result, copy) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);
	zval *src_result;

	if (!ro->r) {
		zend_throw_error(NULL, "OpenCReport\\Result object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(src_result, opencreport_result_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &src_result, opencreport_result_ce) == FAILURE)
		return;
#endif

	php_opencreport_result_object *src_ro = Z_OPENCREPORT_RESULT_P(src_result);
	if (!src_ro->r) {
		zend_throw_error(NULL, "OpenCReport\\Result source object was freed");
		RETURN_THROWS();
	}

	ocrpt_result_copy(ro->r, src_ro->r);
}

PHP_METHOD(opencreport_result, print) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result_print(ro->r);
}

PHP_METHOD(opencreport_result, get_type) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_result_get_type(ro->r));
}

PHP_METHOD(opencreport_result, is_null) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isnull(ro->r));
}

PHP_METHOD(opencreport_result, is_string) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isstring(ro->r));
}

PHP_METHOD(opencreport_result, is_number) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_result_isnumber(ro->r));
}

PHP_METHOD(opencreport_result, get_string) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_string *s = ocrpt_result_get_string(ro->r);
	if (!s)
		RETURN_NULL();

	OCRPT_RETURN_STRINGL(s->str, s->len);
}

PHP_METHOD(opencreport_result, get_number) {
	zval *object = getThis();
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *format = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(format, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *format = NULL;
	int format_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!", &format, &format_len) == FAILURE)
		return;
#endif

	char *fmt = format ? ZSTR_VAL(format) : "%RF";
	mpfr_ptr number = ocrpt_result_get_number(ro->r);
	if (!number)
		RETURN_NULL();

	size_t len = mpfr_snprintf(NULL, 0, fmt, number);
	char *retval = emalloc(len + 1);
	mpfr_snprintf(retval, len + 1, fmt, number);

	OCRPT_RETURN_STRINGL(retval, len);
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_copy, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, src_result, OpenCReport\\Result, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_print, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_get_type, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_is_null, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_is_string, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_is_number, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_get_string, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_result_get_number, 0, 0, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, format, IS_STRING, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_result_free NULL
#define arginfo_opencreport_result_copy NULL
#define arginfo_opencreport_result_print NULL
#define arginfo_opencreport_result_get_type NULL
#define arginfo_opencreport_result_is_null NULL
#define arginfo_opencreport_result_is_string NULL
#define arginfo_opencreport_result_is_number NULL
#define arginfo_opencreport_result_get_string NULL
#define arginfo_opencreport_result_get_number NULL

#endif

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

void register_opencreport_result_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_result_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Result", opencreport_result_class_methods);
	ce.create_object = opencreport_result_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_result_object_handlers.offset = XtOffsetOf(php_opencreport_result_object, zo);
	opencreport_result_object_handlers.free_obj = opencreport_result_object_free;
#endif
	opencreport_result_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_result_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_result_ce = zend_register_internal_class(&ce);
	opencreport_result_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_result_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_result_ce->serialize = zend_class_serialize_deny;
	opencreport_result_ce->unserialize = zend_class_unserialize_deny;
#endif

}
