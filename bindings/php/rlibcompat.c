/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

ZEND_FUNCTION(rlib_version) {
	ZEND_PARSE_PARAMETERS_NONE();

	OCRPT_RETURN_STRING(ocrpt_version());
}

ZEND_FUNCTION(rlib_init) {
	ZEND_PARSE_PARAMETERS_NONE();

	object_init_ex(return_value, opencreport_ce);
	php_opencreport_object *oo = Z_OPENCREPORT_P(return_value);

	oo->o = ocrpt_init();
	oo->free_me = true;
	ocrpt_set_output_parameter(oo->o, "xml_rlib_compat", "yes");
}

ZEND_FUNCTION(rlib_free) {
	zval *obj;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(obj, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &obj, opencreport_ce) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(obj);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	opencreport_object_deinit(oo);
}

ZEND_FUNCTION(rlib_add_datasource_mysql) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *source_name;
	zend_string *host, *dbname;
	zend_string *user, *password;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 6, 6)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(host, 1, 0);
		Z_PARAM_STR_EX(user, 1, 0);
		Z_PARAM_STR_EX(password, 1, 0);
		Z_PARAM_STR_EX(dbname, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name;
	char *host = NULL, *dbname = NULL;
	char *user = NULL, *password = NULL;
	int source_name_len;
	int host_len, dbname_len;
	int user_len, password_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss!s!s!s!", &object, opencreport_ce, &source_name, &source_name_len, &host, &host_len, &user, &user_len, &password, &password_len, &dbname, &dbname_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "host", .param_value = host ? ZSTR_VAL(host) : NULL },
		{ .param_name = "dbname", .param_value = dbname ? ZSTR_VAL(dbname) : NULL },
		{ .param_name = "user", .param_value = user ? ZSTR_VAL(user) : NULL },
		{ .param_name = "password", .param_value = password ? ZSTR_VAL(password) : NULL },
		{ NULL }
	};

	ocrpt_datasource *ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "mariadb", conn_params);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_mysql_from_group) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *source_name;
	zend_string *option_file, *group;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 4)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(group, 1, 0);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(option_file, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name;
	char *option_file = NULL, *group = NULL;
	int source_name_len;
	int option_file_len, group_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss!|s!", &object, opencreport_ce, &source_name, &source_name_len, &option_file, &option_file_len, &group, &group_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "optionfile", .param_value = option_file ? ZSTR_VAL(option_file) : NULL },
		{ .param_name = "group", .param_value = group ? ZSTR_VAL(group) : NULL },
		{ NULL }
	};

	ocrpt_datasource *ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "mariadb", conn_params);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_postgres) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *source_name;
	zend_string *connection_info;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(connection_info, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name, *connection_info;
	int source_name_len, connection_info_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss!", &object, opencreport_ce, &source_name, &source_name_len, &connection_info, &connection_info_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "connstr", .param_value = connection_info ? ZSTR_VAL(connection_info) : NULL },
		{ NULL }
	};

	ocrpt_datasource *ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "postgresql", conn_params);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_odbc) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *source_name;
	zend_string *dbname, *user, *password;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 5)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR_EX(dbname, 1, 0);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(user, 1, 0);
		Z_PARAM_STR_EX(password, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name;
	char *dbname, *user, *password;
	int source_name_len;
	int dbname_len, user_len, password_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss!|s!s!", &object, opencreport_ce, &source_name, &source_name_len, &dbname, &dbname_len, &user, &user_len, &password, &password_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	struct ocrpt_input_connect_parameter conn_params[] = {
		{ .param_name = "dbname", .param_value = dbname ? ZSTR_VAL(dbname) : NULL },
		{ .param_name = "user", .param_value = user ? ZSTR_VAL(user) : NULL },
		{ .param_name = "password", .param_value = password ? ZSTR_VAL(password) : NULL },
		{ NULL }
	};

	ocrpt_datasource *ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "odbc", conn_params);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_array) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *source_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name;
	int source_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &source_name, &source_name_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_datasource *ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "array", NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_xml) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *source_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name;
	int source_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &source_name, &source_name_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_datasource *ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "xml", NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_datasource_csv) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *source_name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name;
	int source_name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &source_name, &source_name_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_datasource *ds = ocrpt_datasource_add(oo->o, ZSTR_VAL(source_name), "csv", NULL);
	if (!ds)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_ds_ce);
	php_opencreport_ds_object *dso = Z_OPENCREPORT_DS_P(return_value);
	dso->ds = ds;
}

