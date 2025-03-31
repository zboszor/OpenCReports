/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_line_object_handlers;

zend_class_entry *opencreport_line_ce;

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_line_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_line_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_line_object *intern = zend_object_alloc(sizeof(php_opencreport_line_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_line_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_line_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

PHP_METHOD(opencreport_line, set_font_name) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
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

	ocrpt_expr *e = ocrpt_line_set_font_name(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, get_font_name) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_line_get_font_name(lo->line);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, set_font_size) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
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

	ocrpt_expr *e = ocrpt_line_set_font_size(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, get_font_size) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_line_get_font_size(lo->line);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, set_bold) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
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

	ocrpt_expr *e = ocrpt_line_set_bold(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, get_bold) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_line_get_bold(lo->line);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, set_italic) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
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

	ocrpt_expr *e = ocrpt_line_set_italic(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, get_italic) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_line_get_italic(lo->line);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, set_suppress) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
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

	ocrpt_expr *e = ocrpt_line_set_suppress(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, get_suppress) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_line_get_suppress(lo->line);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, set_color) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
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

	ocrpt_expr *e = ocrpt_line_set_color(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, get_color) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_line_get_color(lo->line);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, set_bgcolor) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);
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

	ocrpt_expr *e = ocrpt_line_set_bgcolor(lo->line, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, get_bgcolor) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_line_get_bgcolor(lo->line);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_line, add_text) {
	zval *object = getThis();
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
	zval *object = getThis();
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
	zval *object = getThis();
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

PHP_METHOD(opencreport_line, get_first_element) {
	zval *object = getThis();
	php_opencreport_line_object *lo = Z_OPENCREPORT_LINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_line_element *le = ocrpt_line_element_get_next(lo->line, &iter);
	if (!le)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_line_element_ce);
	php_opencreport_line_element_object *leo = Z_OPENCREPORT_LINE_ELEMENT_P(return_value);
	leo->line = lo->line;
	leo->elem = le;
	leo->iter = iter;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_set_font_name, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_get_font_name, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_set_font_size, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_get_font_size, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_set_bold, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_get_bold, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_set_italic, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_get_italic, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_set_color, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_get_color, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_set_bgcolor, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_get_bgcolor, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_add_text, 0, 0, OpenCReport\\Text, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_add_image, 0, 0, OpenCReport\\Image, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_add_barcode, 0, 0, OpenCReport\\Barcode, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_line_get_first_element, 0, 0, OpenCReport\\LineElement, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_line_set_font_name NULL
#define arginfo_opencreport_line_get_font_name NULL
#define arginfo_opencreport_line_set_font_size NULL
#define arginfo_opencreport_line_get_font_size NULL
#define arginfo_opencreport_line_set_bold NULL
#define arginfo_opencreport_line_get_bold NULL
#define arginfo_opencreport_line_set_italic NULL
#define arginfo_opencreport_line_get_italic NULL
#define arginfo_opencreport_line_set_suppress NULL
#define arginfo_opencreport_line_get_suppress NULL
#define arginfo_opencreport_line_set_color NULL
#define arginfo_opencreport_line_get_color NULL
#define arginfo_opencreport_line_set_bgcolor NULL
#define arginfo_opencreport_line_get_bgcolor NULL
#define arginfo_opencreport_line_add_text NULL
#define arginfo_opencreport_line_add_image NULL
#define arginfo_opencreport_line_add_barcode NULL
#define arginfo_opencreport_line_get_first_element NULL

#endif

static const zend_function_entry opencreport_line_class_methods[] = {
	PHP_ME(opencreport_line, set_font_name, arginfo_opencreport_line_set_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, get_font_name, arginfo_opencreport_line_get_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_font_size, arginfo_opencreport_line_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, get_font_size, arginfo_opencreport_line_get_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_bold, arginfo_opencreport_line_set_bold, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, get_bold, arginfo_opencreport_line_get_bold, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_italic, arginfo_opencreport_line_set_italic, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, get_italic, arginfo_opencreport_line_get_italic, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_suppress, arginfo_opencreport_line_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, get_suppress, arginfo_opencreport_line_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_color, arginfo_opencreport_line_set_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, get_color, arginfo_opencreport_line_get_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, set_bgcolor, arginfo_opencreport_line_set_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, get_bgcolor, arginfo_opencreport_line_get_bgcolor, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, add_text, arginfo_opencreport_line_add_text, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, add_image, arginfo_opencreport_line_add_image, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, add_barcode, arginfo_opencreport_line_add_barcode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_line, get_first_element, arginfo_opencreport_line_get_first_element, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

void register_opencreport_line_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_line_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Line", opencreport_line_class_methods);
	ce.create_object = opencreport_line_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_line_object_handlers.offset = XtOffsetOf(php_opencreport_line_object, zo);
#endif
	opencreport_line_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_line_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_line_ce = zend_register_internal_class(&ce);
	opencreport_line_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_line_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_line_ce->serialize = zend_class_serialize_deny;
	opencreport_line_ce->unserialize = zend_class_unserialize_deny;
#endif
}
