/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_part_object_handlers;

zend_class_entry *opencreport_part_ce;

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

void register_opencreport_part_ce(void) {
	zend_class_entry ce;

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

}
