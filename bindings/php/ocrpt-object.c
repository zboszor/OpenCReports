/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

/* {{{ REGISTER_OPENCREPORT_CLASS_CONST_LONG */

/*
 * The below macro is open-coding zend_declare_class_constant_long()
 * with some changes. Since PHP 8.1, class constants can be final.
 * Use the ZEND_ACC_FINAL flag to disallow overriding our constants.
 */
#if PHP_VERSION_ID >= 80100
# define MY_FINAL_CONST (ZEND_ACC_FINAL)
#else
# define MY_FINAL_CONST (0)
#endif

#if PHP_VERSION_ID < 70100

#define REGISTER_OPENCREPORT_CLASS_CONST_LONG(const_name, value) \
	zend_declare_class_constant_long(opencreport_ce, const_name, sizeof(const_name)-1, value);

#else

#define REGISTER_OPENCREPORT_CLASS_CONST_LONG(const_name, value) { \
		zval constant; \
		ZVAL_LONG(&constant, (zend_long)value); \
		zend_string *key; \
		if (opencreport_ce->type == ZEND_INTERNAL_CLASS) { \
			key = zend_string_init_interned(const_name, sizeof(const_name)-1, 1); \
		} else { \
			key = zend_string_init(const_name, sizeof(const_name)-1, 0); \
		} \
		zend_declare_class_constant_ex(opencreport_ce, key, &constant, ZEND_ACC_PUBLIC | MY_FINAL_CONST, NULL); \
		zend_string_release(key); \
	}
#endif

/* }}} */

static zend_object_handlers opencreport_object_handlers;

zend_class_entry *opencreport_ce;

static void opencreport_object_free(zend_object *object TSRMLS_DC);

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_object_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
#else
static zend_object_value opencreport_object_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_object *intern = zend_object_alloc(sizeof(php_opencreport_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) opencreport_object_free, NULL TSRMLS_CC);
	retval.handlers = &opencreport_object_handlers;
#else
	intern->zo.handlers = &opencreport_object_handlers;
#endif

	intern->o = NULL;
	intern->expr_error = NULL;
	intern->free_me = false;

#if PHP_VERSION_ID < 70000
	return retval;
#else
	return &intern->zo;
#endif
}
/* }}} */

void opencreport_object_deinit(php_opencreport_object *oo) {
	ocrpt_strfree(oo->expr_error);
	oo->expr_error = NULL;
	if (oo->free_me)
		ocrpt_free(oo->o);
	oo->o = NULL;
}

static void opencreport_object_free(zend_object *object TSRMLS_DC) /* {{{ */
{
	php_opencreport_object *oo = php_opencreport_from_obj(object);

	if (!oo->o)
		return;

	opencreport_object_deinit(oo);

	zend_object_std_dtor(&oo->zo TSRMLS_CC);
#if PHP_VERSION_ID < 70000
	efree(oo);
#endif
}
/* }}} */

PHP_METHOD(opencreport, __construct) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	oo->o = ocrpt_init();
	oo->free_me = true;
}

PHP_METHOD(opencreport, parse_xml) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *filename;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(filename);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *filename;
	int filename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE)
		return;
#endif

	RETURN_BOOL(ocrpt_parse_xml(oo->o, ZSTR_VAL(filename)));
}

PHP_METHOD(opencreport, parse_xml_from_buffer) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *buffer;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(buffer);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *buffer;
	int buffer_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &buffer, &buffer_len) == FAILURE)
		return;
#endif

	RETURN_BOOL(ocrpt_parse_xml_from_buffer(oo->o, ZSTR_VAL(buffer), ZSTR_LEN(buffer)));
}

PHP_METHOD(opencreport, set_output_format) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_long format;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(format);
	ZEND_PARSE_PARAMETERS_END();
#else
	long format;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &format) == FAILURE)
		return;
#endif

	ocrpt_set_output_format(oo->o, format);
}

PHP_METHOD(opencreport, get_output_format) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_get_output_format(oo->o));
}

PHP_METHOD(opencreport, get_output_format_name) {
#if PHP_VERSION_ID >= 70000
	zend_long format;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_LONG(format);
	ZEND_PARSE_PARAMETERS_END();
#else
	long format;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &format) == FAILURE)
		return;
#endif

	RETURN_STRING(ocrpt_get_output_format_name(format));
}

PHP_METHOD(opencreport, set_output_parameter) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *param;
	zend_string *value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_STR(param);
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *param, *value;
	int param_len, value_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &param, &param_len, &value, &value_len) == FAILURE)
		return;
#endif

	ocrpt_set_output_parameter(oo->o, ZSTR_VAL(param), ZSTR_VAL(value));
}

PHP_METHOD(opencreport, execute) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_execute(oo->o));
}

PHP_METHOD(opencreport, spool) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_spool(oo->o);
}

