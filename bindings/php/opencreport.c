/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

/* Handlers */
static zend_object_handlers opencreport_query_object_handlers;
static zend_object_handlers opencreport_query_result_object_handlers;
static zend_object_handlers opencreport_expr_object_handlers;
static zend_object_handlers opencreport_result_object_handlers;
static zend_object_handlers opencreport_part_object_handlers;
static zend_object_handlers opencreport_row_object_handlers;
static zend_object_handlers opencreport_col_object_handlers;
static zend_object_handlers opencreport_report_object_handlers;
static zend_object_handlers opencreport_variable_object_handlers;
static zend_object_handlers opencreport_break_object_handlers;
static zend_object_handlers opencreport_output_object_handlers;
static zend_object_handlers opencreport_line_object_handlers;
static zend_object_handlers opencreport_hline_object_handlers;
static zend_object_handlers opencreport_image_object_handlers;
static zend_object_handlers opencreport_text_object_handlers;
static zend_object_handlers opencreport_barcode_object_handlers;
static zend_object_handlers opencreport_output_element_object_handlers;
static zend_object_handlers opencreport_line_element_object_handlers;

/* Class entries */
zend_class_entry *opencreport_query_ce;
zend_class_entry *opencreport_query_result_ce;
zend_class_entry *opencreport_expr_ce;
zend_class_entry *opencreport_result_ce;
zend_class_entry *opencreport_part_ce;
zend_class_entry *opencreport_row_ce;
zend_class_entry *opencreport_col_ce;
zend_class_entry *opencreport_report_ce;
zend_class_entry *opencreport_variable_ce;
zend_class_entry *opencreport_break_ce;
zend_class_entry *opencreport_output_ce;
zend_class_entry *opencreport_line_ce;
zend_class_entry *opencreport_hline_ce;
zend_class_entry *opencreport_image_ce;
zend_class_entry *opencreport_text_ce;
zend_class_entry *opencreport_barcode_ce;
zend_class_entry *opencreport_output_element_ce;
zend_class_entry *opencreport_line_element_ce;

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

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_expr_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_expr_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_expr_object *intern = zend_object_alloc(sizeof(php_opencreport_expr_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_expr_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_expr_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

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

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_part_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_part_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_part_object *intern = zend_object_alloc(sizeof(php_opencreport_part_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_part_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_part_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_row_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_row_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_row_object *intern = zend_object_alloc(sizeof(php_opencreport_row_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_row_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_row_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_col_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_col_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_col_object *intern = zend_object_alloc(sizeof(php_opencreport_col_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_col_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_col_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

static void opencreport_report_object_free(zend_object *object);

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_report_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_report_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_report_object *intern = zend_object_alloc(sizeof(php_opencreport_report_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) opencreport_report_object_free, NULL TSRMLS_CC);
	retval.handlers = &opencreport_report_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_report_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

static void opencreport_report_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_report_object *pro = php_opencreport_report_from_obj(object);

	ocrpt_strfree(pro->expr_error);

	zend_object_std_dtor(&pro->zo);
#if PHP_VERSION_ID < 70000
	efree(pro);
#endif
}
/* }}} */

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_variable_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_variable_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_variable_object *intern = zend_object_alloc(sizeof(php_opencreport_variable_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_variable_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_variable_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_break_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_break_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_break_object *intern = zend_object_alloc(sizeof(php_opencreport_break_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_break_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_break_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

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

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_hline_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_hline_object_new(zend_class_entry *class_type) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_hline_object *intern = zend_object_alloc(sizeof(php_opencreport_hline_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_hline_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_hline_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

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

PHP_METHOD(opencreport_expr, free) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_free(eo->e);
	eo->e = NULL;
}

PHP_METHOD(opencreport_expr, print) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_print(eo->e);
}

PHP_METHOD(opencreport_expr, nodes) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_nodes(eo->e));
}

PHP_METHOD(opencreport_expr, optimize) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_optimize(eo->e);
}

PHP_METHOD(opencreport_expr, resolve) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_resolve(eo->e);
}

PHP_METHOD(opencreport_expr, eval) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	php_opencreport_result_object *ro;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result *r = ocrpt_expr_eval(eo->e);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->r = r;
	ro->freed_by_lib = true;
}

PHP_METHOD(opencreport_expr, get_result) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result *r = ocrpt_expr_get_result(eo->e);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->o = NULL;
	ro->r = r;
	ro->freed_by_lib = true;
}

PHP_METHOD(opencreport_expr, set_string) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *value;
	int value_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &value, &value_len) == FAILURE)
		return;
#endif

	ocrpt_expr_set_string(eo->e, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, set_long) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_long value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	long value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &value) == FAILURE)
		return;
#endif

	ocrpt_expr_set_long(eo->e, value);
}

PHP_METHOD(opencreport_expr, set_double) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	double value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_DOUBLE(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d", &value) == FAILURE)
		return;
#endif

	ocrpt_expr_set_double(eo->e, value);
}

PHP_METHOD(opencreport_expr, set_number) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *value;
	int value_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &value, &value_len) == FAILURE)
		return;
#endif

	ocrpt_expr_set_number_from_string(eo->e, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, get_num_operands) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_get_num_operands(eo->e));
}

PHP_METHOD(opencreport_expr, operand_get_result) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_long opidx;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(opidx);
	ZEND_PARSE_PARAMETERS_END();
#else
	long opidx;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &opidx) == FAILURE)
		return;
#endif

	ocrpt_result *r = ocrpt_expr_operand_get_result(eo->e, opidx);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->r = r;
	ro->freed_by_lib = true;
}

PHP_METHOD(opencreport_expr, cmp_results) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_expr_cmp_results(eo->e));
}

PHP_METHOD(opencreport_expr, init_results) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_long result_type;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(result_type);
	ZEND_PARSE_PARAMETERS_END();
#else
	long result_type;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &result_type) == FAILURE)
		return;
#endif

	ocrpt_expr_init_results(eo->e, result_type);
}

PHP_METHOD(opencreport_expr, get_string) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	const ocrpt_string *s = ocrpt_expr_get_string(eo->e);

	if (!s)
		RETURN_NULL();

	OCRPT_RETURN_STRINGL(s->str, s->len);
}

PHP_METHOD(opencreport_expr, get_long) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_expr_get_long(eo->e));
}

PHP_METHOD(opencreport_expr, get_double) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_DOUBLE(ocrpt_expr_get_double(eo->e));
}

PHP_METHOD(opencreport_expr, get_number) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *format = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR(format);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *format = NULL;
	int format_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &format, &format_len) == FAILURE)
		return;
#endif

	char *fmt = format ? ZSTR_VAL(format) : "%RF";
	mpfr_ptr number = ocrpt_expr_get_number(eo->e);
	if (!number)
		RETURN_NULL();

	size_t len = mpfr_snprintf(NULL, 0, fmt, number);
	char *retval = emalloc(len + 1);
	mpfr_snprintf(retval, len + 1, fmt, number);

	OCRPT_RETURN_STRINGL(retval, len);
}