ZEND_FUNCTION(rlib_add_query_as) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *source_name, *array_or_file_or_sql, *name;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 4, 4)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(source_name);
		Z_PARAM_STR(array_or_file_or_sql);
		Z_PARAM_STR(name);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *source_name, *array_or_file_or_sql, *name;
	int source_name_len, array_or_file_or_sql_len, name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Osss", &object, opencreport_ce, &source_name, &source_name_len, &array_or_file_or_sql, &array_or_file_or_sql_len, &name, &name_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_datasource *ds = ocrpt_datasource_get(oo->o, ZSTR_VAL(source_name));
	ocrpt_query *q;

	if (ocrpt_datasource_is_sql(ds))
		q = ocrpt_query_add_sql(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql));
	else if (ocrpt_datasource_is_file(ds))
		q = ocrpt_query_add_file(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), NULL, 0);
	else if (ocrpt_datasource_is_symbolic_data(ds))
		q = ocrpt_query_add_symbolic_data(ds, ZSTR_VAL(name), ZSTR_VAL(array_or_file_or_sql), 0, 0, NULL, 0);
	else
		RETURN_NULL();

	if (!q)
		RETURN_NULL();

	object_init_ex(return_value, opencreport_query_ce);
	php_opencreport_query_object *qo = Z_OPENCREPORT_QUERY_P(return_value);
	qo->q = q;
}

ZEND_FUNCTION(rlib_add_resultset_follower) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *leader, *follower;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(leader);
		Z_PARAM_STR(follower);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *leader, *follower;
	int leader_len, follower_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss", &object, opencreport_ce, &leader, &leader_len, &follower, &follower_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_query *leader_query = ocrpt_query_get(oo->o, ZSTR_VAL(leader));
	ocrpt_query *follower_query = ocrpt_query_get(oo->o, ZSTR_VAL(follower));

	if (!leader_query || !follower_query)
		RETURN_FALSE;

	RETURN_BOOL(ocrpt_query_add_follower(leader_query, follower_query));
}

ZEND_FUNCTION(rlib_add_resultset_follower_n_to_1) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *leader, *leader_field, *follower, *follower_field;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 5, 5)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(leader);
		Z_PARAM_STR(leader_field);
		Z_PARAM_STR(follower);
		Z_PARAM_STR(follower_field);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *leader, *leader_field, *follower, *follower_field;
	int leader_len, leader_field_len, follower_len, follower_field_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss", &object, opencreport_ce, &leader, &leader_len, &leader_field, &leader_field_len, &follower, &follower_len, &follower_field, &follower_field_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_query *leader_query = ocrpt_query_get(oo->o, ZSTR_VAL(leader));
	ocrpt_query *follower_query = ocrpt_query_get(oo->o, ZSTR_VAL(follower));

	if (!leader_query || !follower_query)
		RETURN_FALSE;

	ocrpt_string *match = ocrpt_mem_string_new_printf("(%s) = (%s)", ZSTR_VAL(leader_field), ZSTR_VAL(follower_field));
	char *err = NULL;
	ocrpt_expr *match_expr = ocrpt_expr_parse(oo->o, match->str, &err);

	if (!match_expr) {
		if (err) {
			zend_throw_error(NULL, "rlib_add_resultset_follower_n_to_1: expression parse error: %s\nrlib_add_resultset_follower_n_to_1: expression was: %s\n", err, match->str);
			ocrpt_strfree(err);
			ocrpt_mem_string_free(match, true);
		}
		RETURN_FALSE;
	}

	ocrpt_mem_string_free(match, true);

	RETURN_BOOL(ocrpt_query_add_follower_n_to_1(leader_query, follower_query, match_expr));
}

ZEND_FUNCTION(rlib_set_datasource_encoding) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *name, *encoding;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(name);
		Z_PARAM_STR(encoding);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *name, *encoding;
	int name_len, encoding_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss", &object, opencreport_ce, &name, &name_len, &encoding, &encoding_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_datasource *ds = ocrpt_datasource_get(oo->o, ZSTR_VAL(name));

	if (!ds)
		return;

	ocrpt_datasource_set_encoding(ds, ZSTR_VAL(encoding));
}