PHP_METHOD(opencreport, get_output) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	size_t sz = 0;
	const char *res = ocrpt_get_output(oo->o, &sz);

	if (!res)
		RETURN_FALSE;

	OCRPT_RETURN_STRINGL(res, sz);
}

PHP_METHOD(opencreport, get_content_type) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	const ocrpt_string **content_type = ocrpt_get_content_type(oo->o);

	if (!content_type)
		RETURN_FALSE;

	array_init(return_value);

	for (int32_t i = 0; content_type[i]; i++) {
#if PHP_VERSION_ID >= 70000
		zval tmp;
		ZVAL_STRINGL(&tmp, content_type[i]->str, content_type[i]->len);
		zend_hash_next_index_insert_new(Z_ARRVAL_P(return_value), &tmp);
#else
		zval *tmp;
		MAKE_STD_ZVAL(tmp);
		ZVAL_STRINGL(tmp, content_type[i]->str, content_type[i]->len, 1);
		zend_hash_next_index_insert(Z_ARRVAL_P(return_value), tmp, sizeof(zval *), NULL);
#endif
	}
}

PHP_METHOD(opencreport, version) {
	ZEND_PARSE_PARAMETERS_NONE();

	OCRPT_RETURN_STRING(ocrpt_version());
}

PHP_METHOD(opencreport, set_numeric_precision_bits) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

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

	ocrpt_set_numeric_precision_bits(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport, get_numeric_precision_bits) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(ocrpt_get_numeric_precision_bits(oo->o));
}

PHP_METHOD(opencreport, set_rounding_mode) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

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

	ocrpt_set_rounding_mode(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
}

PHP_METHOD(opencreport, bindtextdomain) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *domainname;
	zend_string *dirname;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_STR(domainname);
		Z_PARAM_STR(dirname);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *domainname, *dirname;
	int domainname_len, dirname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &domainname, &domainname_len, &dirname, &dirname_len) == FAILURE)
		return;
#endif

	ocrpt_bindtextdomain(oo->o, ZSTR_VAL(domainname), ZSTR_VAL(dirname));
}

PHP_METHOD(opencreport, set_locale) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *locale;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(locale);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *locale;
	int locale_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &locale, &locale_len) == FAILURE)
		return;
#endif

	ocrpt_set_locale(oo->o, ZSTR_VAL(locale));
}

PHP_METHOD(opencreport, datasource_add) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	HashTable *source_params = NULL;
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;
	ocrpt_input_connect_parameter *conn_params = NULL;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *source_name;
	zend_string *source_type;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(source_name);
		Z_PARAM_STR(source_type);
		Z_PARAM_OPTIONAL;
		Z_PARAM_ARRAY_HT_OR_NULL(source_params);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name, *source_type;
	int source_name_len, source_type_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|h!", &source_name, &source_name_len, &source_type, &source_type_len, &source_params) == FAILURE)
		return;
#endif

	if (source_params) {
		uint32_t n_elements = zend_hash_num_elements(source_params);

		if (n_elements) {
			HashPosition source_param_pos;
			uint32_t n_params = 0, pos = 0;

			conn_params = ocrpt_mem_malloc((n_elements + 1) * sizeof(ocrpt_input_connect_parameter));
			memset(conn_params, 0, (n_elements + 1) * sizeof(ocrpt_input_connect_parameter));

			for (zend_hash_internal_pointer_reset_ex(source_params, &source_param_pos); pos < n_elements; pos++, zend_hash_move_forward_ex(source_params, &source_param_pos)) {
#if PHP_VERSION_ID >= 70000
				zval key;

				zend_hash_get_current_key_zval_ex(source_params, &key, &source_param_pos);
				if (Z_TYPE(key) != IS_STRING)
					continue;
#else
				char *key;
				uint key_len;
				ulong num_index;

				zend_hash_get_current_key_ex(source_params, &key, &key_len, &num_index, 0, &source_param_pos);
#endif

				zval *value = ocrpt_hash_get_current_data_ex(source_params, &source_param_pos);
				if (Z_TYPE_P(value) != IS_STRING)
					continue;

#if PHP_VERSION_ID >= 70000
				conn_params[n_params].param_name = Z_STRVAL(key);
#else
				conn_params[n_params].param_name = key;
#endif
				conn_params[n_params].param_value = Z_STRVAL_P(value);
				n_params++;
			}
		}
	}

	ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), ZSTR_VAL(source_type), conn_params);

	ocrpt_mem_free(conn_params);

	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, datasource_get) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	ocrpt_datasource *ds;
	php_opencreport_ds_object *dso;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *source_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name;
	int source_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &source_name, &source_name_len) == FAILURE)
		return;
#endif

	ds = ocrpt_datasource_get(oo->o, ZSTR_VAL(source_name));
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

