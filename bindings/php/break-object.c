/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_break_object_handlers;

zend_class_entry *opencreport_break_ce;

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

void register_opencreport_break_ce(void) {
	zend_class_entry ce;

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
}
