/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_query_object_handlers;

zend_class_entry *opencreport_query_ce;

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_query_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_query_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_query_object *intern = zend_object_alloc(sizeof(php_opencreport_query_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_query_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_query_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

PHP_METHOD(opencreport_query, get_result) {
	zval *object = getThis();
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
	zval *object = getThis();
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_navigate_start(qo->q);
}

PHP_METHOD(opencreport_query, navigate_next) {
	zval *object = getThis();
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_query_navigate_next(qo->q));
}

PHP_METHOD(opencreport_query, navigate_use_prev_row) {
	zval *object = getThis();
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_navigate_use_prev_row(qo->q);
}

PHP_METHOD(opencreport_query, navigate_use_next_row) {
	zval *object = getThis();
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_navigate_use_next_row(qo->q);
}

PHP_METHOD(opencreport_query, add_follower) {
	zval *object = getThis();
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);
	zval *fobj;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(fobj, opencreport_query_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &fobj, opencreport_query_ce) == FAILURE)
		return;
#endif

	php_opencreport_query_object *fo = Z_OPENCREPORT_QUERY_P(fobj);

	RETURN_BOOL(ocrpt_query_add_follower(qo->q, fo->q));
}

PHP_METHOD(opencreport_query, add_follower_n_to_1) {
	zval *object = getThis();
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);
	zval *fobj;
	zval *mobj;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(fobj, opencreport_query_ce);
		Z_PARAM_OBJECT_OF_CLASS(mobj, opencreport_expr_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "OO", &fobj, opencreport_query_ce, &mobj, opencreport_expr_ce) == FAILURE)
		return;
#endif

	php_opencreport_query_object *fo = Z_OPENCREPORT_QUERY_P(fobj);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(mobj);

	bool retval = ocrpt_query_add_follower_n_to_1(qo->q, fo->q, eo->e);

	/* The expression's ownership was taken over or it was freed. */
	eo->e = NULL;

	RETURN_BOOL(retval);
}

PHP_METHOD(opencreport_query, free) {
	zval *object = getThis();
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(object);

	if (!qo->q) {
		zend_throw_error(NULL, "OpenCReport\\Query object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_free(qo->q);
	qo->q = NULL;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_query_get_result, 0, 0, OpenCReport\\QueryResult, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_navigate_start, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_navigate_next, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_navigate_use_prev_row, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_navigate_use_next_row, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_add_follower, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, follower, OpenCReport\\Query, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_add_follower_n_to_1, 0, 2, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, follower, OpenCReport\\Query, 0)
ZEND_ARG_OBJ_INFO(0, match, OpenCReport\\Expr, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_query_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_query_get_result NULL
#define arginfo_opencreport_query_navigate_start NULL
#define arginfo_opencreport_query_navigate_next NULL
#define arginfo_opencreport_query_navigate_use_prev_row NULL
#define arginfo_opencreport_query_navigate_use_next_row NULL
#define arginfo_opencreport_query_add_follower NULL
#define arginfo_opencreport_query_add_follower_n_to_1 NULL
#define arginfo_opencreport_query_free NULL

#endif

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

void register_opencreport_query_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_query_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Query", opencreport_query_class_methods);
	ce.create_object = opencreport_query_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_query_object_handlers.offset = XtOffsetOf(php_opencreport_query_object, zo);
#endif
	opencreport_query_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_query_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_query_ce = zend_register_internal_class(&ce);
	opencreport_query_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_query_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_query_ce->serialize = zend_class_serialize_deny;
	opencreport_query_ce->unserialize = zend_class_unserialize_deny;
#endif
}
