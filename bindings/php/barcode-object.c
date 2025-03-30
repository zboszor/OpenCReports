/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_barcode_object_handlers;

zend_class_entry *opencreport_barcode_ce;

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_barcode_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_barcode_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_barcode_object *intern = zend_object_alloc(sizeof(php_opencreport_barcode_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_barcode_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_barcode_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

PHP_METHOD(opencreport_barcode, set_value) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string = NULL;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	ocrpt_expr *e = ocrpt_barcode_set_value(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, get_value) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_barcode_get_value(bco->bc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, set_value_delayed) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string = NULL;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	ocrpt_expr *e = ocrpt_barcode_set_value_delayed(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, get_value_delayed) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_barcode_get_value_delayed(bco->bc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, set_suppress) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string = NULL;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	ocrpt_expr *e = ocrpt_barcode_set_suppress(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, get_suppress) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_barcode_get_suppress(bco->bc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, set_type) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string = NULL;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	ocrpt_expr *e = ocrpt_barcode_set_type(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, get_type) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_barcode_get_type(bco->bc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, set_width) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string = NULL;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	ocrpt_expr *e = ocrpt_barcode_set_width(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, get_width) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_barcode_get_width(bco->bc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, set_height) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string = NULL;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	ocrpt_expr *e = ocrpt_barcode_set_height(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, get_height) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_barcode_get_height(bco->bc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, set_color) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string = NULL;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	ocrpt_expr *e = ocrpt_barcode_set_color(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, get_color) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_barcode_get_color(bco->bc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, set_bgcolor) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR_EX(expr_string, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string = NULL;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	ocrpt_expr *e = ocrpt_barcode_set_bgcolor(bco->bc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_barcode, get_bgcolor) {
	zval *object = getThis();
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_barcode_get_bgcolor(bco->bc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_set_value, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_get_value, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_set_value_delayed, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_get_value_delayed, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_set_type, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_get_type, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_set_width, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_get_width, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_set_height, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_get_height, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_set_color, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_get_color, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_set_bgcolor, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_barcode_get_bgcolor, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_barcode_set_value NULL
#define arginfo_opencreport_barcode_get_value NULL
#define arginfo_opencreport_barcode_set_value_delayed NULL
#define arginfo_opencreport_barcode_get_value_delayed NULL
#define arginfo_opencreport_barcode_set_suppress NULL
#define arginfo_opencreport_barcode_get_suppress NULL
#define arginfo_opencreport_barcode_set_type NULL
#define arginfo_opencreport_barcode_get_type NULL
#define arginfo_opencreport_barcode_set_width NULL
#define arginfo_opencreport_barcode_get_width NULL
#define arginfo_opencreport_barcode_set_height NULL
#define arginfo_opencreport_barcode_get_height NULL
#define arginfo_opencreport_barcode_set_color NULL
#define arginfo_opencreport_barcode_get_color NULL
#define arginfo_opencreport_barcode_set_bgcolor NULL
#define arginfo_opencreport_barcode_get_bgcolor NULL

#endif

static const zend_function_entry opencreport_barcode_class_methods[] = {
	PHP_ME(opencreport_barcode, set_value, arginfo_opencreport_barcode_set_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, get_value, arginfo_opencreport_barcode_get_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_value_delayed, arginfo_opencreport_barcode_set_value_delayed, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, get_value_delayed, arginfo_opencreport_barcode_get_value_delayed, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_suppress, arginfo_opencreport_barcode_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, get_suppress, arginfo_opencreport_barcode_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_type, arginfo_opencreport_barcode_set_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, get_type, arginfo_opencreport_barcode_get_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_width, arginfo_opencreport_barcode_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, get_width, arginfo_opencreport_barcode_get_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_height, arginfo_opencreport_barcode_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, get_height, arginfo_opencreport_barcode_get_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_color, arginfo_opencreport_barcode_set_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, get_color, arginfo_opencreport_barcode_get_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, set_bgcolor, arginfo_opencreport_barcode_set_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_barcode, get_bgcolor, arginfo_opencreport_barcode_get_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

void register_opencreport_barcode_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_barcode_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Barcode", opencreport_barcode_class_methods);
	ce.create_object = opencreport_barcode_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_barcode_object_handlers.offset = XtOffsetOf(php_opencreport_barcode_object, zo);
#endif
	opencreport_barcode_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_barcode_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_barcode_ce = zend_register_internal_class(&ce);
	opencreport_barcode_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_barcode_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_barcode_ce->serialize = zend_class_serialize_deny;
	opencreport_barcode_ce->unserialize = zend_class_unserialize_deny;
#endif
}
