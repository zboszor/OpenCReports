/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

/* Handlers */
static zend_object_handlers opencreport_image_object_handlers;
static zend_object_handlers opencreport_text_object_handlers;
static zend_object_handlers opencreport_barcode_object_handlers;
static zend_object_handlers opencreport_output_element_object_handlers;
static zend_object_handlers opencreport_line_element_object_handlers;

/* Class entries */
zend_class_entry *opencreport_image_ce;
zend_class_entry *opencreport_text_ce;
zend_class_entry *opencreport_barcode_ce;
zend_class_entry *opencreport_output_element_ce;
zend_class_entry *opencreport_line_element_ce;

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_image_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_image_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_image_object *intern = zend_object_alloc(sizeof(php_opencreport_image_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_image_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_image_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_text_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_text_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_text_object *intern = zend_object_alloc(sizeof(php_opencreport_text_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_text_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_text_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

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

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_output_element_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_output_element_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_output_element_object *intern = zend_object_alloc(sizeof(php_opencreport_output_element_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_output_element_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_output_element_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_line_element_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_line_element_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_line_element_object *intern = zend_object_alloc(sizeof(php_opencreport_line_element_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_line_element_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_line_element_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

PHP_METHOD(opencreport_image, set_value) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
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

	ocrpt_expr *e = ocrpt_image_set_value(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, get_value) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_image_get_value(imo->image);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, set_suppress) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
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

	ocrpt_expr *e = ocrpt_image_set_suppress(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, get_suppress) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_image_get_suppress(imo->image);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, set_type) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
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

	ocrpt_expr *e = ocrpt_image_set_type(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, get_type) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_image_get_type(imo->image);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, set_width) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
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

	ocrpt_expr *e = ocrpt_image_set_width(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, get_width) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_image_get_width(imo->image);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, set_height) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
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

	ocrpt_expr *e = ocrpt_image_set_height(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, get_height) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_image_get_height(imo->image);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, set_alignment) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
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

	ocrpt_expr *e = ocrpt_image_set_alignment(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, get_alignment) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_image_get_alignment(imo->image);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, set_bgcolor) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
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

	ocrpt_expr *e = ocrpt_image_set_bgcolor(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, get_bgcolor) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_image_get_bgcolor(imo->image);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, set_text_width) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);
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

	ocrpt_expr *e = ocrpt_image_set_text_width(imo->image, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_image, get_text_width) {
	zval *object = getThis();
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_image_get_text_width(imo->image);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_set_value, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_get_value, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_set_type, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_get_type, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_set_width, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_get_width, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_set_height, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_get_height, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_set_alignment, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_get_alignment, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_set_bgcolor, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_get_bgcolor, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_set_text_width, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_image_get_text_width, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_image_set_value NULL
#define arginfo_opencreport_image_get_value NULL
#define arginfo_opencreport_image_set_suppress NULL
#define arginfo_opencreport_image_get_suppress NULL
#define arginfo_opencreport_image_set_type NULL
#define arginfo_opencreport_image_get_type NULL
#define arginfo_opencreport_image_set_width NULL
#define arginfo_opencreport_image_get_width NULL
#define arginfo_opencreport_image_set_height NULL
#define arginfo_opencreport_image_get_height NULL
#define arginfo_opencreport_image_set_alignment NULL
#define arginfo_opencreport_image_get_alignment NULL
#define arginfo_opencreport_image_set_bgcolor NULL
#define arginfo_opencreport_image_get_bgcolor NULL
#define arginfo_opencreport_image_set_text_width NULL
#define arginfo_opencreport_image_get_text_width NULL

#endif

static const zend_function_entry opencreport_image_class_methods[] = {
	PHP_ME(opencreport_image, set_value, arginfo_opencreport_image_set_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, get_value, arginfo_opencreport_image_get_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_suppress, arginfo_opencreport_image_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, get_suppress, arginfo_opencreport_image_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_type, arginfo_opencreport_image_set_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, get_type, arginfo_opencreport_image_get_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_width, arginfo_opencreport_image_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, get_width, arginfo_opencreport_image_get_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_height, arginfo_opencreport_image_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, get_height, arginfo_opencreport_image_get_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_alignment, arginfo_opencreport_image_set_alignment, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, get_alignment, arginfo_opencreport_image_get_alignment, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_bgcolor, arginfo_opencreport_image_set_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, get_bgcolor, arginfo_opencreport_image_get_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, set_text_width, arginfo_opencreport_image_set_text_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_image, get_text_width, arginfo_opencreport_image_get_text_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_text, set_value_string) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_value_string(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_value_expr) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_value_expr(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_value) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_value(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_value_delayed) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_value_delayed(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_value_delayed) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_value_delayed(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_format) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_format(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_format) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_format(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_translate) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_translate(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_translate) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_translate(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_width) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_width(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_width) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_width(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_alignment) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_alignment(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_alignment) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_alignment(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_color) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_color(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_color) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_color(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_bgcolor) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_bgcolor(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_bgcolor) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_bgcolor(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_font_name) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_font_name(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_font_name) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_font_name(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_font_size) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_font_size(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_font_size) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_font_size(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_bold) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_bold(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_bold) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_bold(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_italic) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_italic(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_italic) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_italic(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_link) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_link(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_link) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_link(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_memo) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_memo(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_memo) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_memo(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_memo_wrap_chars) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_memo_wrap_chars(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_memo_wrap_chars) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_memo_wrap_chars(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, set_memo_max_lines) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);
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

	ocrpt_expr *e = ocrpt_text_set_memo_max_lines(to->text, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_text, get_memo_max_lines) {
	zval *object = getThis();
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_text_get_memo_max_lines(to->text);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_value_string, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_value_expr, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_value, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_value_delayed, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_value_delayed, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_format, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_format, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_translate, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_translate, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_width, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_width, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_alignment, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_alignment, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_color, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_color, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_bgcolor, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_bgcolor, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_font_name, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_font_name, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_font_size, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_font_size, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_bold, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_bold, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_italic, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_italic, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_link, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_link, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_memo, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_memo, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_memo_wrap_chars, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_memo_wrap_chars, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_set_memo_max_lines, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_text_get_memo_max_lines, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_text_set_value_string NULL
#define arginfo_opencreport_text_set_value_expr NULL
#define arginfo_opencreport_text_get_value NULL
#define arginfo_opencreport_text_set_value_delayed NULL
#define arginfo_opencreport_text_get_value_delayed NULL
#define arginfo_opencreport_text_set_format NULL
#define arginfo_opencreport_text_get_format NULL
#define arginfo_opencreport_text_set_translate NULL
#define arginfo_opencreport_text_get_translate NULL
#define arginfo_opencreport_text_set_width NULL
#define arginfo_opencreport_text_get_width NULL
#define arginfo_opencreport_text_set_alignment NULL
#define arginfo_opencreport_text_get_alignment NULL
#define arginfo_opencreport_text_set_color NULL
#define arginfo_opencreport_text_get_color NULL
#define arginfo_opencreport_text_set_bgcolor NULL
#define arginfo_opencreport_text_get_bgcolor NULL
#define arginfo_opencreport_text_set_font_name NULL
#define arginfo_opencreport_text_get_font_name NULL
#define arginfo_opencreport_text_set_font_size NULL
#define arginfo_opencreport_text_get_font_size NULL
#define arginfo_opencreport_text_set_bold NULL
#define arginfo_opencreport_text_get_bold NULL
#define arginfo_opencreport_text_set_italic NULL
#define arginfo_opencreport_text_get_italic NULL
#define arginfo_opencreport_text_set_link NULL
#define arginfo_opencreport_text_get_link NULL
#define arginfo_opencreport_text_set_memo NULL
#define arginfo_opencreport_text_get_memo NULL
#define arginfo_opencreport_text_set_memo_wrap_chars NULL
#define arginfo_opencreport_text_get_memo_wrap_chars NULL
#define arginfo_opencreport_text_set_memo_max_lines NULL
#define arginfo_opencreport_text_get_memo_max_lines NULL

#endif

static const zend_function_entry opencreport_text_class_methods[] = {
	PHP_ME(opencreport_text, set_value_string, arginfo_opencreport_text_set_value_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_value_expr, arginfo_opencreport_text_set_value_expr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_value, arginfo_opencreport_text_get_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_value_delayed, arginfo_opencreport_text_set_value_delayed, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_value_delayed, arginfo_opencreport_text_get_value_delayed, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_format, arginfo_opencreport_text_set_format, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_format, arginfo_opencreport_text_get_format, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_translate, arginfo_opencreport_text_set_translate, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_translate, arginfo_opencreport_text_get_translate, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_width, arginfo_opencreport_text_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_width, arginfo_opencreport_text_get_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_alignment, arginfo_opencreport_text_set_alignment, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_alignment, arginfo_opencreport_text_get_alignment, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_color, arginfo_opencreport_text_set_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_color, arginfo_opencreport_text_get_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_bgcolor, arginfo_opencreport_text_set_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_bgcolor, arginfo_opencreport_text_get_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_font_name, arginfo_opencreport_text_set_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_font_name, arginfo_opencreport_text_get_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_font_size, arginfo_opencreport_text_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_font_size, arginfo_opencreport_text_get_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_bold, arginfo_opencreport_text_set_bold, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_bold, arginfo_opencreport_text_get_bold, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_italic, arginfo_opencreport_text_set_italic, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_italic, arginfo_opencreport_text_get_italic, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_link, arginfo_opencreport_text_set_link, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_link, arginfo_opencreport_text_get_link, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_memo, arginfo_opencreport_text_set_memo, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_memo, arginfo_opencreport_text_get_memo, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_memo_wrap_chars, arginfo_opencreport_text_set_memo_wrap_chars, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_memo_wrap_chars, arginfo_opencreport_text_get_memo_wrap_chars, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, set_memo_max_lines, arginfo_opencreport_text_set_memo_max_lines, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_text, get_memo_max_lines, arginfo_opencreport_text_get_memo_max_lines, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

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

PHP_METHOD(opencreport_output_element, get_next) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_output *output = oeo->output;

	void *iter = oeo->iter;

	ocrpt_output_element *oe = ocrpt_output_iterate_elements(output, &iter);
	if (!oe)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_output_element_ce);
	oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(return_value);
	oeo->output = output;
	oeo->elem = oe;
	oeo->iter = iter;
}

PHP_METHOD(opencreport_output_element, is_line) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_output_element_is_line(oeo->elem));
}