PHP_METHOD(opencreport_expr, set_nth_result_string) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_long which;
	zend_string *value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_LONG(which);
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	long which;
	char *value;
	int value_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &which, &value, &value_len) == FAILURE)
		return;
#endif

	ocrpt_expr_set_nth_result_string(eo->e, which, ZSTR_VAL(value));
}

PHP_METHOD(opencreport_expr, set_nth_result_long) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_long which;
	zend_long value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_LONG(which);
		Z_PARAM_LONG(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	long which;
	long value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &which, &value) == FAILURE)
		return;
#endif

	ocrpt_expr_set_nth_result_long(eo->e, which, value);
}

PHP_METHOD(opencreport_expr, set_nth_result_double) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

	double value;
#if PHP_VERSION_ID >= 70000
	zend_long which;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_LONG(which);
		Z_PARAM_DOUBLE(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	long which;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ld", &which, &value) == FAILURE)
		return;
#endif

	ocrpt_expr_set_nth_result_double(eo->e, which, value);
}

PHP_METHOD(opencreport_expr, set_iterative_start_value) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_bool value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_BOOL(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &value) == FAILURE)
		return;
#endif

	ocrpt_expr_set_iterative_start_value(eo->e, value);
}

PHP_METHOD(opencreport_expr, set_delayed) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);
	zend_bool value;

	if (!eo->e) {
		zend_throw_error(NULL, "OpenCReport\\Expr object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_BOOL(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &value) == FAILURE)
		return;
#endif

	ocrpt_expr_set_delayed(eo->e, value);
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_print, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_nodes, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_optimize, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_resolve, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_eval, 0, 0, OpenCReport\\Result, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_get_result, 0, 0, OpenCReport\\Result, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_string, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_long, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_double, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_number, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_num_operands, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_operand_get_result, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, opidx, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_cmp_results, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_init_results, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, result_type, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_string, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_long, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_double, 0, 0, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_number, 0, 0, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, format, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_string, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_long, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_nth_result_double, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_iterative_start_value, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_set_delayed, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_expr_free NULL
#define arginfo_opencreport_expr_print NULL
#define arginfo_opencreport_expr_nodes NULL
#define arginfo_opencreport_expr_optimize NULL
#define arginfo_opencreport_expr_resolve NULL
#define arginfo_opencreport_expr_eval NULL
#define arginfo_opencreport_expr_get_result NULL
#define arginfo_opencreport_expr_set_string NULL
#define arginfo_opencreport_expr_set_long NULL
#define arginfo_opencreport_expr_set_double NULL
#define arginfo_opencreport_expr_set_number NULL
#define arginfo_opencreport_expr_get_num_operands NULL
#define arginfo_opencreport_expr_operand_get_result NULL
#define arginfo_opencreport_expr_cmp_results NULL
#define arginfo_opencreport_expr_init_results NULL
#define arginfo_opencreport_expr_get_string NULL
#define arginfo_opencreport_expr_get_long NULL
#define arginfo_opencreport_expr_get_double NULL
#define arginfo_opencreport_expr_get_number NULL
#define arginfo_opencreport_expr_set_nth_result_string NULL
#define arginfo_opencreport_expr_set_nth_result_long NULL
#define arginfo_opencreport_expr_set_nth_result_double NULL
#define arginfo_opencreport_expr_set_iterative_start_value NULL
#define arginfo_opencreport_expr_set_delayed NULL

#endif

static const zend_function_entry opencreport_expr_class_methods[] = {
	PHP_ME(opencreport_expr, free, arginfo_opencreport_expr_free, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, print, arginfo_opencreport_expr_print, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, nodes, arginfo_opencreport_expr_nodes, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, optimize, arginfo_opencreport_expr_optimize, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, resolve, arginfo_opencreport_expr_resolve, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, eval, arginfo_opencreport_expr_eval, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_result, arginfo_opencreport_expr_get_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_string, arginfo_opencreport_expr_set_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_long, arginfo_opencreport_expr_set_long, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_double, arginfo_opencreport_expr_set_double, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_number, arginfo_opencreport_expr_set_number, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_num_operands, arginfo_opencreport_expr_get_num_operands, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, operand_get_result, arginfo_opencreport_expr_operand_get_result, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, cmp_results, arginfo_opencreport_expr_cmp_results, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, init_results, arginfo_opencreport_expr_init_results, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_string, arginfo_opencreport_expr_get_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_long, arginfo_opencreport_expr_get_long, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_double, arginfo_opencreport_expr_get_double, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, get_number, arginfo_opencreport_expr_get_number, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_string, arginfo_opencreport_expr_set_nth_result_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_long, arginfo_opencreport_expr_set_nth_result_long, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_nth_result_double, arginfo_opencreport_expr_set_nth_result_double, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_iterative_start_value, arginfo_opencreport_expr_set_iterative_start_value, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_expr, set_delayed, arginfo_opencreport_expr_set_delayed, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

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

PHP_METHOD(opencreport_part, get_next) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	if (!po->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Part object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = po->iter;
	ocrpt_part *p = ocrpt_part_get_next(po->o, &iter);

	if (!p)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_part_ce);
	php_opencreport_part_object *po1 = Z_OPENCREPORT_PART_P(return_value);

	po1->o = po->o;
	po1->p = p;
	po1->iter = iter;
	po1->is_iterator = true;
}

PHP_METHOD(opencreport_part, row_new) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_row_ce);
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(return_value);
	pro->pr = ocrpt_part_new_row(po->p);
}

PHP_METHOD(opencreport_part, row_get_first) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_part_row *pr = ocrpt_part_row_get_next(po->p, &iter);

	if (!pr)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_row_ce);
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(return_value);

	pro->p = po->p;
	pro->pr = pr;
	pro->iter = iter;
	pro->is_iterator = true;
}

PHP_METHOD(opencreport_part, add_iteration_cb) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *callback;
	int callback_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &callback, &callback_len) == FAILURE)
		return;
#endif

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	ocrpt_part_add_iteration_cb(po->p, opencreport_part_cb, cb_name);
}

PHP_METHOD(opencreport_part, equals) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zval *part;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(part, opencreport_part_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &part, opencreport_part_ce) == FAILURE)
		return;
#endif

	php_opencreport_part_object *po1 = Z_OPENCREPORT_PART_P(part);

	RETURN_BOOL(po->p == po1->p);
}

