/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_output_element_object_handlers;

zend_class_entry *opencreport_output_element_ce;

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

PHP_METHOD(opencreport_output_element, get_next) {
	zval *object = getThis();
	php_opencreport_output_element_object *oeo = Z_OPENCREPORT_OUTPUT_ELEMENT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_output *output = oeo->output;

	ocrpt_list *iter = oeo->iter;

	ocrpt_output_element *oe = ocrpt_output_element_get_next(output, &iter);
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

void register_opencreport_output_element_ce(void) {
	zend_class_entry ce;

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
}
