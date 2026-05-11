/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_genline_object_handlers;

zend_class_entry *opencreport_genline_ce;

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_genline_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_genline_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_genline_object *intern = zend_object_alloc(sizeof(php_opencreport_genline_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_genline_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_genline_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

/* Helper macro to define a set_* method that takes one nullable string. */
#define GENLINE_SET_METHOD(method_name, c_func) \
PHP_METHOD(opencreport_genline, method_name) { \
	zval *object = getThis(); \
	php_opencreport_genline_object *glo = Z_OPENCREPORT_GENLINE_P(object); \
	PHP_GENLINE_SET_BODY(c_func, glo->gl) \
}

#if PHP_VERSION_ID >= 70000
#define PHP_GENLINE_SET_BODY(c_func, glptr) \
	zend_string *expr_string = NULL; \
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1) \
		Z_PARAM_STR_EX(expr_string, 1, 0); \
	ZEND_PARSE_PARAMETERS_END(); \
	ocrpt_expr *e = c_func(glptr, expr_string ? ZSTR_VAL(expr_string) : NULL); \
	if (!e) \
		RETURN_NULL(); \
	object_init_ex(return_value, opencreport_expr_ce); \
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value); \
	eo->e = e;
#else
#define PHP_GENLINE_SET_BODY(c_func, glptr) \
	char *expr_string = NULL; \
	int expr_string_len; \
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!", &expr_string, &expr_string_len) == FAILURE) \
		return; \
	ocrpt_expr *e = c_func(glptr, expr_string); \
	if (!e) \
		RETURN_NULL(); \
	object_init_ex(return_value, opencreport_expr_ce); \
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value); \
	eo->e = e;
#endif

PHP_METHOD(opencreport_genline, set_query) {
	zval *object = getThis();
	php_opencreport_genline_object *glo = Z_OPENCREPORT_GENLINE_P(object);
	zval *query;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(query, opencreport_query_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &query, opencreport_query_ce) == FAILURE)
		return;
#endif

	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(query);

	ocrpt_genline_set_query(glo->gl, qo->q);
}

GENLINE_SET_METHOD(set_element_type, ocrpt_genline_set_element_type)
GENLINE_SET_METHOD(set_line_font_name, ocrpt_genline_set_line_font_name)
GENLINE_SET_METHOD(set_line_font_size, ocrpt_genline_set_line_font_size)
GENLINE_SET_METHOD(set_line_color, ocrpt_genline_set_line_color)
GENLINE_SET_METHOD(set_line_bgcolor, ocrpt_genline_set_line_bgcolor)
GENLINE_SET_METHOD(set_line_bold, ocrpt_genline_set_line_bold)
GENLINE_SET_METHOD(set_line_italic, ocrpt_genline_set_line_italic)
GENLINE_SET_METHOD(set_line_suppress, ocrpt_genline_set_line_suppress)
GENLINE_SET_METHOD(set_value, ocrpt_genline_set_value)
GENLINE_SET_METHOD(set_value_delayed, ocrpt_genline_set_value_delayed)
GENLINE_SET_METHOD(set_suppress, ocrpt_genline_set_suppress)
GENLINE_SET_METHOD(set_width, ocrpt_genline_set_width)
GENLINE_SET_METHOD(set_height, ocrpt_genline_set_height)
GENLINE_SET_METHOD(set_alignment, ocrpt_genline_set_alignment)
GENLINE_SET_METHOD(set_color, ocrpt_genline_set_color)
GENLINE_SET_METHOD(set_bgcolor, ocrpt_genline_set_bgcolor)
GENLINE_SET_METHOD(set_font_name, ocrpt_genline_set_font_name)
GENLINE_SET_METHOD(set_font_size, ocrpt_genline_set_font_size)
GENLINE_SET_METHOD(set_bold, ocrpt_genline_set_bold)
GENLINE_SET_METHOD(set_italic, ocrpt_genline_set_italic)
GENLINE_SET_METHOD(set_format, ocrpt_genline_set_format)
GENLINE_SET_METHOD(set_link, ocrpt_genline_set_link)
GENLINE_SET_METHOD(set_translate, ocrpt_genline_set_translate)
GENLINE_SET_METHOD(set_memo, ocrpt_genline_set_memo)
GENLINE_SET_METHOD(set_hyphenate, ocrpt_genline_set_hyphenate)
GENLINE_SET_METHOD(set_wrap_chars, ocrpt_genline_set_wrap_chars)
GENLINE_SET_METHOD(set_max_lines, ocrpt_genline_set_max_lines)
GENLINE_SET_METHOD(set_image_type, ocrpt_genline_set_image_type)
GENLINE_SET_METHOD(set_text_width, ocrpt_genline_set_text_width)
GENLINE_SET_METHOD(set_barcode_type, ocrpt_genline_set_barcode_type)

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_genline_set_query, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, query, OpenCReport\\Query, 0)
ZEND_END_ARG_INFO()

#define GENLINE_ARGINFO_SET(method_name) \
OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_genline_ ## method_name, 0, 1, OpenCReport\\Expr, 1) \
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1) \
ZEND_END_ARG_INFO()