PHP_METHOD(opencreport_part, set_iterations) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_iterations(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_iterations) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_iterations(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_font_name) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_font_name(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_font_name) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_font_name(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_font_size) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_font_size(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_font_size) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_font_size(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_paper) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_paper_type(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_paper) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_paper_type(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_orientation) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_orientation(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_orientation) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_orientation(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_top_margin) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_top_margin(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_top_margin) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_top_margin(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_bottom_margin) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_bottom_margin(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_bottom_margin) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_bottom_margin(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_left_margin) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_left_margin(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_left_margin) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_left_margin(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_right_margin) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_right_margin(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_right_margin) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_right_margin(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_suppress) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_suppress(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_suppress) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_suppress(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, set_suppress_pageheader_firstpage) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
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

	ocrpt_expr *e = ocrpt_part_set_suppress_pageheader_firstpage(po->p, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, get_suppress_pageheader_firstpage) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_get_suppress_pageheader_firstpage(po->p);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part, page_header) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_part_page_header(po->p);
}

PHP_METHOD(opencreport_part, page_header_set_report) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zval *report = NULL;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(report, opencreport_report_ce, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O!", &report, opencreport_report_ce) == FAILURE)
		return;
#endif

	ocrpt_layout_part_page_header_set_report(po->p, report ? Z_OPENCREPORT_REPORT_P(report)->r : NULL);
}

PHP_METHOD(opencreport_part, page_footer) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_part_page_footer(po->p);
}

PHP_METHOD(opencreport_part, page_footer_set_report) {
	zval *object = getThis();
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(object);
	zval *report = NULL;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(report, opencreport_report_ce, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O!", &report, opencreport_report_ce) == FAILURE)
		return;
#endif

	ocrpt_layout_part_page_footer_set_report(po->p, report ? Z_OPENCREPORT_REPORT_P(report)->r : NULL);
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_next, 0, 0, OpenCReport\\Part, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_new, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_row_get_first, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_equals, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, part, OpenCReport\\Part, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_iterations, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_iterations, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_font_name, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_font_name, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_font_size, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_font_size, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_paper, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_paper, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_orientation, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_orientation, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_top_margin, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_top_margin, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_bottom_margin, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_bottom_margin, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_left_margin, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_left_margin, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_right_margin, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_right_margin, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_set_suppress_pageheader_firstpage, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_suppress_pageheader_firstpage, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_page_header, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_page_header_set_report, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, report, OpenCReport\\Report, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_page_footer, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_part_page_footer_set_report, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, report, OpenCReport\\Report, 0)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_part_get_next NULL
#define arginfo_opencreport_part_row_new NULL
#define arginfo_opencreport_part_row_get_first NULL
#define arginfo_opencreport_part_equals NULL
#define arginfo_opencreport_part_set_iterations NULL
#define arginfo_opencreport_part_get_iterations NULL
#define arginfo_opencreport_part_set_font_name NULL
#define arginfo_opencreport_part_get_font_name NULL
#define arginfo_opencreport_part_set_font_size NULL
#define arginfo_opencreport_part_get_font_size NULL
#define arginfo_opencreport_part_set_paper NULL
#define arginfo_opencreport_part_get_paper NULL
#define arginfo_opencreport_part_set_orientation NULL
#define arginfo_opencreport_part_get_orientation NULL
#define arginfo_opencreport_part_set_top_margin NULL
#define arginfo_opencreport_part_get_top_margin NULL
#define arginfo_opencreport_part_set_bottom_margin NULL
#define arginfo_opencreport_part_get_bottom_margin NULL
#define arginfo_opencreport_part_set_left_margin NULL
#define arginfo_opencreport_part_get_left_margin NULL
#define arginfo_opencreport_part_set_right_margin NULL
#define arginfo_opencreport_part_get_right_margin NULL
#define arginfo_opencreport_part_set_suppress NULL
#define arginfo_opencreport_part_get_suppress NULL
#define arginfo_opencreport_part_set_suppress_pageheader_firstpage NULL
#define arginfo_opencreport_part_get_suppress_pageheader_firstpage NULL
#define arginfo_opencreport_part_page_header NULL
#define arginfo_opencreport_part_page_header_set_report NULL
#define arginfo_opencreport_part_page_footer NULL
#define arginfo_opencreport_part_page_footer_set_report NULL

#endif

static const zend_function_entry opencreport_part_class_methods[] = {
	PHP_ME(opencreport_part, get_next, arginfo_opencreport_part_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, row_new, arginfo_opencreport_part_row_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, row_get_first, arginfo_opencreport_part_row_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, add_iteration_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, equals, arginfo_opencreport_part_equals, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_iterations, arginfo_opencreport_part_set_iterations, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_iterations, arginfo_opencreport_part_get_iterations, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_font_name, arginfo_opencreport_part_set_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_font_name, arginfo_opencreport_part_get_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_font_size, arginfo_opencreport_part_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_font_size, arginfo_opencreport_part_get_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_paper, arginfo_opencreport_part_set_paper, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_paper, arginfo_opencreport_part_get_paper, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_orientation, arginfo_opencreport_part_set_orientation, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_orientation, arginfo_opencreport_part_get_orientation, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_top_margin, arginfo_opencreport_part_set_top_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_top_margin, arginfo_opencreport_part_get_top_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_bottom_margin, arginfo_opencreport_part_set_bottom_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_bottom_margin, arginfo_opencreport_part_get_bottom_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_left_margin, arginfo_opencreport_part_set_left_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_left_margin, arginfo_opencreport_part_get_left_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_right_margin, arginfo_opencreport_part_set_right_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_right_margin, arginfo_opencreport_part_get_right_margin, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_suppress, arginfo_opencreport_part_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_suppress, arginfo_opencreport_part_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, set_suppress_pageheader_firstpage, arginfo_opencreport_part_set_suppress_pageheader_firstpage, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, get_suppress_pageheader_firstpage, arginfo_opencreport_part_get_suppress_pageheader_firstpage, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, page_header, arginfo_opencreport_part_page_header, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, page_header_set_report, arginfo_opencreport_part_page_header_set_report, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, page_footer, arginfo_opencreport_part_page_footer, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part, page_footer_set_report, arginfo_opencreport_part_page_footer_set_report, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_row, get_next) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	if (!pro->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Row object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = pro->iter;
	ocrpt_part_row *pr = ocrpt_part_row_get_next(pro->p, &iter);

	if (!pr)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_row_ce);
	php_opencreport_row_object *pro1 = Z_OPENCREPORT_ROW_P(return_value);

	pro1->p = pro->p;
	pro1->pr = pr;
	pro1->iter = iter;
	pro1->is_iterator = true;
}

PHP_METHOD(opencreport_row, column_new) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_col_ce);
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(return_value);
	pco->pc = ocrpt_part_row_new_column(pro->pr);
	pco->pr = NULL;
	pco->iter = NULL;
	pco->is_iterator = false;
}

PHP_METHOD(opencreport_row, column_get_first) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_part_column *pc = ocrpt_part_column_get_next(pro->pr, &iter);
	if (!pc)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_col_ce);
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(return_value);

	pco->pr = pro->pr;
	pco->pc = pc;
	pco->iter = iter;
	pco->is_iterator = true;
}