PHP_METHOD(opencreport_output_element, is_hline) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_output_element_is_hline(oeo->elem));
}

PHP_METHOD(opencreport_output_element, is_image) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_output_element_is_image(oeo->elem));
}

PHP_METHOD(opencreport_output_element, is_barcode) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_output_element_is_barcode(oeo->elem));
}

PHP_METHOD(opencreport_output_element, get_line) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ocrpt_output_element_is_line(oeo->elem))
		RETURN_NULL();

	object_init_ex(return_value, opencreport_line_ce);
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(return_value);
	lo->line = (ocrpt_line *)oeo->elem;
}

PHP_METHOD(opencreport_output_element, get_hline) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ocrpt_output_element_is_hline(oeo->elem))
		RETURN_NULL();

	object_init_ex(return_value, opencreport_hline_ce);
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(return_value);
	hlo->hline = (ocrpt_hline *)oeo->elem;
}

PHP_METHOD(opencreport_output_element, get_image) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ocrpt_output_element_is_image(oeo->elem))
		RETURN_NULL();

	object_init_ex(return_value, opencreport_image_ce);
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(return_value);
	imo->image = (ocrpt_image *)oeo->elem;
}

PHP_METHOD(opencreport_output_element, get_barcode) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ocrpt_output_element_is_barcode(oeo->elem))
		RETURN_NULL();

	object_init_ex(return_value, opencreport_barcode_ce);
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(return_value);
	bco->bc = (ocrpt_barcode *)oeo->elem;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_element_get_next, 0, 0, OpenCReport\\OutputElement, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_output_element_is_line, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_output_element_is_hline, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_output_element_is_image, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_output_element_is_barcode, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_element_get_line, 0, 0, OpenCReport\\Line, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_element_get_hline, 0, 0, OpenCReport\\HorizontalLine, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_element_get_image, 0, 0, OpenCReport\\Image, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_element_get_barcode, 0, 0, OpenCReport\\Barcode, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_output_get_first_element NULL