ZEND_FUNCTION(rlib_add_report) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *filename;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(filename);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *filename;
	int filename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &filename, &filename_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	RETURN_BOOL(ocrpt_parse_xml(oo->o, ZSTR_VAL(filename)));
#else
	RETURN_BOOL(ocrpt_parse_xml(oo->o, filename));
#endif
}

ZEND_FUNCTION(rlib_add_report_from_buffer) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *buffer;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(buffer);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *buffer;
	int buffer_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &buffer, &buffer_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	RETURN_BOOL(ocrpt_parse_xml_from_buffer(oo->o, ZSTR_VAL(buffer), ZSTR_LEN(buffer)));
#else
	RETURN_BOOL(ocrpt_parse_xml_from_buffer(oo->o, buffer, buffer_len));
#endif
}

ZEND_FUNCTION(rlib_add_search_path) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *path;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(path);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *path;
	int path_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &object, opencreport_ce, &path, &path_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	ocrpt_add_search_path(oo->o, ZSTR_VAL(path));
#else
	ocrpt_add_search_path(oo->o, path);
#endif
}

ZEND_FUNCTION(rlib_set_locale) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *locale;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(locale);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *locale;
	int locale_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &locale, &locale_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	ocrpt_set_locale(oo->o, ZSTR_VAL(locale));
#else
	ocrpt_set_locale(oo->o, locale);
#endif
}

ZEND_FUNCTION(rlib_bindtextdomain) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *domainname;
	zend_string *dirname;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(domainname);
		Z_PARAM_STR(dirname);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *domainname, *dirname;
	int domainname_len, dirname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss", &object, opencreport_ce, &domainname, &domainname_len, &dirname, &dirname_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

#if PHP_VERSION_ID >= 70000
	ocrpt_bindtextdomain(oo->o, ZSTR_VAL(domainname), ZSTR_VAL(dirname));
#else
	ocrpt_bindtextdomain(oo->o, domainname, dirname);
#endif
}

ZEND_FUNCTION(rlib_set_output_format_from_text) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *format;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(format);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *format;
	int format_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &format, &format_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_format_type type = OCRPT_OUTPUT_PDF;
	const char *fmt = ZSTR_VAL(format);

	if (strcasecmp(fmt, "pdf") == 0)
		type = OCRPT_OUTPUT_PDF;
	else if (strcasecmp(fmt, "html") == 0)
		type = OCRPT_OUTPUT_HTML;
	else if (strcasecmp(fmt, "txt") == 0 || strcasecmp(fmt, "text") == 0)
		type = OCRPT_OUTPUT_TXT;
	else if (strcasecmp(fmt, "csv") == 0)
		type = OCRPT_OUTPUT_CSV;
	else if (strcasecmp(fmt, "xml") == 0)
		type = OCRPT_OUTPUT_XML;
	else if (strcasecmp(fmt, "json") == 0)
		type = OCRPT_OUTPUT_JSON;

	ocrpt_set_output_format(oo->o, type);
}