PHP_METHOD(opencreport_row, set_suppress) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);
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

	ocrpt_expr *e = ocrpt_part_row_set_suppress(pro->pr, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_row, get_suppress) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_row_get_suppress(pro->pr);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_row, set_newpage) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);
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

	ocrpt_expr *e = ocrpt_part_row_set_newpage(pro->pr, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_row, get_newpage) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_row_get_newpage(pro->pr);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_row, set_layout) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);
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

	ocrpt_expr *e = ocrpt_part_row_set_layout(pro->pr, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_row, get_layout) {
	zval *object = getThis();
	php_opencreport_row_object *pro = Z_OPENCREPORT_ROW_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_row_get_layout(pro->pr);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_get_next, 0, 0, OpenCReport\\Row, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_column_new, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_column_get_first, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_set_newpage, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_get_newpage, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_set_layout, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_row_get_layout, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_row_get_next NULL
#define arginfo_opencreport_row_column_new NULL
#define arginfo_opencreport_row_column_get_first NULL
#define arginfo_opencreport_row_set_suppress NULL
#define arginfo_opencreport_row_get_suppress NULL
#define arginfo_opencreport_row_set_newpage NULL
#define arginfo_opencreport_row_get_newpage NULL
#define arginfo_opencreport_row_set_layout NULL
#define arginfo_opencreport_row_get_layout NULL

#endif

static const zend_function_entry opencreport_row_class_methods[] = {
	PHP_ME(opencreport_row, get_next, arginfo_opencreport_row_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, column_new, arginfo_opencreport_row_column_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, column_get_first, arginfo_opencreport_row_column_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, set_suppress, arginfo_opencreport_row_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, get_suppress, arginfo_opencreport_row_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, set_newpage, arginfo_opencreport_row_set_newpage, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, get_newpage, arginfo_opencreport_row_get_newpage, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, set_layout, arginfo_opencreport_row_set_layout, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_row, get_layout, arginfo_opencreport_row_get_layout, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_part_col, get_next) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	if (!pco->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Column object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = pco->iter;
	ocrpt_part_column *pc = ocrpt_part_column_get_next(pco->pr, &iter);

	if (!pc)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_col_ce);
	php_opencreport_col_object *pco1 = Z_OPENCREPORT_COL_P(return_value);

	pco1->pr = pco->pr;
	pco1->pc = pc;
	pco1->iter = iter;
	pco1->is_iterator = true;
}

PHP_METHOD(opencreport_part_col, report_new) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_report *r = ocrpt_part_column_new_report(pco->pc);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(return_value);

	pro->pc = NULL;
	pro->r = r;
	pro->iter = NULL;
	pro->is_iterator = false;
}

PHP_METHOD(opencreport_part_col, report_get_first) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_report *r = ocrpt_report_get_next(pco->pc, &iter);

	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(return_value);

	pro->pc = pco->pc;
	pro->r = r;
	pro->iter = iter;
	pro->is_iterator = true;
}

PHP_METHOD(opencreport_part_col, set_suppress) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
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

	ocrpt_expr *e = ocrpt_part_column_set_suppress(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, get_suppress) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_column_get_suppress(pco->pc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, set_width) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
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

	ocrpt_expr *e = ocrpt_part_column_set_width(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, get_width) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_column_get_width(pco->pc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, set_height) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
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

	ocrpt_expr *e = ocrpt_part_column_set_height(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, get_height) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_column_get_height(pco->pc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, set_border_width) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
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

	ocrpt_expr *e = ocrpt_part_column_set_border_width(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, get_border_width) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_column_get_border_width(pco->pc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, set_border_color) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
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

	ocrpt_expr *e = ocrpt_part_column_set_border_color(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, get_border_color) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_column_get_border_color(pco->pc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, set_detail_columns) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
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

	ocrpt_expr *e = ocrpt_part_column_set_detail_columns(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, get_detail_columns) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_column_get_detail_columns(pco->pc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, set_column_padding) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);
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

	ocrpt_expr *e = ocrpt_part_column_set_column_padding(pco->pc, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_part_col, get_column_padding) {
	zval *object = getThis();
	php_opencreport_col_object *pco = Z_OPENCREPORT_COL_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_part_column_get_column_padding(pco->pc);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_next, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_report_new, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_report_get_first, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_set_width, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_width, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_set_height, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_height, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_set_border_width, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_border_width, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_set_border_color, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_border_color, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_set_detail_columns, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_detail_columns, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_set_column_padding, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_col_get_column_padding, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_part_col_get_next NULL
#define arginfo_opencreport_part_col_report_new NULL
#define arginfo_opencreport_part_col_report_get_first NULL
#define arginfo_opencreport_part_col_set_suppress NULL
#define arginfo_opencreport_part_col_get_suppress NULL
#define arginfo_opencreport_part_col_set_width NULL
#define arginfo_opencreport_part_col_get_width NULL
#define arginfo_opencreport_part_col_set_height NULL
#define arginfo_opencreport_part_col_get_height NULL
#define arginfo_opencreport_part_col_set_border_width NULL
#define arginfo_opencreport_part_col_get_border_width NULL
#define arginfo_opencreport_part_col_set_border_color NULL
#define arginfo_opencreport_part_col_get_border_color NULL
#define arginfo_opencreport_part_col_set_detail_columns NULL
#define arginfo_opencreport_part_col_get_detail_columns NULL
#define arginfo_opencreport_part_col_set_column_padding NULL
#define arginfo_opencreport_part_col_get_column_padding NULL

#endif

static const zend_function_entry opencreport_part_col_class_methods[] = {
	PHP_ME(opencreport_part_col, get_next, arginfo_opencreport_part_col_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, report_new, arginfo_opencreport_part_col_report_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, report_get_first, arginfo_opencreport_part_col_report_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_suppress, arginfo_opencreport_part_col_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, get_suppress, arginfo_opencreport_part_col_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_width, arginfo_opencreport_part_col_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, get_width, arginfo_opencreport_part_col_get_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_height, arginfo_opencreport_part_col_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, get_height, arginfo_opencreport_part_col_get_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_border_width, arginfo_opencreport_part_col_set_border_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, get_border_width, arginfo_opencreport_part_col_get_border_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_border_color, arginfo_opencreport_part_col_set_border_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, get_border_color, arginfo_opencreport_part_col_get_border_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_detail_columns, arginfo_opencreport_part_col_set_detail_columns, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, get_detail_columns, arginfo_opencreport_part_col_get_detail_columns, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, set_column_padding, arginfo_opencreport_part_col_set_column_padding, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_part_col, get_column_padding, arginfo_opencreport_part_col_get_column_padding, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_report, get_next) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	if (!pro->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\Report object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = pro->iter;
	ocrpt_report *r = ocrpt_report_get_next(pro->pc, &iter);

	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_report_ce);
	php_opencreport_report_object *pro1 = Z_OPENCREPORT_REPORT_P(return_value);

	pro1->pc = pro->pc;
	pro1->r = r;
	pro1->iter = iter;
	pro1->is_iterator = true;
}

PHP_METHOD(opencreport_report, variable_new) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_long variable_type;
	zend_string *name;
	zend_string *expr;
	zend_string *reset_on_break_name = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3,4)
		Z_PARAM_LONG(variable_type);
		Z_PARAM_STR(name);
		Z_PARAM_STR(expr);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(reset_on_break_name, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	long variable_type;
	char *name;
	char *expr;
	char *reset_on_break_name = NULL;
	int name_len, expr_len, reset_on_break_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lss|s!", &variable_type, &name, &name_len, &expr, &expr_len, &reset_on_break_name, &reset_on_break_name_len)== FAILURE)
		return;
#endif

	ocrpt_var *v = ocrpt_variable_new(pro->r, variable_type, ZSTR_VAL(name), ZSTR_VAL(expr), reset_on_break_name ? ZSTR_VAL(reset_on_break_name) : NULL);
	if (!v)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_variable_ce);
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(return_value);
	vo->v = v;
}

