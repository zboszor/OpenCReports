/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_query_result_object_handlers;

zend_class_entry *opencreport_query_result_ce;

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_query_result_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_query_result_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_query_result_object *intern = zend_object_alloc(sizeof(php_opencreport_query_result_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_query_result_object_handlers;
#else
	intern->zo.handlers = &opencreport_query_result_object_handlers;
#endif

	intern->qr = NULL;
	intern->cols = 0;

#if PHP_VERSION_ID < 70000
	return retval;
#else
	return &intern->zo;
#endif
}
/* }}} */

PHP_METHOD(opencreport_query_result, columns) {
	zval *object = getThis();
	php_opencreport_query_result_object *qr = Z_OPENCREPORT_QUERY_RESULT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(qr->cols);
}

PHP_METHOD(opencreport_query_result, column_name) {
	zval *object = getThis();
	php_opencreport_query_result_object *qr = Z_OPENCREPORT_QUERY_RESULT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_long index;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(index);
	ZEND_PARSE_PARAMETERS_END();
#else
	long index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) == FAILURE)
		return;
#endif

	if (index < 0 || index >= qr->cols)
		RETURN_NULL();

	OCRPT_RETURN_STRING(ocrpt_query_result_column_name(qr->qr, index));
}

PHP_METHOD(opencreport_query_result, column_result) {
	zval *object = getThis();
	php_opencreport_query_result_object *qr = Z_OPENCREPORT_QUERY_RESULT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_long index;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(index);
	ZEND_PARSE_PARAMETERS_END();
#else
	long index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) == FAILURE)
		return;
#endif

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

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_result_columns, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_result_column_name, 0, 1, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_query_result_column_result, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_query_result_columns NULL
#define arginfo_opencreport_query_result_column_name NULL
#define arginfo_opencreport_query_result_column_result NULL

#endif

static const zend_function_entry opencreport_query_result_class_methods[] = {
	PHP_ME(opencreport_query_result, columns, arginfo_opencreport_query_result_columns, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query_result, column_name, arginfo_opencreport_query_result_column_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_query_result, column_result, arginfo_opencreport_query_result_column_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

void register_opencreport_query_result_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_query_result_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "QueryResult", opencreport_query_result_class_methods);
	ce.create_object = opencreport_query_result_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_query_result_object_handlers.offset = XtOffsetOf(php_opencreport_query_result_object, zo);
#endif
	opencreport_query_result_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_query_result_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_query_result_ce = zend_register_internal_class(&ce);
	opencreport_query_result_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_query_result_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_query_result_ce->serialize = zend_class_serialize_deny;
	opencreport_query_result_ce->unserialize = zend_class_unserialize_deny;
#endif
}
