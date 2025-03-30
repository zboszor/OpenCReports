/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

static zend_object_handlers opencreport_ds_object_handlers;

zend_class_entry *opencreport_ds_ce;

#if PHP_VERSION_ID >= 70000
static zend_object *opencreport_ds_object_new(zend_class_entry *class_type) /* {{{ */
#else
static zend_object_value opencreport_ds_object_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
#endif
{
#if PHP_VERSION_ID < 70000
	zend_object_value retval;
#endif
	php_opencreport_ds_object *intern = zend_object_alloc(sizeof(php_opencreport_ds_object), class_type);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

#if PHP_VERSION_ID < 70000
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
	retval.handlers = &opencreport_ds_object_handlers;
	return retval;
#else
	intern->zo.handlers = &opencreport_ds_object_handlers;
	return &intern->zo;
#endif
}
/* }}} */

PHP_METHOD(opencreport_ds, free) {
	zval *object = getThis();
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(object);

	ZEND_PARSE_PARAMETERS_NONE();

	ocrpt_datasource_free(dso->ds);
	dso->ds = NULL;
}

PHP_METHOD(opencreport_ds, query_add) {
	zval *object = getThis();
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(object);
	ocrpt_query *q;
	php_opencreport_query_object *qo;

#if PHP_VERSION_ID >= 70000
	zend_string *name, *array_or_file_or_sql, *coltypes = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_STR(name);
		Z_PARAM_STR(array_or_file_or_sql);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(coltypes, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *name, *array_or_file_or_sql, *coltypes = NULL;
	int name_len, array_or_file_or_sql_len, coltypes_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|s!", &name, &name_len, &array_or_file_or_sql, &array_or_file_or_sql_len, &coltypes, &coltypes_len) == FAILURE)
		return;
#endif

	if (ocrpt_datasource_is_sql(dso->ds)) {
		q = ocrpt_query_add_sql(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	} else if (ocrpt_datasource_is_file(dso->ds)) {
		void *types_x = NULL;
		int32_t types_cols = 0;
		bool free_types = false;

		if (coltypes && ZSTR_LEN(coltypes) > 0)
			ocrpt_query_discover_data(NULL, NULL, NULL, NULL, ZSTR_VAL(coltypes), &types_x, &types_cols, &free_types);

		q = ocrpt_query_add_file(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), types_x, types_cols);

		if (free_types)
			ocrpt_mem_free(types_x);
	} else if (ocrpt_datasource_is_symbolic_data(dso->ds))
		q = ocrpt_query_add_symbolic_data(dso->ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), 0, 0, coltypes ? ZSTR_VAL(coltypes) : NULL, 0);
	else
		RETURN_NULL();

	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;
}

PHP_METHOD(opencreport_ds, set_encoding) {
	zval *object = getThis();
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(object);
#if PHP_VERSION_ID >= 70000
	zend_string *encoding;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_STR(encoding);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *encoding;
	int encoding_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &encoding, &encoding_len) == FAILURE)
		return;
#endif

	ocrpt_datasource_set_encoding(dso->ds, ZSTR_VAL(encoding));
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_ds_free, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_opencreport_ds_query_add, 0, 2, OpenCReport\\Query, 1)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, array_or_file_or_sql, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, types, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_opencreport_ds_set_encoding, 0, 1, IS_VOID, 0)
ZEND_ARG_TYPE_INFO(0, encoding, IS_STRING, 0)
ZEND_END_ARG_INFO()

#else

#define arginfo_opencreport_ds_free NULL
#define arginfo_opencreport_ds_query_add NULL
#define arginfo_opencreport_ds_set_encoding NULL

#endif

static const zend_function_entry opencreport_ds_class_methods[] = {
	PHP_ME(opencreport_ds, free, arginfo_opencreport_ds_free, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_ds, query_add, arginfo_opencreport_ds_query_add, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_ME(opencreport_ds, set_encoding, arginfo_opencreport_ds_set_encoding, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
	PHP_FE_END
};

void register_opencreport_ds_ce(void) {
	zend_class_entry ce;

	memcpy(&opencreport_ds_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	INIT_NS_CLASS_ENTRY(ce, "OpenCReport", "Datasource", opencreport_ds_class_methods);
	ce.create_object = opencreport_ds_object_new;
#if PHP_VERSION_ID >= 70000
	opencreport_ds_object_handlers.offset = XtOffsetOf(php_opencreport_ds_object, zo);
#endif
	opencreport_ds_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
	opencreport_ds_object_handlers.compare = zend_objects_not_comparable;
#endif
	opencreport_ds_ce = zend_register_internal_class(&ce);
	opencreport_ds_ce->ce_flags |= ZEND_ACC_FINAL;
#if PHP_VERSION_ID >= 80100
	opencreport_ds_ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_ds_ce->serialize = zend_class_serialize_deny;
	opencreport_ds_ce->unserialize = zend_class_unserialize_deny;
#endif
}