PHP_METHOD(opencreport_report, variable_new_full) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_long result_type;
	zend_string *name;
	zend_string *baseexpr = NULL;
	zend_string *intermedexpr = NULL;
	zend_string *intermed2expr = NULL;
	zend_string *resultexpr = NULL;
	zend_string *reset_on_break_name = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 7)
		Z_PARAM_LONG(result_type);
		Z_PARAM_STR(name);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(baseexpr, 1, 0);
		Z_PARAM_STR_EX(intermedexpr, 1, 0);
		Z_PARAM_STR_EX(intermed2expr, 1, 0);
		Z_PARAM_STR_EX(resultexpr, 1, 0);
		Z_PARAM_STR_EX(reset_on_break_name, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	long result_type;
	char *name;
	char *baseexpr = NULL;
	char *intermedexpr = NULL;
	char *intermed2expr = NULL;
	char *resultexpr = NULL;
	char *reset_on_break_name = NULL;
	int name_len, baseexpr_len, intermedexpr_len, intermed2expr_len;
	int resultexpr_len, reset_on_break_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls|s!s!s!s!s!", &result_type, &name, &name_len, &baseexpr, &baseexpr_len, &intermedexpr, &intermedexpr_len, &intermed2expr, &intermed2expr_len, &resultexpr, &resultexpr_len, &reset_on_break_name, &reset_on_break_name_len) == FAILURE)
		return;
#endif

	ocrpt_var *v = ocrpt_variable_new_full(pro->r, result_type, ZSTR_VAL(name),
												baseexpr ? ZSTR_VAL(baseexpr) : NULL,
												intermedexpr ? ZSTR_VAL(intermedexpr) : NULL,
												intermed2expr ? ZSTR_VAL(intermed2expr) : NULL,
												resultexpr ? ZSTR_VAL(resultexpr) : NULL,
												reset_on_break_name ? ZSTR_VAL(reset_on_break_name) : NULL);

	if (!v)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_variable_ce);
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(return_value);
	vo->v = v;
}

PHP_METHOD(opencreport_report, expr_parse) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *expr_string;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(expr_string);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_string;
	int expr_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &expr_string, &expr_string_len) == FAILURE)
		return;
#endif

	char *err = NULL;
	ocrpt_expr *e = ocrpt_report_expr_parse(pro->r, ZSTR_VAL(expr_string), &err);

	if (e) {
		object_init_ex(return_value, opencreport_expr_ce);
		php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
		eo->e = e;
		ocrpt_strfree(pro->expr_error);
		pro->expr_error = NULL;
	} else {
		pro->expr_error = err;
		RETURN_NULL();
	}
}

PHP_METHOD(opencreport_report, expr_error) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (pro->expr_error) {
		OCRPT_RETURN_STRING(pro->expr_error);
	} else
		RETURN_NULL();
}

PHP_METHOD(opencreport_report, resolve_variables) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_report_resolve_variables(pro->r);
}

PHP_METHOD(opencreport_report, evaluate_variables) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_report_evaluate_variables(pro->r);
}

PHP_METHOD(opencreport_report, break_new) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *name;
	int name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE)
		return;
#endif

	ocrpt_break *br = ocrpt_break_new(pro->r, ZSTR_VAL(name));
	if (!br)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(return_value);
	bro->br = br;
}

PHP_METHOD(opencreport_report, break_get_first) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_break *br = ocrpt_break_get_next(pro->r, &iter);
	if (!br)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(return_value);
	bro->r = pro->r;
	bro->br = br;
	bro->iter = iter;
	bro->is_iterator = true;
}

PHP_METHOD(opencreport_report, resolve_breaks) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_report_resolve_breaks(pro->r);
}

PHP_METHOD(opencreport_report, get_query_rownum) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_report_get_query_rownum(pro->r));
}

PHP_METHOD(opencreport_report, add_start_cb) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *callback;
	int callback_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &callback, &callback_len) == FAILURE)
		return;
#endif

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	ocrpt_report_add_start_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_done_cb) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *callback;
	int callback_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &callback, &callback_len) == FAILURE)
		return;
#endif

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	ocrpt_report_add_done_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_new_row_cb) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *callback;
	int callback_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &callback, &callback_len) == FAILURE)
		return;
#endif

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	ocrpt_report_add_new_row_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_iteration_cb) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *callback;
	int callback_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &callback, &callback_len) == FAILURE)
		return;
#endif

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	ocrpt_report_add_iteration_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, add_precalculation_done_cb) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *callback;
	int callback_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &callback, &callback_len) == FAILURE)
		return;
#endif

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	ocrpt_report_add_precalculation_done_cb(pro->r, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport_report, break_get) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *break_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(break_name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *break_name;
	int break_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &break_name, &break_name_len) == FAILURE)
		return;
#endif

	ocrpt_break *br = ocrpt_break_get(pro->r, ZSTR_VAL(break_name));

	if (!br)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(return_value);

	bro->r = pro->r;
	bro->br = br;
	bro->iter = NULL;
	bro->is_iterator = false;
}

PHP_METHOD(opencreport_report, equals) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
	zval *report;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(report, opencreport_report_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &report, opencreport_report_ce) == FAILURE)
		return;
#endif

	php_opencreport_report_object *pro1 = Z_OPENCREPORT_REPORT_P(report);

	RETURN_BOOL(pro->r == pro1->r);
}

PHP_METHOD(opencreport_report, set_main_query) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
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

	ocrpt_report_set_main_query(pro->r, qo->q);
}

PHP_METHOD(opencreport_report, set_main_query_by_name) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *query_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(query_name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *query_name;
	int query_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &query_name, &query_name_len) == FAILURE)
		return;
#endif

	ocrpt_report_set_main_query_from_expr(pro->r, ZSTR_VAL(query_name));
}

