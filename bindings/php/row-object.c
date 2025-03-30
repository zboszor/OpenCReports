/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_row_object_handlers;

zend_class_entry *opencreport_row_ce;

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

void register_opencreport_row_ce(void) {
	zend_class_entry ce;

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

}
