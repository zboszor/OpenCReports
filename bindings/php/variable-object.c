/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_variable_object_handlers;

zend_class_entry *opencreport_variable_ce;

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

PHP_METHOD(opencreport_variable, ignoreexpr) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_ignoreexpr(vo->v);

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

PHP_METHOD(opencreport_variable, get_type) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_variable_get_type(vo->v));
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

PHP_METHOD(opencreport_variable, get_precalculate) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_variable_get_precalculate(vo->v);
	if (!e)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
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

PHP_METHOD(opencreport_variable, get_next) {
	zval *object = getThis();
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	if (!vo->is_iterator)
		RETURN_NULL();

	ocrpt_report *r = vo->r;
	ocrpt_list *iter = vo->iter;
	ocrpt_var *v = ocrpt_variable_get_next(r, &iter);
	if (!v)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_variable_ce);
	vo = Z_OPENCREPORT_VARIABLE_P(return_value);
	vo->v = v;
	vo->r = r;
	vo->is_iterator = true;
	vo->iter = iter;
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_baseexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_ignoreexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_intermedexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_intermed2expr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_resultexpr, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_get_type, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_set_precalculate, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_get_precalculate, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_resolve, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_variable_eval, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_variable_get_next, 0, 0, OpenCReport\\Variable, 1)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_variable_baseexpr NULL
#define arginfo_opencreport_variable_ignoreexpr NULL
#define arginfo_opencreport_variable_intermedexpr NULL
#define arginfo_opencreport_variable_intermed2expr NULL
#define arginfo_opencreport_variable_resultexpr NULL
#define arginfo_opencreport_variable_get_type NULL
#define arginfo_opencreport_variable_set_precalculate NULL
#define arginfo_opencreport_variable_get_precalculate NULL
#define arginfo_opencreport_variable_resolve NULL
#define arginfo_opencreport_variable_eval NULL
#define arginfo_opencreport_variable_get_next NULL

#endif

static const zend_function_entry opencreport_variable_class_methods[] = {
	PHP_ME(opencreport_variable, baseexpr, arginfo_opencreport_variable_baseexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, ignoreexpr, arginfo_opencreport_variable_ignoreexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, intermedexpr, arginfo_opencreport_variable_intermedexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, intermed2expr, arginfo_opencreport_variable_intermed2expr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, resultexpr, arginfo_opencreport_variable_resultexpr, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, get_type, arginfo_opencreport_variable_get_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, set_precalculate, arginfo_opencreport_variable_set_precalculate, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, get_precalculate, arginfo_opencreport_variable_get_precalculate, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, resolve, arginfo_opencreport_variable_resolve, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, eval, arginfo_opencreport_variable_eval, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_variable, get_next, arginfo_opencreport_variable_get_next, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

void register_opencreport_variable_ce(void) {
	zend_class_entry ce;

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
}