OCRPT_STATIC_FUNCTION(rlib_default_function) {
	ocrpt_string *fname = user_data;
	ocrpt_string *sval;
	mpfr_ptr mval;
	bool failed = false;

	ocrpt_expr_make_error_result(e, "not implemented");

	int32_t n_ops = ocrpt_expr_get_num_operands(e);

#if PHP_VERSION_ID >= 70000
	zval zfname;
	zval retval;
	zval params[n_ops];

	ZVAL_STRINGL(&zfname, fname->str, fname->len);

	for (int32_t i = 0; i < n_ops; i++) {
		ocrpt_result *rs = ocrpt_expr_operand_get_result(e, i);
		switch (ocrpt_result_get_type(rs)) {
		case OCRPT_RESULT_ERROR:
			sval = ocrpt_result_get_string(rs);
			ocrpt_expr_make_error_result(e, sval->str);
			goto out;
		case OCRPT_RESULT_STRING:
			sval = ocrpt_result_get_string(rs);
			ZVAL_STRINGL(&params[i], sval->str, sval->len);
			break;
		case OCRPT_RESULT_NUMBER:
			/*
			 * Naive representation.
			 * MPFR number may exceed double precision.
			 */
			mval = ocrpt_result_get_number(rs);
			ZVAL_DOUBLE(&params[i], mpfr_get_d(mval, MPFR_RNDN));
			break;
		case OCRPT_RESULT_DATETIME:
			/* Not implemented yet */
			ZVAL_NULL(&params[i]);
			break;
		}
	}
#else
	zval *zfname;
	zval *retval;
	zval *params0[n_ops];
	zval ***params = emalloc(sizeof(zval **) * n_ops);

	MAKE_STD_ZVAL(zfname);
	ZVAL_STRINGL(zfname, fname->str, fname->len, 1);

	for (int32_t i = 0; i < n_ops; i++) {
		MAKE_STD_ZVAL(params0[i]);
		params[i] = &params0[i];

		ocrpt_result *rs = ocrpt_expr_operand_get_result(e, i);
		switch (ocrpt_result_get_type(rs)) {
		case OCRPT_RESULT_ERROR:
			sval = ocrpt_result_get_string(rs);
			ocrpt_expr_make_error_result(e, sval->str);
			goto out;
		case OCRPT_RESULT_STRING:
			sval = ocrpt_result_get_string(rs);
			ZVAL_STRINGL(params0[i], sval->str, sval->len, 1);
			break;
		case OCRPT_RESULT_NUMBER:
			/*
			 * Naive representation.
			 * MPFR number may exceed double precision.
			 */
			mval = ocrpt_result_get_number(rs);
			ZVAL_DOUBLE(params0[i], mpfr_get_d(mval, MPFR_RNDN));
			break;
		case OCRPT_RESULT_DATETIME:
			/* Not implemented yet */
			ZVAL_NULL(params0[i]);
			break;
		}
	}
#endif

#if PHP_VERSION_ID >= 80000
	failed = (call_user_function(CG(function_table), NULL, &zfname, &retval, n_ops, params) == FAILURE);
#elif PHP_VERSION_ID >= 70000
	failed = (call_user_function_ex(CG(function_table), NULL, &zfname, &retval, n_ops, params, 0, NULL TSRMLS_CC) == FAILURE);
#else
	failed = (call_user_function_ex(CG(function_table), NULL, zfname, &retval, n_ops, params, 0, NULL TSRMLS_CC) == FAILURE);
#endif

out:
#if PHP_VERSION_ID >= 70000
	zend_string_release(Z_STR(zfname));
#else
	zval_dtor(zfname);
	efree(zfname);
	efree(params);
#endif

	for (int32_t i = 0; i < n_ops; i++) {
#if PHP_VERSION_ID >= 70000
		ocrpt_result *rs = ocrpt_expr_operand_get_result(e, i);
		switch (ocrpt_result_get_type(rs)) {
		case OCRPT_RESULT_ERROR:
			break;
		case OCRPT_RESULT_STRING:
			zend_string_release(Z_STR(params[i]));
			break;
		case OCRPT_RESULT_NUMBER:
			break;
		case OCRPT_RESULT_DATETIME:
			break;
		}
#else
		zval_dtor(params0[i]);
		efree(params0[i]);
#endif
	}

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
	if (!retval || failed)
		ocrpt_expr_make_error_result(e, "invalid function call");
	else if (Z_TYPE_P(retval) == IS_NULL)
		/* Do nothing. */;
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

	zval_dtor(retval);
	efree(retval);
#endif
}

ZEND_FUNCTION(rlib_add_function) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *name, *function;
	zend_long params;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 4, 4)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(name);
		Z_PARAM_STR(function);
		Z_PARAM_LONG(params);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *name, *function;
	int name_len, function_len;
	long params;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ossl", &object, opencreport_ce, &name, &name_len, &function, &function_len, &params) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_string *ofunc = ocrpt_mem_string_new_with_len(ZSTR_VAL(function), ZSTR_LEN(function));

	ocrpt_function_add(oo->o, ZSTR_VAL(name), rlib_default_function, ofunc, params, false, false, false, true);
}

ZEND_FUNCTION(rlib_set_output_encoding) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *encoding;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(encoding);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *encoding;
	int encoding_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ossl", &object, opencreport_ce, &encoding, &encoding_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Do nothing. OpenCReports is UTF-8 only. */
}

