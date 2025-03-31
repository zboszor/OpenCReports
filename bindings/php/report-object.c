/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_report_object_handlers;

zend_class_entry *opencreport_report_ce;

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
	zend_string *ignoreexpr = NULL;
	zend_string *reset_on_break_name = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 5)
		Z_PARAM_LONG(variable_type);
		Z_PARAM_STR(name);
		Z_PARAM_STR(expr);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(ignoreexpr, 1, 0);
		Z_PARAM_STR_EX(reset_on_break_name, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	long variable_type;
	char *name;
	char *expr;
	char *ignoreexpr = NULL;
	char *reset_on_break_name = NULL;
	int name_len, expr_len, ignoreexpr_len, reset_on_break_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lss|s!s!", &variable_type, &name, &name_len, &expr, &expr_len, &ignoreexpr, &ignoreexpr_len, &reset_on_break_name, &reset_on_break_name_len)== FAILURE)
		return;
#endif

	ocrpt_var *v = ocrpt_variable_new(pro->r, variable_type, ZSTR_VAL(name), ZSTR_VAL(expr), ignoreexpr ? ZSTR_VAL(ignoreexpr) : NULL, reset_on_break_name ? ZSTR_VAL(reset_on_break_name) : NULL);
	if (!v)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_variable_ce);
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(return_value);
	vo->v = v;
	vo->r = pro->r;
	vo->is_iterator = false;
	vo->iter = NULL;
}

PHP_METHOD(opencreport_report, variable_new_full) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_long result_type;
	zend_string *name;
	zend_string *baseexpr = NULL;
	zend_string *ignoreexpr = NULL;
	zend_string *intermedexpr = NULL;
	zend_string *intermed2expr = NULL;
	zend_string *resultexpr = NULL;
	zend_string *reset_on_break_name = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 8)
		Z_PARAM_LONG(result_type);
		Z_PARAM_STR(name);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(baseexpr, 1, 0);
		Z_PARAM_STR_EX(ignoreexpr, 1, 0);
		Z_PARAM_STR_EX(intermedexpr, 1, 0);
		Z_PARAM_STR_EX(intermed2expr, 1, 0);
		Z_PARAM_STR_EX(resultexpr, 1, 0);
		Z_PARAM_STR_EX(reset_on_break_name, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	long result_type;
	char *name;
	char *baseexpr = NULL;
	char *ignoreexpr = NULL;
	char *intermedexpr = NULL;
	char *intermed2expr = NULL;
	char *resultexpr = NULL;
	char *reset_on_break_name = NULL;
	int name_len, baseexpr_len, ignoreexpr_len, intermedexpr_len, intermed2expr_len;
	int resultexpr_len, reset_on_break_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls|s!s!s!s!s!s!", &result_type, &name, &name_len, &baseexpr, &baseexpr_len, &ignoreexpr, &ignoreexpr_len, &intermedexpr, &intermedexpr_len, &intermed2expr, &intermed2expr_len, &resultexpr, &resultexpr_len, &reset_on_break_name, &reset_on_break_name_len) == FAILURE)
		return;
#endif

	ocrpt_var *v = ocrpt_variable_new_full(pro->r, result_type, ZSTR_VAL(name),
												baseexpr ? ZSTR_VAL(baseexpr) : NULL,
												ignoreexpr ? ZSTR_VAL(ignoreexpr) : NULL,
												intermedexpr ? ZSTR_VAL(intermedexpr) : NULL,
												intermed2expr ? ZSTR_VAL(intermed2expr) : NULL,
												resultexpr ? ZSTR_VAL(resultexpr) : NULL,
												reset_on_break_name ? ZSTR_VAL(reset_on_break_name) : NULL);

	if (!v)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_variable_ce);
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(return_value);
	vo->v = v;
	vo->r = pro->r;
	vo->is_iterator = false;
	vo->iter = NULL;
}

PHP_METHOD(opencreport_report, variable_get_first) {
	zval *object = getThis();
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_var *v = ocrpt_variable_get_next(pro->r, &iter);

	if (!v)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_variable_ce);
	php_opencreport_variable_object *vo = Z_OPENCREPORT_VARIABLE_P(return_value);
	vo->v = v;
	vo->r = pro->r;
	vo->is_iterator = true;
	vo->iter = iter;
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

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_variable_new, 0, 3, OpenCReport\\Variable, 1)
ZEND_ARG_TYPE_INFO(0, variable_type, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, expr, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, ignoreexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, reset_on_break_name, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_variable_new_full, 0, 2, OpenCReport\\Variable, 1)
ZEND_ARG_TYPE_INFO(0, result_type, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, baseexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, ignoreexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, intermedexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, intermed2expr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, resultexpr, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, reset_on_break_name, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_report_variable_get_first, 0, 0, OpenCReport\\Variable, 1)
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
#define arginfo_opencreport_report_variable_new NULL
#define arginfo_opencreport_report_variable_new_full NULL
#define arginfo_opencreport_report_variable_get_first NULL
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
	PHP_ME(opencreport_report, variable_new, arginfo_opencreport_report_variable_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, variable_new_full, arginfo_opencreport_report_variable_new_full, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_report, variable_get_first, arginfo_opencreport_report_variable_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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

void register_opencreport_report_ce(void) {
	zend_class_entry ce;

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

}