#define arginfo_opencreport_output_element_is_line NULL
#define arginfo_opencreport_output_element_is_hline NULL
#define arginfo_opencreport_output_element_is_image NULL
#define arginfo_opencreport_output_element_is_barcode NULL
#define arginfo_opencreport_output_element_get_line NULL
#define arginfo_opencreport_output_element_get_hline NULL
#define arginfo_opencreport_output_element_get_image NULL
#define arginfo_opencreport_output_element_get_barcode NULL

#endif

static const zend_function_entry opencreport_output_element_class_methods[] = {
	PHP_ME(opencreport_output_element, get_next, arginfo_opencreport_output_element_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output_element, is_line, arginfo_opencreport_output_element_is_line, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output_element, is_hline, arginfo_opencreport_output_element_is_hline, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output_element, is_image, arginfo_opencreport_output_element_is_image, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output_element, is_barcode, arginfo_opencreport_output_element_is_barcode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output_element, get_line, arginfo_opencreport_output_element_get_line, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output_element, get_hline, arginfo_opencreport_output_element_get_hline, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output_element, get_image, arginfo_opencreport_output_element_get_image, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output_element, get_barcode, arginfo_opencreport_output_element_get_barcode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_line_element, get_next) {
	zval *object = getThis();
	php_opencreport_line_element_object *leo = Z_OPENCREPORT_LINE_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_line *line = leo->line;

	void *iter = leo->iter;

	ocrpt_line_element *le = ocrpt_line_iterate_elements(line, &iter);
	if (!le)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_line_element_ce);
	leo = Z_OPENCREPORT_LINE_ELEMENT_P(return_value);
	leo->line = line;
	leo->elem = le;
	leo->iter = iter;
}