ZEND_FUNCTION(rlib_add_parameter) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *param;
	zend_string *value = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(param);
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR_EX(value, 1, 0);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *param, *value = NULL;
	int param_len, value_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os|s!", &object, opencreport_ce, &param, &param_len, &value, &value_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_set_mvariable(oo->o, ZSTR_VAL(param), value ? ZSTR_VAL(value) : NULL);
}

ZEND_FUNCTION(rlib_set_output_parameter) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *param, *value;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(param);
		Z_PARAM_STR(value);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *param, *value;
	int param_len, value_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss", &object, opencreport_ce, &param, &param_len, &value, &value_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_set_output_parameter(oo->o, ZSTR_VAL(param), ZSTR_VAL(value));
}

ZEND_FUNCTION(rlib_query_refresh) {
	zval *object;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &object, opencreport_ce) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_query_refresh(oo->o);
}

static void rlib_report_cb(opencreport *o, ocrpt_report *r, void *data) {
	char *callback = data;

#if PHP_VERSION_ID >= 70000
	zval zfname;
	zval retval;

	ZVAL_STRING(&zfname, callback);
#else
	zval *zfname;
	zval *retval = NULL;

	MAKE_STD_ZVAL(zfname);
	ZVAL_STRING(zfname, callback, 1);
#endif

#if PHP_VERSION_ID >= 80000
	call_user_function(CG(function_table), NULL, &zfname, &retval, 0, NULL);
#elif PHP_VERSION_ID >= 70000
	call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 0, NULL, 0, NULL TSRMLS_CC);
#else
	call_user_function_ex(CG(function_table), NULL, zfname, &retval, 0, NULL, 0, NULL TSRMLS_CC);
#endif

#if PHP_VERSION_ID >= 70000
	zend_string_release(Z_STR(zfname));
#else
	zval_dtor(zfname);
	efree(zfname);
	if (retval) {
		zval_dtor(retval);
		efree(retval);
	}
#endif
}

static void rlib_part_cb(opencreport *o, ocrpt_part *p, void *data) {
	char *callback = data;

#if PHP_VERSION_ID >= 70000
	zval zfname;
	zval retval;

	ZVAL_STRING(&zfname, callback);
#else
	zval *zfname;
	zval *retval = NULL;

	MAKE_STD_ZVAL(zfname);
	ZVAL_STRING(zfname, callback, 1);
#endif

#if PHP_VERSION_ID >= 80000
	call_user_function(CG(function_table), NULL, &zfname, &retval, 0, NULL);
#elif PHP_VERSION_ID >= 70000
	call_user_function_ex(CG(function_table), NULL, &zfname, &retval, 0, NULL, 0, NULL TSRMLS_CC);
#else
	call_user_function_ex(CG(function_table), NULL, zfname, &retval, 0, NULL, 0, NULL TSRMLS_CC);
#endif

#if PHP_VERSION_ID >= 70000
	zend_string_release(Z_STR(zfname));
#else
	zval_dtor(zfname);
	efree(zfname);
	if (retval) {
		zval_dtor(retval);
		efree(retval);
	}
#endif
}

ZEND_FUNCTION(rlib_signal_connect) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *signal, *function;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(signal);
		Z_PARAM_STR(function);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *signal, *function;
	int signal_len, function_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss", &object, opencreport_ce, &signal, &signal_len, &function, &function_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_string *cb_name = ocrpt_mem_strdup(ZSTR_VAL(function));

	char *signal_name = ZSTR_VAL(signal);
	if (!strcasecmp(signal_name, "row_change"))
		ocrpt_report_add_new_row_cb2(oo->o, rlib_report_cb, cb_name);
	else if (!strcasecmp(signal_name, "report_done"))
		ocrpt_report_add_done_cb2(oo->o, rlib_report_cb, cb_name);
	else if (!strcasecmp(signal_name, "report_start"))
		ocrpt_report_add_start_cb2(oo->o, rlib_report_cb, cb_name);
	else if (!strcasecmp(signal_name, "report_iteration"))
		ocrpt_report_add_iteration_cb2(oo->o, rlib_report_cb, cb_name);
	else if (!strcasecmp(signal_name, "part_iteration"))
		ocrpt_part_add_iteration_cb2(oo->o, rlib_part_cb, cb_name);
	else if (!strcasecmp(signal_name, "precalculation_done"))
		ocrpt_report_add_precalculation_done_cb2(oo->o, rlib_report_cb, cb_name);
}