PHP_METHOD(opencreport_report, set_suppress) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
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

	ocrpt_expr *e = ocrpt_report_set_suppress(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, get_suppress) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_report_get_suppress(pro->r);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, set_iterations) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
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

	ocrpt_expr *e = ocrpt_report_set_iterations(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, get_iterations) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_report_get_iterations(pro->r);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, set_font_name) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
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

	ocrpt_expr *e = ocrpt_report_set_font_name(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, get_font_name) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_report_get_font_name(pro->r);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, set_font_size) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
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

	ocrpt_expr *e = ocrpt_report_set_font_size(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, get_font_size) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_report_get_font_size(pro->r);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, set_height) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
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

	ocrpt_expr *e = ocrpt_report_set_height(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, get_height) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_report_get_height(pro->r);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, set_fieldheader_priority) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
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

	ocrpt_expr *e = ocrpt_report_set_fieldheader_priority(pro->r, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, get_fieldheader_priority) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_report_get_fieldheader_priority(pro->r);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_report, nodata) {
	zval *object = getThis();
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_nodata(ro->r);
}

PHP_METHOD(opencreport_report, header) {
	zval *object = getThis();
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_header(ro->r);
}

PHP_METHOD(opencreport_report, footer) {
	zval *object = getThis();
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_footer(ro->r);
}

PHP_METHOD(opencreport_report, field_header) {
	zval *object = getThis();
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_field_header(ro->r);
}

PHP_METHOD(opencreport_report, field_details) {
	zval *object = getThis();
	php_opencreport_report_object *ro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_layout_report_field_details(ro->r);
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_get_next, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_new, 0, 3, OpenCReport\\Variable, 1)
ZEND_ARG_TYPE_INFO(0, variable_type, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, expr, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, reset_on_break_name, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_new_full, 0, 2, OpenCReport\\Variable, 1)
ZEND_ARG_TYPE_INFO(0, result_type, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, baseexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, intermedexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, intermed2expr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, resultexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, reset_on_break_name, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_expr_parse, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_expr_error, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_resolve_variables, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_evaluate_variables, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_new, 0, 1, OpenCReport\\ReportBreak, 1)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_get_first, 0, 0, OpenCReport\\ReportBreak, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_resolve_breaks, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_get_query_rownum, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_get, 0, 1, OpenCReport\\ReportBreak, 1)
ZEND_ARG_TYPE_INFO(0, break_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_equals, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, report, OpenCReport\\Report, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_main_query, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, query, OpenCReport\\Query, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_report_set_main_query_by_name, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, query_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_set_iterations, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_get_iterations, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_set_font_name, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_get_font_name, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_set_font_size, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_get_font_size, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_set_height, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_get_height, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_set_fieldheader_priority, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_get_fieldheader_priority, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_nodata, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_header, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_footer, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_field_header, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_field_details, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_report_get_next NULL
#define arginfo_opencreport_variable_new NULL
#define arginfo_opencreport_variable_new_full NULL
#define arginfo_opencreport_report_expr_parse NULL
#define arginfo_opencreport_report_expr_error NULL
#define arginfo_opencreport_report_resolve_variables NULL
#define arginfo_opencreport_report_evaluate_variables NULL
#define arginfo_opencreport_break_new NULL
#define arginfo_opencreport_break_get_first NULL
#define arginfo_opencreport_report_resolve_breaks NULL
#define arginfo_opencreport_report_get_query_rownum NULL
#define arginfo_opencreport_break_get NULL
#define arginfo_opencreport_report_equals NULL
#define arginfo_opencreport_report_set_main_query NULL
#define arginfo_opencreport_report_set_main_query_by_name NULL
#define arginfo_opencreport_report_set_suppress NULL
#define arginfo_opencreport_report_get_suppress NULL
#define arginfo_opencreport_report_set_iterations NULL
#define arginfo_opencreport_report_get_iterations NULL
#define arginfo_opencreport_report_set_font_name NULL
#define arginfo_opencreport_report_get_font_name NULL
#define arginfo_opencreport_report_set_font_size NULL
#define arginfo_opencreport_report_get_font_size NULL
#define arginfo_opencreport_report_set_height NULL
#define arginfo_opencreport_report_get_height NULL
#define arginfo_opencreport_report_set_fieldheader_priority NULL
#define arginfo_opencreport_report_get_fieldheader_priority NULL
#define arginfo_opencreport_report_nodata NULL
#define arginfo_opencreport_report_header NULL
#define arginfo_opencreport_report_footer NULL
#define arginfo_opencreport_report_field_header NULL
#define arginfo_opencreport_report_field_details NULL

#endif

static const zend_function_entry opencreport_report_class_methods[] = {
	PHP_ME(opencreport_report, get_next, arginfo_opencreport_report_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_main_query, arginfo_opencreport_report_set_main_query, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_main_query_by_name, arginfo_opencreport_report_set_main_query_by_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, variable_new, arginfo_opencreport_variable_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, variable_new_full, arginfo_opencreport_variable_new_full, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, expr_parse, arginfo_opencreport_report_expr_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, expr_error, arginfo_opencreport_report_expr_error, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, resolve_variables, arginfo_opencreport_report_resolve_variables, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, evaluate_variables, arginfo_opencreport_report_evaluate_variables, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_new, arginfo_opencreport_break_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_get_first, arginfo_opencreport_break_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, resolve_breaks, arginfo_opencreport_report_resolve_breaks, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_query_rownum, arginfo_opencreport_report_get_query_rownum, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_start_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_new_row_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_iteration_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, add_precalculation_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, break_get, arginfo_opencreport_break_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, equals, arginfo_opencreport_report_equals, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_suppress, arginfo_opencreport_report_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_suppress, arginfo_opencreport_report_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_iterations, arginfo_opencreport_report_set_iterations, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_iterations, arginfo_opencreport_report_get_iterations, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_font_name, arginfo_opencreport_report_set_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_font_name, arginfo_opencreport_report_get_font_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_font_size, arginfo_opencreport_report_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_font_size, arginfo_opencreport_report_get_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_height, arginfo_opencreport_report_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_height, arginfo_opencreport_report_get_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, set_fieldheader_priority, arginfo_opencreport_report_set_fieldheader_priority, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, get_fieldheader_priority, arginfo_opencreport_report_get_fieldheader_priority, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, nodata, arginfo_opencreport_report_nodata, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, header, arginfo_opencreport_report_header, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, footer, arginfo_opencreport_report_footer, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, field_header, arginfo_opencreport_report_field_header, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, field_details, arginfo_opencreport_report_field_details, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_variable, baseexpr) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_baseexpr(vo->v);

	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_variable, intermedexpr) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_intermedexpr(vo->v);

	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_variable, intermed2expr) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_intermed2expr(vo->v);

	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_variable, resultexpr) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_resultexpr(vo->v);

	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_variable, set_precalculate) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);
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

	ocrpt_variable_set_precalculate(vo->v, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport_variable, resolve) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_variable_resolve(vo->v);
}