PHP_METHOD(opencreport, query_get) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	ocrpt_query *q;
	php_opencreport_query_object *qo;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

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

	q = ocrpt_query_get(oo->o, ZSTR_VAL(query_name));
	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;
}

PHP_METHOD(opencreport, query_refresh) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_query_refresh(oo->o);
}

PHP_METHOD(opencreport, expr_parse) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	char *err = NULL;
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

	ocrpt_expr *e = ocrpt_expr_parse(oo->o, ZSTR_VAL(expr_string), &err);

	ocrpt_strfree(oo->expr_error);
	if (e) {
		object_init_ex(return_value, opencreport_expr_ce);
		php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
		eo->e = e;
		oo->expr_error = NULL;
	} else {
		oo->expr_error = err;
		RETURN_NULL();
	}
}

PHP_METHOD(opencreport, expr_error) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	if (oo->expr_error) {
		OCRPT_RETURN_STRING(oo->expr_error);
	} else
		RETURN_NULL();
}

OCRPT_STATIC_FUNCTION(opencreport_default_function) {
	ocrpt_string *fname = user_data;

	ocrpt_expr_make_error_result(e, "not implemented");

#if PHP_VERSION_ID >= 70000
	zval zfname;
	zval retval;
	zval params[1];

	ZVAL_STRINGL(&zfname, fname->str, fname->len);

	object_init_ex(&params[0], opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(&params[0]);
#else
	zval *zfname;
	zval *retval;
	zval *params0[1];
	zval ***params = emalloc(sizeof(zval **));

	MAKE_STD_ZVAL(zfname);
	ZVAL_STRINGL(zfname, fname->str, fname->len, 1);

	ALLOC_INIT_ZVAL(params0[0]);
	object_init_ex(params0[0], opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(params0[0]);

	params[0] = &params0[0];
#endif

	eo->e = e;

#if PHP_VERSION_ID >= 80000
	bool failed = (call_user_function(CG(function_table), NULL, &zfname, &retval, 1, params) == FAILURE);
#elif PHP_VERSION_ID >= 70000
	bool failed = (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 1, params, 0, NULL TSRMLS_CC) == FAILURE);
#else
	bool failed = (call_user_function_ex(CG(function_table), NULL, zfname, &retval, 1, params, 0, NULL TSRMLS_CC) == FAILURE);
#endif

#if PHP_VERSION_ID >= 70000
	zend_object_std_dtor(&eo->zo);
	OBJ_RELEASE(&eo->zo);

	zend_string_release(Z_STR(zfname));
#else
	zval_dtor(zfname);
	efree(zfname);
	efree(params);
	zval_dtor(params0[0]);
	efree(params0[0]);
#endif

	/*
	 * The function must be a real OpenCReports user function
	 * and called $e->set_{long|double|string}_value(...)
	 */
#if PHP_VERSION_ID >= 70000
	if (failed)
		ocrpt_expr_make_error_result(e, "invalid function call");
	else if (Z_TYPE(retval) == IS_UNDEF || Z_TYPE(retval) == IS_NULL)
		return;
	else if (Z_TYPE(retval) == IS_FALSE)
		ocrpt_expr_set_long(e, 0);
	else if (Z_TYPE(retval) == IS_TRUE)
		ocrpt_expr_set_long(e, 1);
	else if (Z_TYPE(retval) == IS_LONG)
		ocrpt_expr_set_long(e, Z_LVAL(retval));
	else if (Z_TYPE(retval) == IS_DOUBLE)
		ocrpt_expr_set_double(e, Z_DVAL(retval));
	else if (Z_TYPE(retval) == IS_STRING)
		ocrpt_expr_set_string(e, Z_STRVAL(retval));
	else
		ocrpt_expr_make_error_result(e, "invalid return value");
#else
	if (failed)
		ocrpt_expr_make_error_result(e, "invalid function call");
	else if (Z_TYPE_P(retval) == IS_NULL)
		/* Do nothing */;
	else if (Z_TYPE_P(retval) == IS_BOOL)
		ocrpt_expr_set_long(e, Z_BVAL_P(retval));
	else if (Z_TYPE_P(retval) == IS_LONG)
		ocrpt_expr_set_long(e, Z_LVAL_P(retval));
	else if (Z_TYPE_P(retval) == IS_DOUBLE)
		ocrpt_expr_set_double(e, Z_DVAL_P(retval));
	else if (Z_TYPE_P(retval) == IS_STRING)
		ocrpt_expr_set_string(e, Z_STRVAL_P(retval));
	else
		ocrpt_expr_make_error_result(e, "invalid return value");

	if (retval) {
		zval_dtor(retval);
		efree(retval);
	}
#endif
}

PHP_METHOD(opencreport, part_new) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_part *p = ocrpt_part_new(oo->o);
	if (!p)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_part_ce);
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(return_value);
	po->o = NULL;
	po->p = p;
	po->iter = NULL;
	po->is_iterator = false;
}

