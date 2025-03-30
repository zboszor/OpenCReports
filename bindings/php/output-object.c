/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_output_object_handlers;

zend_class_entry *opencreport_output_ce;

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_output_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_output_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_output_object *intern = zend_object_alloc(sizeof(php_opencreport_output_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_output_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_output_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

PHP_METHOD(opencreport_output, set_suppress) {
	zval *object = getThis();
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
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

	ocrpt_expr *e = ocrpt_output_set_suppress(oo->output, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_output, get_suppress) {
	zval *object = getThis();
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_output_get_suppress(oo->output);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_output, add_line) {
	zval *object = getThis();
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	ocrpt_line *l;
	php_opencreport_line_object *lo;

	ZEND_PARSE_PARAMETERS_NONE();

	l = ocrpt_output_add_line(oo->output);
	if (!l)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_line_ce);
	lo = Z_OPENCREPORT_LINE_P(return_value);
	lo->line = l;
}

PHP_METHOD(opencreport_output, add_hline) {
	zval *object = getThis();
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	ocrpt_hline *hl;
	php_opencreport_hline_object *hlo;

	ZEND_PARSE_PARAMETERS_NONE();

	hl = ocrpt_output_add_hline(oo->output);
	if (!hl)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_hline_ce);
	hlo = Z_OPENCREPORT_HLINE_P(return_value);
	hlo->hline = hl;
}

PHP_METHOD(opencreport_output, add_image) {
	zval *object = getThis();
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	ocrpt_image *im;
	php_opencreport_image_object *imo;

	ZEND_PARSE_PARAMETERS_NONE();

	im = ocrpt_output_add_image(oo->output);
	if (!im)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_image_ce);
	imo = Z_OPENCREPORT_IMAGE_P(return_value);
	imo->image = im;
}

PHP_METHOD(opencreport_output, add_image_end) {
	zval *object = getThis();
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_output_add_image_end(oo->output);
}

PHP_METHOD(opencreport_output, add_barcode) {
	zval *object = getThis();
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);
	ocrpt_barcode *bc;
	php_opencreport_barcode_object *bco;

	ZEND_PARSE_PARAMETERS_NONE();

	bc = ocrpt_output_add_barcode(oo->output);
	if (!bc)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_barcode_ce);
	bco = Z_OPENCREPORT_BARCODE_P(return_value);
	bco->bc = bc;
}

PHP_METHOD(opencreport_output, get_first_element) {
	zval *object = getThis();
	php_opencreport_output_object *oo = Z_OPENCREPORT_OUTPUT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	void *iter = NULL;
	ocrpt_output_element *oe = ocrpt_output_iterate_elements(oo->output, &iter);
	if (!oe)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_output_element_ce);
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(return_value);
	oeo->output = oo->output;
	oeo->elem = oe;
	oeo->iter = iter;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_add_line, 0, 0, OpenCReport\\Line, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_add_hline, 0, 0, OpenCReport\\HorizontalLine, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_add_image, 0, 0, OpenCReport\\Image, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_output_add_image_end, 0, 0, IS_VOID, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_add_barcode, 0, 0, OpenCReport\\Image, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_output_get_first_element, 0, 0, OpenCReport\\OutputElement, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_output_set_suppress NULL
#define arginfo_opencreport_output_get_suppress NULL
#define arginfo_opencreport_output_add_line NULL
#define arginfo_opencreport_output_add_hline NULL
#define arginfo_opencreport_output_add_image NULL
#define arginfo_opencreport_output_add_image_end NULL
#define arginfo_opencreport_output_add_barcode NULL
#define arginfo_opencreport_output_get_first_element NULL

#endif

static const zend_function_entry opencreport_output_class_methods[] = {
	PHP_ME(opencreport_output, set_suppress, arginfo_opencreport_output_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, get_suppress, arginfo_opencreport_output_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_line, arginfo_opencreport_output_add_line, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_hline, arginfo_opencreport_output_add_hline, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_image, arginfo_opencreport_output_add_image, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_image_end, arginfo_opencreport_output_add_image_end, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, add_barcode, arginfo_opencreport_output_add_barcode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_output, get_first_element, arginfo_opencreport_output_get_first_element, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

void register_opencreport_output_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_output_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Output", opencreport_output_class_methods);
	ce.create_object = opencreport_output_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_output_object_handlers.offset = XtOffsetOf(php_opencreport_output_object, zo);
#endif
	opencreport_output_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_output_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_output_ce = zend_register_internal_class(&ce);
	opencreport_output_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_output_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_output_ce->serialize = zend_class_serialize_deny;
	opencreport_output_ce->unserialize = zend_class_unserialize_deny;
#endif
}