PHP_METHOD(opencreport_variable, eval) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_variable_evaluate(vo->v);
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_baseexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_intermedexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_intermed2expr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_resultexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_set_precalculate, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_resolve, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_eval, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_variable_baseexpr NULL
#define arginfo_opencreport_variable_intermedexpr NULL
#define arginfo_opencreport_variable_intermed2expr NULL
#define arginfo_opencreport_variable_resultexpr NULL
#define arginfo_opencreport_variable_set_precalculate NULL
#define arginfo_opencreport_variable_resolve NULL
#define arginfo_opencreport_variable_eval NULL

#endif

static const zend_function_entry opencreport_variable_class_methods[] = {
	PHP_ME(opencreport_variable, baseexpr, arginfo_opencreport_variable_baseexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, intermedexpr, arginfo_opencreport_variable_intermedexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, intermed2expr, arginfo_opencreport_variable_intermed2expr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, resultexpr, arginfo_opencreport_variable_resultexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, set_precalculate, arginfo_opencreport_variable_set_precalculate, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, resolve, arginfo_opencreport_variable_resolve, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, eval, arginfo_opencreport_variable_eval, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

PHP_METHOD(opencreport_break, get_next) {
	zval *object = getThis();
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	if (!bro->is_iterator) {
		zend_throw_error(NULL, "OpenCReport\\ReportBreak object is not an iterator");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = bro->iter;
	ocrpt_break *br = ocrpt_break_get_next(bro->r, &iter);
	if (!br)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_break_ce);
	php_opencreport_break_object *bro1 = Z_OPENCREPORT_BREAK_P(return_value);
	bro1->r = bro->r;
	bro1->br = br;
	bro1->iter = iter;
	bro1->is_iterator = true;
}

PHP_METHOD(opencreport_break, breakfield_add) {
	zval *object = getThis();
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);
	zval *breakfield_expr;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(breakfield_expr, opencreport_expr_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &breakfield_expr, opencreport_expr_ce) == FAILURE)
		return;
#endif

	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(breakfield_expr);

	bool retval = ocrpt_break_add_breakfield(bro->br, eo->e);

	/* The expression's ownership was taken over or it was freed */
	eo->e = NULL;

	RETURN_BOOL(retval);
}

PHP_METHOD(opencreport_break, check_fields) {
	zval *object = getThis();
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_break_check_fields(bro->br));
}

PHP_METHOD(opencreport_break, reset_vars) {
	zval *object = getThis();
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_break_reset_vars(bro->br);
}