PHP_METHOD(opencreport, part_get_first) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_list *iter = NULL;
	ocrpt_part *p = ocrpt_part_get_next(oo->o, &iter);

	if (!p)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_part_ce);
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(return_value);
	po->o = oo->o;
	po->p = p;
	po->iter = iter;
	po->is_iterator = true;
}

PHP_METHOD(opencreport, function_add) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_bool commutative, associative, left_associative, dont_optimize;

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *expr_func_name;
	zend_string *zend_func_name;
	zend_long n_ops;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 7, 7)
		Z_PARAM_STR(expr_func_name);
		Z_PARAM_STR(zend_func_name);
		Z_PARAM_LONG(n_ops);
		Z_PARAM_BOOL(commutative);
		Z_PARAM_BOOL(associative);
		Z_PARAM_BOOL(left_associative);
		Z_PARAM_BOOL(dont_optimize);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *expr_func_name, *zend_func_name;
	int expr_func_name_len, zend_func_name_len;
	long n_ops;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sslbbbb", &expr_func_name, &expr_func_name_len, &zend_func_name, &zend_func_name_len, &n_ops, &commutative, &associative, &left_associative, &dont_optimize) == FAILURE)
		return;
#endif

	ocrpt_string *ofunc = ocrpt_mem_string_new_with_len(ZSTR_VAL(zend_func_name), ZSTR_LEN(zend_func_name));
	bool ret = ocrpt_function_add(oo->o, ZSTR_VAL(expr_func_name), opencreport_default_function, ofunc, n_ops, commutative, associative, left_associative, dont_optimize);

	RETURN_BOOL(ret);
}