PHP_METHOD(opencreport_line_element, is_text) {
	zval *object = getThis();
	php_opencreport_line_element_object *leo = Z_OPENCREPORT_LINE_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_line_element_is_text(leo->elem));
}

PHP_METHOD(opencreport_line_element, is_image) {
	zval *object = getThis();
	php_opencreport_line_element_object *leo = Z_OPENCREPORT_LINE_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_line_element_is_image(leo->elem));
}

PHP_METHOD(opencreport_line_element, is_barcode) {
	zval *object = getThis();
	php_opencreport_line_element_object *leo = Z_OPENCREPORT_LINE_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_line_element_is_barcode(leo->elem));
}

PHP_METHOD(opencreport_line_element, get_text) {
	zval *object = getThis();
	php_opencreport_line_element_object *leo = Z_OPENCREPORT_LINE_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ocrpt_line_element_is_text(leo->elem))
		RETURN_NULL();

	object_init_ex(return_value, opencreport_text_ce);
	php_opencreport_text_object *to = Z_OPENCREPORT_TEXT_P(return_value);
	to->text = (ocrpt_text *)leo->elem;
}

PHP_METHOD(opencreport_line_element, get_image) {
	zval *object = getThis();
	php_opencreport_line_element_object *leo = Z_OPENCREPORT_LINE_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ocrpt_line_element_is_image(leo->elem))
		RETURN_NULL();

	object_init_ex(return_value, opencreport_image_ce);
	php_opencreport_image_object *imo = Z_OPENCREPORT_IMAGE_P(return_value);
	imo->image = (ocrpt_image *)leo->elem;
}