static void opencreport_break_cb(opencreport *o, ocrpt_report *r, ocrpt_break *br, void *data) {
	char *callback = data;

#if PHP_VERSION_ID >= 70000
	zval zfname;
	ZVAL_STRING(&zfname, callback);

	zval retval;
	zval params[3];

	object_init_ex(&params[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(&params[0]);
	oo->o = o;

	object_init_ex(&params[1], opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(&params[1]);
	pro->r = r;

	object_init_ex(&params[2], opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(&params[2]);
	bro->r = r;
	bro->br = br;
#else
	zval *zfname;
	MAKE_STD_ZVAL(zfname);
	ZVAL_STRING(zfname, callback, 1);

	zval *retval = NULL;
	zval *params0[3];
	zval ***params = emalloc(sizeof(zval **) * 3);

	ALLOC_INIT_ZVAL(params0[0]);
	object_init_ex(params0[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(params0[0]);
	oo->o = o;

	ALLOC_INIT_ZVAL(params0[1]);
	object_init_ex(params0[1], opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(params0[1]);
	pro->r = r;

	ALLOC_INIT_ZVAL(params0[2]);
	object_init_ex(params0[2], opencreport_break_ce);
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(params0[2]);
	bro->r = r;
	bro->br = br;

	for (int32_t i = 0; i < 3; i++)
		params[i] = &params0[i];
#endif

#if PHP_VERSION_ID >= 80000
	call_user_function(CG(function_table), NULL, &zfname, &retval, 3, params);
#elif PHP_VERSION_ID >= 70000
	call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 3, params, 0, NULL TSRMLS_CC);
#else
	call_user_function_ex(CG(function_table), NULL, zfname, &retval, 3, params, 0, NULL TSRMLS_CC);
#endif

#if PHP_VERSION_ID >= 70000
	zend_object_std_dtor(&oo->zo);
	OBJ_RELEASE(&oo->zo);
	zend_object_std_dtor(&pro->zo);
	OBJ_RELEASE(&pro->zo);
	zend_object_std_dtor(&bro->zo);
	OBJ_RELEASE(&bro->zo);

	zend_string_release(Z_STR(zfname));
#else
	zval_dtor(zfname);
	efree(zfname);
	efree(params);
	zend_object_std_dtor(&oo->zo);
	zend_object_std_dtor(&pro->zo);
	zend_object_std_dtor(&bro->zo);
	for (int32_t i = 0; i < 3; i++) {
		if (Z_REFCOUNT_P(params0[i]) == 1) {
			zval_dtor(params0[i]);
			efree(params0[i]);
		}
	}
	if (retval) {
		zval_dtor(retval);
		efree(retval);
	}
#endif
}

PHP_METHOD(opencreport_break, add_trigger_cb) {
	zval *object = getThis();
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *callback;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(callback);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *callback;
	int callback_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &callback, &callback_len) == FAILURE)
		return;
#endif

	char *cb_name = ocrpt_mem_strdup(ZSTR_VAL(callback));

	ocrpt_break_add_trigger_cb(bro->br, opencreport_break_cb, cb_name);
}

PHP_METHOD(opencreport_break, name) {
	zval *object = getThis();
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	OCRPT_RETURN_STRING(ocrpt_break_get_name(bro->br));
}

PHP_METHOD(opencreport_break, header) {
	zval *object = getThis();
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_break_get_header(bro->br);
}

PHP_METHOD(opencreport_break, footer) {
	zval *object = getThis();
	php_opencreport_break_object *bro = Z_OPENCREPORT_BREAK_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_output_ce);
	php_opencreport_output_object *out = Z_OPENCREPORT_OUTPUT_P(return_value);

	out->output = ocrpt_break_get_footer(bro->br);
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_get_next, 0, 0, OpenCReport\\ReportBreak, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_break_breakfield_add, 0, 1, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, breakfield_expr, OpenCReport\\Expr, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_break_check_fields, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_break_reset_vars, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_break_name, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_header, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_break_footer, 0, 0, OpenCReport\\Output, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_break_get_next NULL
#define arginfo_opencreport_break_breakfield_add NULL
#define arginfo_opencreport_break_check_fields NULL
#define arginfo_opencreport_break_reset_vars NULL
#define arginfo_opencreport_break_name NULL
#define arginfo_opencreport_break_header NULL
#define arginfo_opencreport_break_footer NULL

#endif

static const zend_function_entry opencreport_break_class_methods[] = {
	PHP_ME(opencreport_break, get_next, arginfo_opencreport_break_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, breakfield_add, arginfo_opencreport_break_breakfield_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, check_fields, arginfo_opencreport_break_check_fields, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, reset_vars, arginfo_opencreport_break_reset_vars, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, add_trigger_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, name, arginfo_opencreport_break_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, header, arginfo_opencreport_break_header, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_break, footer, arginfo_opencreport_break_footer, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

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

	void *iter = NULL;
	ocrpt_line_element *le = ocrpt_line_iterate_elements(lo->line, &iter);
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

PHP_METHOD(opencreport_hline, set_size) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
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

	ocrpt_expr *e = ocrpt_hline_set_size(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, get_size) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_hline_get_size(hlo->hline);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, set_alignment) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
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

	ocrpt_expr *e = ocrpt_hline_set_alignment(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, get_alignment) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_hline_get_alignment(hlo->hline);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, set_indentation) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
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

	ocrpt_expr *e = ocrpt_hline_set_indentation(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, get_indentation) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_hline_get_indentation(hlo->hline);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, set_length) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
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

	ocrpt_expr *e = ocrpt_hline_set_length(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, get_length) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_hline_get_length(hlo->hline);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, set_font_size) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
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

	ocrpt_expr *e = ocrpt_hline_set_font_size(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, get_font_size) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_hline_get_font_size(hlo->hline);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, set_suppress) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
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

	ocrpt_expr *e = ocrpt_hline_set_suppress(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, get_suppress) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_hline_get_suppress(hlo->hline);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, set_color) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);
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

	ocrpt_expr *e = ocrpt_hline_set_color(hlo->hline, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport_hline, get_color) {
	zval *object = getThis();
	php_opencreport_hline_object *hlo = Z_OPENCREPORT_HLINE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_hline_get_color(hlo->hline);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_set_size, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_get_size, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_set_alignment, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_get_alignment, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_set_indentation, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_get_indentation, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_set_length, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_get_length, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_set_font_size, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_get_font_size, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_set_color, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_hline_get_color, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_hline_set_size NULL
#define arginfo_opencreport_hline_get_size NULL
#define arginfo_opencreport_hline_set_alignment NULL
#define arginfo_opencreport_hline_get_alignment NULL
#define arginfo_opencreport_hline_set_indentation NULL
#define arginfo_opencreport_hline_get_indentation NULL
#define arginfo_opencreport_hline_set_length NULL
#define arginfo_opencreport_hline_get_length NULL
#define arginfo_opencreport_hline_set_font_size NULL
#define arginfo_opencreport_hline_get_font_size NULL
#define arginfo_opencreport_hline_set_suppress NULL
#define arginfo_opencreport_hline_get_suppress NULL
#define arginfo_opencreport_hline_set_color NULL
#define arginfo_opencreport_hline_get_color NULL

#endif

static const zend_function_entry opencreport_hline_class_methods[] = {
	PHP_ME(opencreport_hline, set_size, arginfo_opencreport_hline_set_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, get_size, arginfo_opencreport_hline_get_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_alignment, arginfo_opencreport_hline_set_alignment, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, get_alignment, arginfo_opencreport_hline_get_alignment, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_indentation, arginfo_opencreport_hline_set_indentation, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, get_indentation, arginfo_opencreport_hline_get_indentation, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_length, arginfo_opencreport_hline_set_length, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, get_length, arginfo_opencreport_hline_get_length, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_font_size, arginfo_opencreport_hline_set_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, get_font_size, arginfo_opencreport_hline_get_font_size, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_suppress, arginfo_opencreport_hline_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, get_suppress, arginfo_opencreport_hline_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, set_color, arginfo_opencreport_hline_set_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_hline, get_color, arginfo_opencreport_hline_get_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

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

	memcpy(&opencreport_expr_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Expr", opencreport_expr_class_methods);
	ce.create_object = opencreport_expr_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_expr_object_handlers.offset = XtOffsetOf(php_opencreport_expr_object, zo);
#endif
	opencreport_expr_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_expr_object_handlers.compare = zend_objects_not_comparable;
#endif
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

	memcpy(&opencreport_part_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Part", opencreport_part_class_methods);
	ce.create_object = opencreport_part_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_part_object_handlers.offset = XtOffsetOf(php_opencreport_part_object, zo);
#endif
	opencreport_part_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_part_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_part_ce = zend_register_internal_class(&ce);
	opencreport_part_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_part_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_part_ce->serialize = zend_class_serialize_deny;
	opencreport_part_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_row_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Row", opencreport_row_class_methods);
	ce.create_object = opencreport_row_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_row_object_handlers.offset = XtOffsetOf(php_opencreport_row_object, zo);
#endif
	opencreport_row_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_row_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_row_ce = zend_register_internal_class(&ce);
	opencreport_row_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_row_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_row_ce->serialize = zend_class_serialize_deny;
	opencreport_row_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_col_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Column", opencreport_part_col_class_methods);
	ce.create_object = opencreport_col_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_col_object_handlers.offset = XtOffsetOf(php_opencreport_col_object, zo);
#endif
	opencreport_col_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_col_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_col_ce = zend_register_internal_class(&ce);
	opencreport_col_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_col_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_col_ce->serialize = zend_class_serialize_deny;
	opencreport_col_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_report_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Report", opencreport_report_class_methods);
	ce.create_object = opencreport_report_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_report_object_handlers.offset = XtOffsetOf(php_opencreport_report_object, zo);
	opencreport_report_object_handlers.free_obj = opencreport_report_object_free;
#endif
	opencreport_report_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_report_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_report_ce = zend_register_internal_class(&ce);
	opencreport_report_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_report_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_report_ce->serialize = zend_class_serialize_deny;
	opencreport_report_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_variable_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Variable", opencreport_variable_class_methods);
	ce.create_object = opencreport_variable_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_variable_object_handlers.offset = XtOffsetOf(php_opencreport_variable_object, zo);
#endif
	opencreport_variable_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_variable_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_variable_ce = zend_register_internal_class(&ce);
	opencreport_variable_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_variable_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_variable_ce->serialize = zend_class_serialize_deny;
	opencreport_variable_ce->unserialize = zend_class_unserialize_deny;
#endif

	memcpy(&opencreport_break_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "ReportBreak", opencreport_break_class_methods);
	ce.create_object = opencreport_break_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_break_object_handlers.offset = XtOffsetOf(php_opencreport_break_object, zo);
#endif
	opencreport_break_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_break_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_break_ce = zend_register_internal_class(&ce);
	opencreport_break_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_break_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_break_ce->serialize = zend_class_serialize_deny;
	opencreport_break_ce->unserialize = zend_class_unserialize_deny;
#endif

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

	memcpy(&opencreport_hline_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "HorizontalLine", opencreport_hline_class_methods);
	ce.create_object = opencreport_hline_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_hline_object_handlers.offset = XtOffsetOf(php_opencreport_hline_object, zo);
#endif
	opencreport_hline_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_hline_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_hline_ce = zend_register_internal_class(&ce);
	opencreport_hline_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_hline_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_hline_ce->serialize = zend_class_serialize_deny;
	opencreport_hline_ce->unserialize = zend_class_unserialize_deny;
#endif

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