static void opencreport_cb(opencreport *o, void *data) {
	char *callback = data;

#if PHP_VERSION_ID >= 70000
	zval zfname;
	ZVAL_STRING(&zfname, callback);

	zval retval;
	zval params[1];

	object_init_ex(&params[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(&params[0]);
	oo->o = o;
#else
	zval *zfname;
	MAKE_STD_ZVAL(zfname);
	ZVAL_STRING(zfname, callback, 1);

	zval *retval = NULL;
	zval *params0[1];
	zval ***params = emalloc(sizeof(zval **));

	ALLOC_INIT_ZVAL(params0[0]);
	object_init_ex(params0[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(params0[0]);
	oo->o = o;

	params[0] = &params0[0];
#endif

#if PHP_VERSION_ID >= 80000
	call_user_function(CG(function_table), NULL, &zfname, &retval, 1, params);
#elif PHP_VERSION_ID >= 70000
	call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 1, params, 0, NULL TSRMLS_CC);
#else
	call_user_function_ex(CG(function_table), NULL, zfname, &retval, 1, params, 0, NULL TSRMLS_CC);
#endif

#if PHP_VERSION_ID >= 70000
	zend_object_std_dtor(&oo->zo);
	OBJ_RELEASE(&oo->zo);

	zend_string_release(Z_STR(zfname));
#else
	zval_dtor(zfname);
	efree(zfname);
	efree(params);
	zend_object_std_dtor(&oo->zo);
	if (Z_REFCOUNT_P(params0[0]) == 1) {
		zval_dtor(params0[0]);
		efree(params0[0]);
	}
	if (retval) {
		zval_dtor(retval);
		efree(retval);
	}
#endif
}

void opencreport_part_cb(opencreport *o, ocrpt_part *p, void *data) {
	char *callback = data;

#if PHP_VERSION_ID >= 70000
	zval zfname;
	ZVAL_STRING(&zfname, callback);

	zval retval;
	zval params[2];

	object_init_ex(&params[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(&params[0]);
	oo->o = o;

	object_init_ex(&params[1], opencreport_part_ce);
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(&params[1]);
	po->p = p;
#else
	zval *zfname;
	MAKE_STD_ZVAL(zfname);
	ZVAL_STRING(zfname, callback, 1);

	zval *retval = NULL;
	zval *params0[2];
	zval ***params = emalloc(sizeof(zval **) * 2);

	ALLOC_INIT_ZVAL(params0[0]);
	object_init_ex(params0[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(params0[0]);
	oo->o = o;

	ALLOC_INIT_ZVAL(params0[1]);
	object_init_ex(params0[1], opencreport_part_ce);
	php_opencreport_part_object *po = Z_OPENCREPORT_PART_P(params0[1]);
	po->p = p;

	for (int32_t i = 0; i < 2; i++)
		params[i] = &params0[i];
#endif

#if PHP_VERSION_ID >= 80000
	call_user_function(CG(function_table), NULL, &zfname, &retval, 2, params);
#elif PHP_VERSION_ID >= 70000
	call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 2, params, 0, NULL TSRMLS_CC);
#else
	call_user_function_ex(CG(function_table), NULL, zfname, &retval, 2, params, 0, NULL TSRMLS_CC);
#endif

#if PHP_VERSION_ID >= 70000
	zend_object_std_dtor(&oo->zo);
	OBJ_RELEASE(&oo->zo);
	zend_object_std_dtor(&po->zo);
	OBJ_RELEASE(&po->zo);

	zend_string_release(Z_STR(zfname));
#else
	zval_dtor(zfname);
	efree(zfname);
	efree(params);
	zend_object_std_dtor(&oo->zo);
	zend_object_std_dtor(&po->zo);
	for (int32_t i = 0; i < 2; i++) {
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

PHP_METHOD(opencreport, add_precalculation_done_cb) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
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

	ocrpt_add_precalculation_done_cb(oo->o, opencreport_cb, cb_name);
}

PHP_METHOD(opencreport, add_part_added_cb) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
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

	ocrpt_add_part_added_cb(oo->o, opencreport_part_cb, cb_name);
}

void opencreport_report_cb(opencreport *o, ocrpt_report *r, void *data) {
	char *callback = data;

#if PHP_VERSION_ID >= 70000
	zval zfname;
	ZVAL_STRING(&zfname, callback);

	zval retval;
	zval params[2];

	object_init_ex(&params[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(&params[0]);
	oo->o = o;

	object_init_ex(&params[1], opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(&params[1]);
	pro->r = r;
#else
	zval *zfname;
	MAKE_STD_ZVAL(zfname);
	ZVAL_STRING(zfname, callback, 1);

	zval *retval = NULL;
	zval *params0[2];
	zval ***params = emalloc(sizeof(zval **) * 2);

	ALLOC_INIT_ZVAL(params0[0]);
	object_init_ex(params0[0], opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(params0[0]);
	oo->o = o;

	ALLOC_INIT_ZVAL(params0[1]);
	object_init_ex(params0[1], opencreport_report_ce);
	php_opencreport_report_object *pro = Z_OPENCREPORT_REPORT_P(params0[1]);
	pro->r = r;

	for (int32_t i = 0; i < 2; i++)
		params[i] = &params0[i];
#endif

#if PHP_VERSION_ID >= 80000
	call_user_function(CG(function_table), NULL, &zfname, &retval, 2, params);
#elif PHP_VERSION_ID >= 70000
	call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 2, params, 0, NULL TSRMLS_CC);
#else
	call_user_function_ex(CG(function_table), NULL, zfname, &retval, 2, params, 0, NULL TSRMLS_CC);
#endif

#if PHP_VERSION_ID >= 70000
	zend_object_std_dtor(&oo->zo);
	OBJ_RELEASE(&oo->zo);
	zend_object_std_dtor(&pro->zo);
	OBJ_RELEASE(&pro->zo);

	zend_string_release(Z_STR(zfname));
#else
	zval_dtor(zfname);
	efree(zfname);
	efree(params);
	zend_object_std_dtor(&oo->zo);
	zend_object_std_dtor(&pro->zo);
	for (int32_t i = 0; i < 2; i++) {
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

PHP_METHOD(opencreport, add_report_added_cb) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
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

	ocrpt_add_report_added_cb(oo->o, opencreport_report_cb, cb_name);
}

PHP_METHOD(opencreport, env_get) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *var_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(var_name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *var_name;
	int var_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &var_name, &var_name_len) == FAILURE)
		return;
#endif

	ocrpt_result *r = ocrpt_env_get(oo->o, ZSTR_VAL(var_name));
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->o = oo->o;
	ro->r = r;
	ro->freed_by_lib = false;
}

PHP_METHOD(opencreport, result_new) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_result *r = ocrpt_result_new(oo->o);
	if (!r)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_result_ce);
	php_opencreport_result_object *ro = Z_OPENCREPORT_RESULT_P(return_value);
	ro->o = oo->o;
	ro->r = r;
	ro->freed_by_lib = false;
}

PHP_METHOD(opencreport, set_mvariable) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *name;
	zend_string *value = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 2)
		Z_PARAM_STR(name);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(value, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *name, *value = NULL;
	int name_len, value_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s!", &name, &name_len, &value, &value_len) == FAILURE)
		return;
#endif

	ocrpt_set_mvariable(oo->o, ZSTR_VAL(name), value ? ZSTR_VAL(value) : NULL);
}

PHP_METHOD(opencreport, add_search_path) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	zend_string *path;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(path);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *path;
	int path_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE)
		return;
#endif

	ocrpt_add_search_path(oo->o, ZSTR_VAL(path));
}

PHP_METHOD(opencreport, canonicalize_path) {
#if PHP_VERSION_ID >= 70000
	zend_string *path;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(path);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *path;
	int path_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE)
		return;
#endif

	char *cpath = ocrpt_canonicalize_path(ZSTR_VAL(path));
	OCRPT_RETVAL_STRING(cpath);
	ocrpt_mem_free(cpath);
}

PHP_METHOD(opencreport, find_file) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

#if PHP_VERSION_ID >= 70000
	zend_string *path;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(path);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *path;
	int path_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE)
		return;
#endif

	char *file = ocrpt_find_file(oo->o, ZSTR_VAL(path));

	if (!file)
		RETURN_NULL();

	OCRPT_RETVAL_STRING(file);
	ocrpt_mem_free(file);
}