ZEND_FUNCTION(rlib_execute) {
	zval *object;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &object, opencreport_ce) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_execute(oo->o);
}

ZEND_FUNCTION(rlib_spool) {
	zval *object;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &object, opencreport_ce) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	ocrpt_spool(oo->o);
}

ZEND_FUNCTION(rlib_get_content_type) {
	zval *object;

#if PHP_VERSION_ID >= 70000
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
	ZEND_PARSE_PARAMETERS_END();
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &object, opencreport_ce) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	const ocrpt_string **content_type = ocrpt_get_content_type(oo->o);

	if (!content_type)
		RETURN_FALSE;

	ocrpt_string *retval = ocrpt_mem_string_new("", true);

	for (int32_t i = 0; content_type[i]; i++)
		ocrpt_mem_string_append_printf(retval, "%s\n", content_type[i]->str);

	OCRPT_RETVAL_STRINGL(retval->str, retval->len);

	ocrpt_mem_string_free(retval, true);
}

ZEND_FUNCTION(rlib_set_radix_character) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *radix;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(radix);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *radix;
	int radix_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &radix, &radix_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. The radix character depends on the locale. */
}

ZEND_FUNCTION(rlib_compile_infix) {
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

	opencreport *o = ocrpt_init();

	char *err = NULL;
	ocrpt_expr *e = ocrpt_expr_parse(o, ZSTR_VAL(expr_string), &err);

	if (!e) {
		OCRPT_RETVAL_STRING(err);
		ocrpt_strfree(err);
		ocrpt_free(o);
		return;
	}

	ocrpt_expr_optimize(e);
	ocrpt_expr_eval(e);

	mpfr_ptr mval;
	ocrpt_string *sval;
	ocrpt_result *rs = ocrpt_expr_get_result(e);
	switch (ocrpt_result_get_type(rs)) {
	case OCRPT_RESULT_ERROR:
	case OCRPT_RESULT_STRING:
		sval = ocrpt_result_get_string(rs);
		OCRPT_RETVAL_STRINGL(sval->str, sval->len);
		break;
	case OCRPT_RESULT_NUMBER:
		/*
		 * Naive representation.
		 * MPFR number may exceed double precision.
		 */
		mval = ocrpt_result_get_number(rs);
		RETVAL_DOUBLE(mpfr_get_d(mval, MPFR_RNDN));
		break;
	case OCRPT_RESULT_DATETIME:
		/* Not implemented yet */
		RETVAL_NULL();
		break;
	}

	ocrpt_free(o);
}

ZEND_FUNCTION(rlib_graph_add_bg_region) {
	zval *object;
	double start, end;
#if PHP_VERSION_ID >= 70000
	zend_string *graph, *region, *color;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 6, 6)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(graph);
		Z_PARAM_STR(region);
		Z_PARAM_STR(color);
		Z_PARAM_DOUBLE(start);
		Z_PARAM_DOUBLE(end);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *graph, *region, *color;
	int graph_len, region_len, color_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Osssdd", &object, opencreport_ce, &graph, &graph_len, &region, &region_len, &color, &color_len, &start, &end) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. Not implemented yet. */
}

ZEND_FUNCTION(rlib_graph_clear_bg_region) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *graph;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(graph);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *graph;
	int graph_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &object, opencreport_ce, &graph, &graph_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. Not implemented yet. */
}

ZEND_FUNCTION(rlib_graph_set_x_minor_tick) {
	zval *object;
#if PHP_VERSION_ID >= 70000
	zend_string *graph, *x;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(graph);
		Z_PARAM_STR(x);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *graph, *x;
	int graph_len, x_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss", &object, opencreport_ce, &graph, &graph_len, &x, &x_len) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. Not implemented yet. */
}

ZEND_FUNCTION(rlib_graph_set_x_minor_tick_by_location) {
	zval *object;
	double location;
#if PHP_VERSION_ID >= 70000
	zend_string *graph;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 3)
		Z_PARAM_OBJECT_OF_CLASS(object, opencreport_ce);
		Z_PARAM_STR(graph);
		Z_PARAM_DOUBLE(location);
	ZEND_PARSE_PARAMETERS_END();