PHP_METHOD(opencreport_line_element, get_barcode) {
	zval *object = getThis();
	php_opencreport_line_element_object *leo = Z_OPENCREPORT_LINE_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!ocrpt_line_element_is_barcode(leo->elem))
		RETURN_NULL();

	object_init_ex(return_value, opencreport_barcode_ce);
	php_opencreport_barcode_object *bco = Z_OPENCREPORT_BARCODE_P(return_value);
	bco->bc = (ocrpt_barcode *)leo->elem;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_element_get_next, 0, 0, OpenCReport\\LineElement, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_element_is_text, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_element_is_image, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_line_element_is_barcode, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_element_get_text, 0, 0, OpenCReport\\Line, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_element_get_image, 0, 0, OpenCReport\\Image, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_element_get_barcode, 0, 0, OpenCReport\\Barcode, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_line_element_get_next NULL
#define arginfo_opencreport_line_element_is_text NULL
#define arginfo_opencreport_line_element_is_image NULL
#define arginfo_opencreport_line_element_is_barcode NULL
#define arginfo_opencreport_line_element_get_text NULL
#define arginfo_opencreport_line_element_get_image NULL
#define arginfo_opencreport_line_element_get_barcode NULL

#endif

static const zend_function_entry opencreport_line_element_class_methods[] = {
	PHP_ME(opencreport_line_element, get_next, arginfo_opencreport_line_element_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line_element, is_text, arginfo_opencreport_line_element_is_text, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line_element, is_image, arginfo_opencreport_line_element_is_image, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line_element, is_barcode, arginfo_opencreport_line_element_is_barcode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line_element, get_text, arginfo_opencreport_line_element_get_text, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line_element, get_image, arginfo_opencreport_line_element_get_image, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line_element, get_barcode, arginfo_opencreport_line_element_get_barcode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

/* {{{ PHP_MINIT_FUNCTION */
static PHP_MINIT_FUNCTION(opencreport)
{
	if (!opencreport_init())
		return FAILURE;

	register_opencreport_ce();
	register_opencreport_ds_ce();
	register_opencreport_query_ce();
	register_opencreport_query_result_ce();
	register_opencreport_expr_ce();
	register_opencreport_result_ce();
	register_opencreport_part_ce();
	register_opencreport_row_ce();
	register_opencreport_col_ce();
	register_opencreport_report_ce();
	register_opencreport_variable_ce();
	register_opencreport_break_ce();
	register_opencreport_output_ce();
	register_opencreport_line_ce();
	register_opencreport_hline_ce();

	zend_class_entry ce;

	memcpy(&opencreport_image_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Image", opencreport_image_class_methods);
	ce.create_object = opencreport_image_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_image_object_handlers.offset = XtOffsetOf(php_opencreport_image_object, zo);
#endif
	opencreport_image_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_image_object_handlers.compare = zend_objects_not_comparable;
#endif
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
#if PHP_VERSION_ID >= 70000
	opencreport_text_object_handlers.offset = XtOffsetOf(php_opencreport_text_object, zo);
#endif
	opencreport_text_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_text_object_handlers.compare = zend_objects_not_comparable;
#endif
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

	memcpy(&opencreport_output_element_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "OutputElement", opencreport_output_element_class_methods);
	ce.create_object = opencreport_output_element_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_output_element_object_handlers.offset = XtOffsetOf(php_opencreport_output_element_object, zo);
#endif
	opencreport_output_element_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_output_element_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_output_element_ce = zend_register_internal_class(&ce);
	opencreport_output_element_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_output_element_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_output_element_ce->serialize = zend_class_serialize_deny;
	opencreport_output_element_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_line_element_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "LineElement", opencreport_line_element_class_methods);
	ce.create_object = opencreport_line_element_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_line_element_object_handlers.offset = XtOffsetOf(php_opencreport_line_element_object, zo);
#endif
	opencreport_line_element_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_line_element_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_line_element_ce = zend_register_internal_class(&ce);
	opencreport_line_element_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_line_element_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_line_element_ce->serialize = zend_class_serialize_deny;
	opencreport_line_element_ce->unserialize = zend_class_unserialize_deny;
#endif

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