PHP_METHOD(opencreport, get_color) {
	zend_bool bgcolor = false;
	ocrpt_color c;
#if PHP_VERSION_ID >= 70000
	zend_string *color_name = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 2)
		Z_PARAM_STR_EX(color_name, 1, 0);
		Z_PARAM_OPTIONAL;
		Z_PARAM_BOOL(bgcolor);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *color_name = NULL;
	int color_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!|b", &color_name, &color_name_len, &bgcolor) == FAILURE)
		return;
#endif

	ocrpt_get_color(color_name ? ZSTR_VAL(color_name) : NULL, &c, bgcolor);

	array_init(return_value);

#if PHP_VERSION_ID >= 70000
	zval tmp;

	ZVAL_DOUBLE(&tmp, c.r);
	zend_hash_str_add_new(Z_ARRVAL_P(return_value), "r", 1, &tmp);

	ZVAL_DOUBLE(&tmp, c.g);
	zend_hash_str_add_new(Z_ARRVAL_P(return_value), "g", 1, &tmp);

	ZVAL_DOUBLE(&tmp, c.b);
	zend_hash_str_add_new(Z_ARRVAL_P(return_value), "b", 1, &tmp);
#else
	zval *tmp;

	ALLOC_INIT_ZVAL(tmp);
	ZVAL_DOUBLE(tmp, c.r);
	zend_hash_add(Z_ARRVAL_P(return_value), "r", sizeof("r"), &tmp, sizeof(zval*), NULL);

	ALLOC_INIT_ZVAL(tmp);
	ZVAL_DOUBLE(tmp, c.g);
	zend_hash_add(Z_ARRVAL_P(return_value), "g", sizeof("g"), &tmp, sizeof(zval*), NULL);

	ALLOC_INIT_ZVAL(tmp);
	ZVAL_DOUBLE(tmp, c.b);
	zend_hash_add(Z_ARRVAL_P(return_value), "b", sizeof("b"), &tmp, sizeof(zval*), NULL);
#endif
}

PHP_METHOD(opencreport, set_paper) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *paper;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(paper);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *paper;
	int paper_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &paper, &paper_len) == FAILURE)
		return;
#endif

	ocrpt_set_paper_by_name(oo->o, ZSTR_VAL(paper));
}

PHP_METHOD(opencreport, set_size_unit) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
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

	ocrpt_expr *e = ocrpt_set_size_unit(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport, get_size_unit) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_get_size_unit(oo->o);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport, set_noquery_show_nodata) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
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

	ocrpt_expr *e = ocrpt_set_noquery_show_nodata(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport, get_noquery_show_nodata) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_get_noquery_show_nodata(oo->o);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport, set_report_height_after_last) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
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

	ocrpt_expr *e = ocrpt_set_report_height_after_last(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport, get_report_height_after_last) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_get_report_height_after_last(oo->o);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport, set_follower_match_single) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
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

	ocrpt_expr *e = ocrpt_set_follower_match_single(oo->o, expr_string ? ZSTR_VAL(expr_string) : NULL);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport, get_follower_match_single) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_expr *e = ocrpt_get_follower_match_single(oo->o);
	if (!e)
		RETURN_NULL();
	object_init_ex(return_value, opencreport_expr_ce);
	php_opencreport_expr_object *eo = Z_OPENCREPORT_EXPR_P(return_value);
	eo->e = e;
}

PHP_METHOD(opencreport, set_follower_match_single_direct) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	zend_bool value = false;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_BOOL(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &value) == FAILURE)
		return;
#endif

	ocrpt_set_follower_match_single_direct(oo->o, value);
}

PHP_METHOD(opencreport, get_follower_match_single_direct) {
	zval *object = getThis();
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_BOOL(ocrpt_get_follower_match_single_direct(oo->o));
}

#if PHP_VERSION_ID >= 70000

