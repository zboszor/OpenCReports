/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_col_object_handlers;

zend_class_entry *opencreport_col_ce;

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

PHP_METHOD(opencreport_col, get_next) {
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

PHP_METHOD(opencreport_col, report_new) {
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

PHP_METHOD(opencreport_col, report_get_first) {
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

PHP_METHOD(opencreport_col, set_suppress) {
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

PHP_METHOD(opencreport_col, get_suppress) {
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

PHP_METHOD(opencreport_col, set_width) {
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

PHP_METHOD(opencreport_col, get_width) {
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

PHP_METHOD(opencreport_col, set_height) {
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

PHP_METHOD(opencreport_col, get_height) {
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

PHP_METHOD(opencreport_col, set_border_width) {
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

PHP_METHOD(opencreport_col, get_border_width) {
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

PHP_METHOD(opencreport_col, set_border_color) {
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

PHP_METHOD(opencreport_col, get_border_color) {
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

PHP_METHOD(opencreport_col, set_detail_columns) {
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

PHP_METHOD(opencreport_col, get_detail_columns) {
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

PHP_METHOD(opencreport_col, set_column_padding) {
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

PHP_METHOD(opencreport_col, get_column_padding) {
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

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_get_next, 0, 0, OpenCReport\\Column, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_report_new, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_report_get_first, 0, 0, OpenCReport\\Report, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_set_suppress, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_get_suppress, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_set_width, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_get_width, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_set_height, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_get_height, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_set_border_width, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_get_border_width, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_set_border_color, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_get_border_color, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_set_detail_columns, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_get_detail_columns, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_set_column_padding, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_col_get_column_padding, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_col_get_next NULL
#define arginfo_opencreport_col_report_new NULL
#define arginfo_opencreport_col_report_get_first NULL
#define arginfo_opencreport_col_set_suppress NULL
#define arginfo_opencreport_col_get_suppress NULL
#define arginfo_opencreport_col_set_width NULL
#define arginfo_opencreport_col_get_width NULL
#define arginfo_opencreport_col_set_height NULL
#define arginfo_opencreport_col_get_height NULL
#define arginfo_opencreport_col_set_border_width NULL
#define arginfo_opencreport_col_get_border_width NULL
#define arginfo_opencreport_col_set_border_color NULL
#define arginfo_opencreport_col_get_border_color NULL
#define arginfo_opencreport_col_set_detail_columns NULL
#define arginfo_opencreport_col_get_detail_columns NULL
#define arginfo_opencreport_col_set_column_padding NULL
#define arginfo_opencreport_col_get_column_padding NULL

#endif

static const zend_function_entry opencreport_col_class_methods[] = {
	PHP_ME(opencreport_col, get_next, arginfo_opencreport_col_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, report_new, arginfo_opencreport_col_report_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, report_get_first, arginfo_opencreport_col_report_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, set_suppress, arginfo_opencreport_col_set_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, get_suppress, arginfo_opencreport_col_get_suppress, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, set_width, arginfo_opencreport_col_set_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, get_width, arginfo_opencreport_col_get_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, set_height, arginfo_opencreport_col_set_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, get_height, arginfo_opencreport_col_get_height, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, set_border_width, arginfo_opencreport_col_set_border_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, get_border_width, arginfo_opencreport_col_get_border_width, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, set_border_color, arginfo_opencreport_col_set_border_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, get_border_color, arginfo_opencreport_col_get_border_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, set_detail_columns, arginfo_opencreport_col_set_detail_columns, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, get_detail_columns, arginfo_opencreport_col_get_detail_columns, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, set_column_padding, arginfo_opencreport_col_set_column_padding, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_col, get_column_padding, arginfo_opencreport_col_get_column_padding, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

void register_opencreport_col_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_col_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Column", opencreport_col_class_methods);
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
}