#else
	char *graph;
	int graph_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Osd", &object, opencreport_ce, &graph, &graph_len, &location) == FAILURE)
		return;
#endif

	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	if (!oo->o) {
		zend_throw_error(NULL, "OpenCReport object was freed");
		RETURN_THROWS();
	}

	/* Silently do nothing. Not implemented yet. */
}

#if PHP_VERSION_ID >= 70000

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_version, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_init, 0, 0, OpenCReport, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_free, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_mysql, 0, 6, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, password, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dbname, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_mysql_from_group, 0, 3, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, group, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, option_file, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_postgres, 0, 3, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, connection_info, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_odbc, 0, 3, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dsn, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, user, IS_STRING, 1)
ZEND_ARG_VARIADIC_TYPE_INFO(0, password, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_array, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_xml, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_datasource_csv, 0, 2, OpenCReport\\Datasource, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_rlib_add_query_as, 0, 4, OpenCReport\\Query, 1)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, source_name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, array_or_file_or_sql, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, query_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_resultset_follower, 0, 3, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, leader, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, follower, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_resultset_follower_n_to_1, 0, 5, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, leader, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, leader_field, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, follower, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, follower_field, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_datasource_encoding, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, encoding, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_report, 0, 2, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_report_from_buffer, 0, 2, _IS_BOOL, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, buffer, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_search_path, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_locale, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, locale, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_bindtextdomain, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, domainname, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, dirname, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_output_format_from_text, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, format, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_function, 0, 4, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, function, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, params, IS_LONG, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_output_encoding, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, encoding, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_add_parameter, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, param, IS_STRING, 0)
ZEND_ARG_VARIADIC_TYPE_INFO(0, value, IS_STRING, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_output_parameter, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, param, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_query_refresh, 0, 0, IS_VOID, 0)
ZEND_ARG_VARIADIC_OBJ_INFO(0, r, OpenCReport, 1)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_signal_connect, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, signal, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, function, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_execute, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_spool, 0, 1, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_get_content_type, 0, 1, IS_STRING, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_set_radix_character, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, radix, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* The return type may be string, double or false */
OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_compile_infix, 0, 1, IS_STRING, 1)
ZEND_ARG_TYPE_INFO(0, expr_string, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_graph_add_bg_region, 0, 6, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, graph, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, region, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, color, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, start, IS_DOUBLE, 0)
ZEND_ARG_TYPE_INFO(0, end, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_graph_clear_bg_region, 0, 2, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, graph, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_graph_set_x_minor_tick, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, graph, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, x, IS_STRING, 0)
ZEND_END_ARG_INFO()