GENLINE_ARGINFO_SET(set_element_type)
GENLINE_ARGINFO_SET(set_line_font_name)
GENLINE_ARGINFO_SET(set_line_font_size)
GENLINE_ARGINFO_SET(set_line_color)
GENLINE_ARGINFO_SET(set_line_bgcolor)
GENLINE_ARGINFO_SET(set_line_bold)
GENLINE_ARGINFO_SET(set_line_italic)
GENLINE_ARGINFO_SET(set_line_suppress)
GENLINE_ARGINFO_SET(set_value)
GENLINE_ARGINFO_SET(set_value_delayed)
GENLINE_ARGINFO_SET(set_suppress)
GENLINE_ARGINFO_SET(set_width)
GENLINE_ARGINFO_SET(set_height)
GENLINE_ARGINFO_SET(set_alignment)
GENLINE_ARGINFO_SET(set_color)
GENLINE_ARGINFO_SET(set_bgcolor)
GENLINE_ARGINFO_SET(set_font_name)
GENLINE_ARGINFO_SET(set_font_size)
GENLINE_ARGINFO_SET(set_bold)
GENLINE_ARGINFO_SET(set_italic)
GENLINE_ARGINFO_SET(set_format)
GENLINE_ARGINFO_SET(set_link)
GENLINE_ARGINFO_SET(set_translate)
GENLINE_ARGINFO_SET(set_memo)
GENLINE_ARGINFO_SET(set_hyphenate)
GENLINE_ARGINFO_SET(set_wrap_chars)
GENLINE_ARGINFO_SET(set_max_lines)
GENLINE_ARGINFO_SET(set_image_type)
GENLINE_ARGINFO_SET(set_text_width)
GENLINE_ARGINFO_SET(set_barcode_type)

#else

#define arginfo_opencreport_genline_set_query NULL
#define arginfo_opencreport_genline_set_element_type NULL
#define arginfo_opencreport_genline_set_line_font_name NULL
#define arginfo_opencreport_genline_set_line_font_size NULL
#define arginfo_opencreport_genline_set_line_color NULL
#define arginfo_opencreport_genline_set_line_bgcolor NULL
#define arginfo_opencreport_genline_set_line_bold NULL
#define arginfo_opencreport_genline_set_line_italic NULL
#define arginfo_opencreport_genline_set_line_suppress NULL
#define arginfo_opencreport_genline_set_value NULL
#define arginfo_opencreport_genline_set_value_delayed NULL
#define arginfo_opencreport_genline_set_suppress NULL
#define arginfo_opencreport_genline_set_width NULL
#define arginfo_opencreport_genline_set_height NULL
#define arginfo_opencreport_genline_set_alignment NULL
#define arginfo_opencreport_genline_set_color NULL
#define arginfo_opencreport_genline_set_bgcolor NULL
#define arginfo_opencreport_genline_set_font_name NULL
#define arginfo_opencreport_genline_set_font_size NULL
#define arginfo_opencreport_genline_set_bold NULL
#define arginfo_opencreport_genline_set_italic NULL
#define arginfo_opencreport_genline_set_format NULL
#define arginfo_opencreport_genline_set_link NULL
#define arginfo_opencreport_genline_set_translate NULL
#define arginfo_opencreport_genline_set_memo NULL
#define arginfo_opencreport_genline_set_hyphenate NULL
#define arginfo_opencreport_genline_set_wrap_chars NULL
#define arginfo_opencreport_genline_set_max_lines NULL
#define arginfo_opencreport_genline_set_image_type NULL
#define arginfo_opencreport_genline_set_text_width NULL
#define arginfo_opencreport_genline_set_barcode_type NULL

#endif

#define GENLINE_ME(method_name) \
	PHP_ME(opencreport_genline, method_name, arginfo_opencreport_genline_ ## method_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)

static const zend_function_entry opencreport_genline_class_methods[] = {
	GENLINE_ME(set_query)
	GENLINE_ME(set_element_type)
	GENLINE_ME(set_line_font_name)
	GENLINE_ME(set_line_font_size)
	GENLINE_ME(set_line_color)
	GENLINE_ME(set_line_bgcolor)
	GENLINE_ME(set_line_bold)
	GENLINE_ME(set_line_italic)
	GENLINE_ME(set_line_suppress)
	GENLINE_ME(set_value)
	GENLINE_ME(set_value_delayed)
	GENLINE_ME(set_suppress)
	GENLINE_ME(set_width)
	GENLINE_ME(set_height)
	GENLINE_ME(set_alignment)
	GENLINE_ME(set_color)
	GENLINE_ME(set_bgcolor)
	GENLINE_ME(set_font_name)
	GENLINE_ME(set_font_size)
	GENLINE_ME(set_bold)
	GENLINE_ME(set_italic)
	GENLINE_ME(set_format)
	GENLINE_ME(set_link)
	GENLINE_ME(set_translate)
	GENLINE_ME(set_memo)
	GENLINE_ME(set_hyphenate)
	GENLINE_ME(set_wrap_chars)
	GENLINE_ME(set_max_lines)
	GENLINE_ME(set_image_type)
	GENLINE_ME(set_text_width)
	GENLINE_ME(set_barcode_type)
	PHP_FE_END
};

void register_opencreport_genline_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_genline_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "GenLine", opencreport_genline_class_methods);
	ce.create_object = opencreport_genline_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_genline_object_handlers.offset = XtOffsetOf(php_opencreport_genline_object, zo);
#endif
	opencreport_genline_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_genline_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_genline_ce = zend_register_internal_class(&ce);
	opencreport_genline_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_genline_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_genline_ce->serialize = zend_class_serialize_deny;
	opencreport_genline_ce->unserialize = zend_class_unserialize_deny;
#endif
}
