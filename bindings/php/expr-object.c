/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_expr_object_handlers;

zend_class_entry *opencreport_expr_ce;

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

PHP_METHOD(opencreport_expr, free) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr_free(eo->e);
	eo->e = NULL;
}

PHP_METHOD(opencreport_expr, get_expr_string) {
	zval *object = getThis();
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_STRING(ocrpt_expr_get_expr_string(eo->e));
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

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_get_expr_string, 0, 0, IS_STRING, 0)
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
#define arginfo_opencreport_expr_get_expr_string NULL
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
	PHP_ME(opencreport_expr, get_expr_string, arginfo_opencreport_expr_get_expr_string, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
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

void register_opencreport_expr_ce(void) {
	zend_class_entry ce;

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
}