ZEND_BEGIN_ARG_INFO_EX(arginfo_opencreport___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_parse_xml, 0, 1, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_parse_xml_from_buffer, 0, 1, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, buffer, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_output_format, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, format, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_output_format, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_output_format_name, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_output_parameter, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, param, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_execute, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_spool, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_output, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_content_type, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_version, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_numeric_precision_bits, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_numeric_precision_bits, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_rounding_mode, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_bindtextdomain, 0, 2, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, domainname, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dirname, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_locale, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, locale, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_add, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, source_type, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, source_params, IS_ARRAY, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_datasource_get, 0, 1, OpenCReport\\Datasource, 1)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_query_get, 0, 1, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, query_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_array_query_refresh, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_expr_parse, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_expr_error, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_new, 0, 0, OpenCReport\\Part, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_part_get_first, 0, 0, OpenCReport\\Part, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_function_add, 0, 7, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, expr_func_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, zend_func_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, n_ops, IS_LONG, 0)
ZEND_ARG_TYPE_INFO(0, commutative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, associative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, left_associative, _IS_BOOL, 0)
ZEND_ARG_TYPE_INFO(0, dont_optimize, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_env_get, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, var_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_result_new, 0, 1, OpenCReport\\Result, 1)
ZEND_ARG_TYPE_INFO(0, var_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_mvariable, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, name, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_add_search_path, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_canonicalize_path, 0, 1, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_find_file, 0, 1, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_color, 0, 0, IS_ARRAY, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, color_name, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, bgcolor, _IS_BOOL, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_paper, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, paper, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_set_size_unit, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_get_size_unit, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_set_noquery_show_nodata, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_get_noquery_show_nodata, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_set_report_height_after_last, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_get_report_height_after_last, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_set_follower_match_single, 0, 1, OpenCReport\\Expr, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_get_follower_match_single, 0, 0, OpenCReport\\Expr, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_set_follower_match_single_direct, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, value, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_get_follower_match_single_direct, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport___construct NULL
#define arginfo_opencreport_parse_xml NULL
#define arginfo_opencreport_parse_xml_from_buffer NULL
#define arginfo_opencreport_set_output_format NULL
#define arginfo_opencreport_get_output_format NULL
#define arginfo_opencreport_get_output_format_name NULL
#define arginfo_opencreport_set_output_parameter NULL
#define arginfo_opencreport_execute NULL
#define arginfo_opencreport_spool NULL
#define arginfo_opencreport_get_output NULL
#define arginfo_opencreport_get_content_type NULL
#define arginfo_opencreport_version NULL
#define arginfo_opencreport_set_numeric_precision_bits NULL
#define arginfo_opencreport_get_numeric_precision_bits NULL
#define arginfo_opencreport_set_rounding_mode NULL
#define arginfo_opencreport_bindtextdomain NULL
#define arginfo_opencreport_set_locale NULL
#define arginfo_opencreport_datasource_add NULL
#define arginfo_opencreport_datasource_get NULL
#define arginfo_opencreport_query_get NULL
#define arginfo_array_query_refresh NULL
#define arginfo_opencreport_expr_parse NULL
#define arginfo_opencreport_expr_error NULL
#define arginfo_opencreport_part_new NULL
#define arginfo_opencreport_part_get_first NULL
#define arginfo_opencreport_function_add NULL
#define arginfo_opencreport_env_get NULL
#define arginfo_opencreport_result_new NULL
#define arginfo_opencreport_set_mvariable NULL
#define arginfo_opencreport_add_search_path NULL
#define arginfo_opencreport_canonicalize_path NULL
#define arginfo_opencreport_find_file NULL
#define arginfo_opencreport_get_color NULL
#define arginfo_opencreport_set_paper NULL
#define arginfo_opencreport_set_size_unit NULL
#define arginfo_opencreport_get_size_unit NULL
#define arginfo_opencreport_set_noquery_show_nodata NULL
#define arginfo_opencreport_get_noquery_show_nodata NULL
#define arginfo_opencreport_set_report_height_after_last NULL
#define arginfo_opencreport_get_report_height_after_last NULL
#define arginfo_opencreport_set_follower_match_single NULL
#define arginfo_opencreport_get_follower_match_single NULL
#define arginfo_opencreport_set_follower_match_single_direct NULL
#define arginfo_opencreport_get_follower_match_single_direct NULL

#endif

static const zend_function_entry opencreport_class_methods[] = {
	/*
	 * High level API
	 */
	PHP_ME(opencreport, __construct, arginfo_opencreport___construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, parse_xml, arginfo_opencreport_parse_xml, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, parse_xml_from_buffer, arginfo_opencreport_parse_xml_from_buffer, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_output_format, arginfo_opencreport_set_output_format, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_output_format, arginfo_opencreport_get_output_format, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_output_format_name, arginfo_opencreport_get_output_format_name, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	PHP_ME(opencreport, set_output_parameter, arginfo_opencreport_set_output_parameter, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, execute, arginfo_opencreport_execute, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, spool, arginfo_opencreport_spool, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_output, arginfo_opencreport_get_output, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_content_type, arginfo_opencreport_get_content_type, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, version, arginfo_opencreport_version, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	/*
	 * Low level API
	 */
	/* Numeric behavior related methods */
	PHP_ME(opencreport, set_numeric_precision_bits, arginfo_opencreport_set_numeric_precision_bits, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_numeric_precision_bits, arginfo_opencreport_get_numeric_precision_bits, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_rounding_mode, arginfo_opencreport_set_rounding_mode, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Locale related methods */
	PHP_ME(opencreport, bindtextdomain, arginfo_opencreport_bindtextdomain, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_locale, arginfo_opencreport_set_locale, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Datasource and query related methods */
	PHP_ME(opencreport, datasource_add, arginfo_opencreport_datasource_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, datasource_get, arginfo_opencreport_datasource_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, query_get, arginfo_opencreport_query_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, query_refresh, arginfo_array_query_refresh, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Expression related methods */
	PHP_ME(opencreport, expr_parse, arginfo_opencreport_expr_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, expr_error, arginfo_opencreport_expr_error, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Function related methods */
	PHP_ME(opencreport, function_add, arginfo_opencreport_function_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Report part related functions */
	PHP_ME(opencreport, part_new, arginfo_opencreport_part_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, part_get_first, arginfo_opencreport_part_get_first, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Callback related methods */
	PHP_ME(opencreport, add_precalculation_done_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, add_part_added_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, add_report_added_cb, arginfo_opencreport_add_any_cb, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Environment related methods */
	PHP_ME(opencreport, env_get, arginfo_opencreport_env_get, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_mvariable, arginfo_opencreport_set_mvariable, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Result related methods */
	PHP_ME(opencreport, result_new, arginfo_opencreport_result_new, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* File handling related methods */
	PHP_ME(opencreport, add_search_path, arginfo_opencreport_add_search_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, canonicalize_path, arginfo_opencreport_canonicalize_path, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	PHP_ME(opencreport, find_file, arginfo_opencreport_find_file, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Color related methods */
	PHP_ME(opencreport, get_color, arginfo_opencreport_get_color, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL | ZEND_ACC_STATIC)
	/* Paper related methods */
	PHP_ME(opencreport, set_paper, arginfo_opencreport_set_paper, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	/* Layout related methods */
	PHP_ME(opencreport, set_size_unit, arginfo_opencreport_set_size_unit, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_size_unit, arginfo_opencreport_get_size_unit, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_noquery_show_nodata, arginfo_opencreport_set_noquery_show_nodata, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_noquery_show_nodata, arginfo_opencreport_get_noquery_show_nodata, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_report_height_after_last, arginfo_opencreport_set_report_height_after_last, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_report_height_after_last, arginfo_opencreport_get_report_height_after_last, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_follower_match_single, arginfo_opencreport_set_follower_match_single, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_follower_match_single, arginfo_opencreport_get_follower_match_single, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, set_follower_match_single_direct, arginfo_opencreport_set_follower_match_single_direct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport, get_follower_match_single_direct, arginfo_opencreport_get_follower_match_single_direct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

void register_opencreport_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_CLASS_ENTRY(ce, "OpenCReport", opencreport_class_methods);
	ce.create_object = opencreport_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_object_handlers.offset = XtOffsetOf(php_opencreport_object, zo);
	opencreport_object_handlers.free_obj = opencreport_object_free;
#endif
	opencreport_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_ce = zend_register_internal_class(&ce);
#if PHP_VERSION_ID >= 80100
	opencreport_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_ce->serialize = zend_class_serialize_deny;
	opencreport_ce->unserialize = zend_class_unserialize_deny;
#endif

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("EXPR_RESULTS", OCRPT_EXPR_RESULTS);

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_PDF", OCRPT_OUTPUT_PDF);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_HTML", OCRPT_OUTPUT_HTML);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_TXT", OCRPT_OUTPUT_TXT);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_CSV", OCRPT_OUTPUT_CSV);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_XML", OCRPT_OUTPUT_XML);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("OUTPUT_JSON", OCRPT_OUTPUT_JSON);

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("RESULT_ERROR", OCRPT_RESULT_ERROR);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("RESULT_STRING", OCRPT_RESULT_STRING);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("RESULT_NUMBER", OCRPT_RESULT_NUMBER);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("RESULT_DATETIME", OCRPT_RESULT_DATETIME);

	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_EXPRESSION", OCRPT_VARIABLE_EXPRESSION);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_COUNT", OCRPT_VARIABLE_COUNT);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_COUNTALL", OCRPT_VARIABLE_COUNTALL);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_SUM", OCRPT_VARIABLE_SUM);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_AVERAGE", OCRPT_VARIABLE_AVERAGE);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_AVERAGEALL", OCRPT_VARIABLE_AVERAGEALL);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_LOWEST", OCRPT_VARIABLE_LOWEST);
	REGISTER_OPENCREPORT_CLASS_CONST_LONG("VARIABLE_HIGHEST", OCRPT_VARIABLE_HIGHEST);
}