OCRPT_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_rlib_graph_set_x_minor_tick_by_location, 0, 3, IS_VOID, 0)
ZEND_ARG_OBJ_INFO(0, r, OpenCReport, 0)
ZEND_ARG_TYPE_INFO(0, graph, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, location, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#else

#define arginfo_rlib_version NULL
#define arginfo_rlib_init NULL
#define arginfo_rlib_free NULL
#define arginfo_rlib_add_datasource_mysql NULL
#define arginfo_rlib_add_datasource_mysql_from_group NULL
#define arginfo_rlib_add_datasource_postgres NULL
#define arginfo_rlib_add_datasource_odbc NULL
#define arginfo_rlib_add_datasource_array NULL
#define arginfo_rlib_add_datasource_xml NULL
#define arginfo_rlib_add_datasource_csv NULL
#define arginfo_rlib_add_query_as NULL
#define arginfo_rlib_add_resultset_follower NULL
#define arginfo_rlib_add_resultset_follower_n_to_1 NULL
#define arginfo_rlib_set_datasource_encoding NULL
#define arginfo_rlib_add_report NULL
#define arginfo_rlib_add_report_from_buffer NULL
#define arginfo_rlib_add_search_path NULL
#define arginfo_rlib_set_locale NULL
#define arginfo_rlib_bindtextdomain NULL
#define arginfo_rlib_set_output_format_from_text NULL
#define arginfo_rlib_add_function NULL
#define arginfo_rlib_set_output_encoding NULL
#define arginfo_rlib_add_parameter NULL
#define arginfo_rlib_set_output_parameter NULL
#define arginfo_rlib_query_refresh NULL
#define arginfo_rlib_signal_connect NULL
#define arginfo_rlib_execute NULL
#define arginfo_rlib_spool NULL
#define arginfo_rlib_get_content_type NULL
#define arginfo_rlib_set_radix_character NULL
#define arginfo_rlib_compile_infix NULL
#define arginfo_rlib_graph_add_bg_region NULL
#define arginfo_rlib_graph_clear_bg_region NULL
#define arginfo_rlib_graph_set_x_minor_tick NULL
#define arginfo_rlib_graph_set_x_minor_tick_by_location NULL

#endif

/* {{{ zend_function_entry */
const zend_function_entry opencreport_functions[] = {
	ZEND_FE(rlib_version, arginfo_rlib_version)
	ZEND_FE(rlib_init, arginfo_rlib_init)
	ZEND_FE(rlib_free, arginfo_rlib_free)
	ZEND_FE(rlib_add_datasource_mysql, arginfo_rlib_add_datasource_mysql)
	ZEND_FE(rlib_add_datasource_mysql_from_group, arginfo_rlib_add_datasource_mysql_from_group)
	ZEND_FE(rlib_add_datasource_postgres, arginfo_rlib_add_datasource_postgres)
	ZEND_FE(rlib_add_datasource_odbc, arginfo_rlib_add_datasource_odbc)
	ZEND_FE(rlib_add_datasource_array, arginfo_rlib_add_datasource_array)
	ZEND_FE(rlib_add_datasource_xml, arginfo_rlib_add_datasource_xml)
	ZEND_FE(rlib_add_datasource_csv, arginfo_rlib_add_datasource_csv)
	ZEND_FE(rlib_add_query_as, arginfo_rlib_add_query_as)
	ZEND_FE(rlib_add_resultset_follower, arginfo_rlib_add_resultset_follower)
	ZEND_FE(rlib_add_resultset_follower_n_to_1, arginfo_rlib_add_resultset_follower_n_to_1)
	ZEND_FE(rlib_set_datasource_encoding, arginfo_rlib_set_datasource_encoding)
	ZEND_FE(rlib_add_report, arginfo_rlib_add_report)
	ZEND_FE(rlib_add_report_from_buffer, arginfo_rlib_add_report_from_buffer)
	ZEND_FE(rlib_add_search_path, arginfo_rlib_add_search_path)
	ZEND_FE(rlib_set_locale, arginfo_rlib_set_locale)
	ZEND_FE(rlib_bindtextdomain, arginfo_rlib_bindtextdomain)
	ZEND_FE(rlib_set_output_format_from_text, arginfo_rlib_set_output_format_from_text)
	ZEND_FE(rlib_add_function, arginfo_rlib_add_function)
	ZEND_FE(rlib_set_output_encoding, arginfo_rlib_set_output_encoding)
	ZEND_FE(rlib_add_parameter, arginfo_rlib_add_parameter)
	ZEND_FE(rlib_set_output_parameter, arginfo_rlib_set_output_parameter)
	ZEND_FE(rlib_query_refresh, arginfo_rlib_query_refresh)
	ZEND_FE(rlib_signal_connect, arginfo_rlib_signal_connect)
	ZEND_FE(rlib_execute, arginfo_rlib_execute)
	ZEND_FE(rlib_spool, arginfo_rlib_spool)
	ZEND_FE(rlib_get_content_type, arginfo_rlib_get_content_type)
	ZEND_FE(rlib_set_radix_character, arginfo_rlib_set_radix_character)
	ZEND_FE(rlib_compile_infix, arginfo_rlib_compile_infix)
	ZEND_FE(rlib_graph_add_bg_region, arginfo_rlib_graph_add_bg_region)
	ZEND_FE(rlib_graph_clear_bg_region, arginfo_rlib_graph_clear_bg_region)
	ZEND_FE(rlib_graph_set_x_minor_tick, arginfo_rlib_graph_set_x_minor_tick)
	ZEND_FE(rlib_graph_set_x_minor_tick_by_location, arginfo_rlib_graph_set_x_minor_tick_by_location)
	PHP_FE_END
};
/* }}} */

