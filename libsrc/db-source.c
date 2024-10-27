/*
 * OpenCReports array data source
 *
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <alloca.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_POSTGRESQL
#include <libpq-fe.h>
#endif
#include <mysql.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#ifndef SQL_ATTR_APP_WCHAR_TYPE
#define SQL_ATTR_APP_WCHAR_TYPE 1061
#endif
#ifndef SQL_ATTR_APP_UNICODE_TYPE
#define SQL_ATTR_APP_UNICODE_TYPE 1064
#endif
#ifndef SQL_DD_CP_ANSI
#define SQL_DD_CP_ANSI 0
#endif
#ifndef SQL_DD_CP_UCS2
#define SQL_DD_CP_UCS2 1
#endif
#ifndef SQL_DD_CP_UTF8
#define SQL_DD_CP_UTF8 2
#endif
#ifndef SQL_DD_CP_UTF16
#define SQL_DD_CP_UTF16 SQL_DD_CP_UCS2
#endif

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "datasource.h"

#if HAVE_POSTGRESQL
/* Fetch (cache) this many rows at once from the cursor */
#define PGFETCHSIZE (1024)

struct ocrpt_postgresql_results {
	ocrpt_query_result *result;
	PGresult *desc;
	char *rewindquery;
	char *fetchquery;
	PGresult *res;
	int32_t cols;
	int32_t chunk;
	int32_t row;
	bool isdone;
};
typedef struct ocrpt_postgresql_results ocrpt_postgresql_results;

static const ocrpt_input_connect_parameter ocrpt_postgresql_connect_method1[] = {
	{ .param_name = "connstr", { .optional = false } },
	{ .param_name = NULL }
};

static const ocrpt_input_connect_parameter ocrpt_postgresql_connect_method2[] = {
	{ .param_name = "unix_socket", { .optional = false } },
	{ .param_name = "dbname", { .optional = false } },
	{ .param_name = "user", { .optional = true } },
	{ .param_name = "password", { .optional = true } },
	{ .param_name = NULL }
};

static const ocrpt_input_connect_parameter ocrpt_postgresql_connect_method3[] = {
	{ .param_name = "dbname", { .optional = false } },
	{ .param_name = "host", { .optional = true } },
	{ .param_name = "port", { .optional = true } },
	{ .param_name = "user", { .optional = true } },
	{ .param_name = "password", { .optional = true } },
	{ .param_name = NULL }
};
#endif

static bool ocrpt_postgresql_connect(ocrpt_datasource *source, const ocrpt_input_connect_parameter *params) {
#if HAVE_POSTGRESQL
	if (!source || !params)
		return false;

#define PG_APPNAMEIDX (1)
#define PG_DBNAMEIDX (2)

	int32_t i, n_keywords = 3;
	const char *keywords[8] = { "client_encoding", "application_name", "dbname", NULL };
	const char *values[8] = { "UTF-8", NULL, NULL, NULL };
	char *appname;

	const char *dsname = ocrpt_datasource_get_name(source);
	int32_t len = snprintf(NULL, 0, PACKAGE_STRING " datasource %s", dsname);
	appname = alloca(len + 1);
	snprintf(appname, len + 1, PACKAGE_STRING " datasource %s", dsname);
	values[PG_APPNAMEIDX] = appname;

	for (i = 0; params[i].param_name; i++) {
		if (strcasecmp(params[i].param_name, "connstr") == 0 ||
				strcasecmp(params[i].param_name, "dbname") == 0 ||
				strcasecmp(params[i].param_name, "unix_socket") == 0)
			values[PG_DBNAMEIDX] = params[i].param_value;
	}

	if (values[PG_DBNAMEIDX] == NULL)
		return false;

	for (i = 0; params[i].param_name; i++) {
		if (strcasecmp(params[i].param_name, "host") == 0 ||
				strcasecmp(params[i].param_name, "port") == 0 ||
				strcasecmp(params[i].param_name, "user") == 0 ||
				strcasecmp(params[i].param_name, "password") == 0) {
			keywords[n_keywords] = params[i].param_name;
			values[n_keywords] = params[i].param_value;
			n_keywords++;
		}
	}

	keywords[n_keywords] = NULL;
	values[n_keywords] = NULL;

	PGconn *conn = PQconnectdbParams(keywords, values, 1);
	if (PQstatus(conn) != CONNECTION_OK) {
		PQfinish(conn);
		return false;
	}

	ocrpt_datasource_set_private(source, conn);

	return true;
#else
	return false;
#endif
}

#ifdef HAVE_POSTGRESQL
static ocrpt_query *ocrpt_postgresql_query_add(ocrpt_datasource *source, const char *name, const char *querystr) {
	if (!source || !name || !*name || !querystr || !*querystr)
		return NULL;

	int32_t len = snprintf(NULL, 0, "DECLARE \"%s\" SCROLL CURSOR WITH HOLD FOR %s", name, querystr);
	char *cursor = alloca(len + 1);
	snprintf(cursor, len + 1, "DECLARE \"%s\" SCROLL CURSOR WITH HOLD FOR %s", name, querystr);
	cursor[len] = 0;

	PGconn *conn = (PGconn *)ocrpt_datasource_get_private(source);
	PGresult *res = PQexec(conn, cursor);
	switch (PQresultStatus(res)) {
	case PGRES_COMMAND_OK:
	case PGRES_NONFATAL_ERROR:
		break;
	default:
		ocrpt_err_printf("failed to execute query: %s\nwith error message: %s", querystr, PQerrorMessage(conn));
		PQclear(res);
		return NULL;
	}
	PQclear(res);

	ocrpt_query *query = ocrpt_query_alloc(source, name);
	if (!query)
		goto out_error;

	struct ocrpt_postgresql_results *result = ocrpt_mem_malloc(sizeof(struct ocrpt_postgresql_results));
	if (!result) {
		ocrpt_query_free(query);
		goto out_error;
	}

	memset(result, 0, sizeof(ocrpt_postgresql_results));

	snprintf(cursor, len + 1, "FETCH %d FROM \"%s\"", PGFETCHSIZE, name);
	result->fetchquery = ocrpt_mem_strdup(cursor);
	result->chunk = -1;
	result->row = -1;
	ocrpt_query_set_private(query, result);

	return query;

	out_error:

	snprintf(cursor, len + 1, "CLOSE \"%s\"", name);
	res = PQexec(conn, cursor);
	PQclear(res);
	return NULL;
}

static void ocrpt_postgresql_describe(ocrpt_query *query, ocrpt_query_result **qresult, int32_t *cols) {
	ocrpt_postgresql_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	PGconn *conn = ocrpt_datasource_get_private(source);
	opencreport *o = ocrpt_datasource_get_opencreport(source);

	if (!result->result) {
		PGresult *res;
		ocrpt_query_result *qr;
		int32_t i;

		res = PQdescribePortal(conn, ocrpt_query_get_name(query));
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			PQclear(res);
			if (qresult)
				*qresult = NULL;
			if (cols)
				*cols = 0;
			return;
		}

		result->cols = PQnfields(res);
		qr = ocrpt_mem_malloc(OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));

		if (!qr) {
			PQclear(res);
			if (qresult)
				*qresult = NULL;
			if (cols)
				*cols = 0;
			return;
		}

		memset(qr, 0, OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));

		for (i = 0; i < result->cols; i++) {
			enum ocrpt_result_type type;

			switch (PQftype(res, i)) {
			case 16: /* bool */
			case 20: /* int8 */
			case 21: /* int2 */
			case 23: /* int4 */
			case 26: /* oid */
			case 700: /* float4 */
			case 701: /* float8 */
			case 1700: /* numeric */
				type = OCRPT_RESULT_NUMBER;
				break;
			case 18: /* char */
			case 19: /* name */
			case 25: /* text */
			case 1042: /* bpchar */
			case 1043: /* varchar */
			case 1560: /* bit */
			case 1562: /* varbit */
				type = OCRPT_RESULT_STRING;
				break;
			case 1082: /* date */
			case 1083: /* time */
			case 1114: /* timestamp */
			case 1184: /* timestamptz */
			case 1186: /* interval */
			case 1266: /* timetz */
				type = OCRPT_RESULT_DATETIME;
				break;
			default:
				ocrpt_err_printf("type %d\n", PQftype(res, i));
				type = OCRPT_RESULT_STRING;
				break;
			}



			for (int j = 0; j < OCRPT_EXPR_RESULTS; j++) {
				int32_t idx = j * result->cols + i;

				qr[idx].name = PQfname(res, i);
				qr[idx].result.o = o;
				qr[idx].result.type = type;
				qr[idx].result.orig_type = type;

				if (qr[idx].result.type == OCRPT_RESULT_NUMBER) {
					mpfr_init2(qr[idx].result.number, ocrpt_get_numeric_precision_bits(o));
					qr[idx].result.number_initialized = true;
				}

				qr[idx].result.isnull = true;
			}
		}

		result->result = qr;
		result->desc = res;
	}

	if (qresult)
		*qresult = result->result;
	if (cols)
		*cols = (result->result ? result->cols : 0);
}

static PGresult *ocrpt_postgresql_fetch(ocrpt_query *query) {
	ocrpt_postgresql_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	PGconn *conn = ocrpt_datasource_get_private(source);
	PGresult *res;

	if (result->res) {
		PQclear(result->res);
		result->res = NULL;
	}

	res = PQexec(conn, result->fetchquery);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		return NULL;
	}

	result->res = res;
	result->chunk++;
	result->row = -1;

	return res;
}

static void ocrpt_postgresql_rewind(ocrpt_query *query) {
	ocrpt_postgresql_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	PGconn *conn = ocrpt_datasource_get_private(source);

	if (result->chunk > 0) {
		if (!result->rewindquery) {
			int len = snprintf(NULL, 0, "MOVE ABSOLUTE 0 IN \"%s\"", query->name);

			result->rewindquery = ocrpt_mem_malloc(len + 1);
			if (result->rewindquery) {
				len = snprintf(result->rewindquery, len + 1, "MOVE ABSOLUTE 0 IN \"%s\"", query->name);
				result->rewindquery[len] = 0;
			}
		}

		if (result->rewindquery) {
			PGresult *res = PQexec(conn, result->rewindquery);
			PQclear(res);
		}

		PQclear(result->res);
		result->res = NULL;
		result->chunk = -1;
	}

	if (result->chunk == -1)
		result->res = ocrpt_postgresql_fetch(query);

	result->row = -1;
	result->isdone = false;
}

static bool ocrpt_postgresql_populate_result(ocrpt_query *query) {
	struct ocrpt_postgresql_results *result = ocrpt_query_get_private(query);
	int32_t i;

	if (result->isdone || result->row < 0) {
		ocrpt_query_result_set_values_null(query);
		return false;
	}

	for (i = 0; i < result->cols; i++) {
		bool isnull = PQgetisnull(result->res, result->row, i);
		const char *str = PQgetvalue(result->res, result->row, i);
		int32_t len = PQgetlength(result->res, result->row, i);

		ocrpt_query_result_set_value(query, i, isnull, (iconv_t)-1, str, len);
	}

	return true;
}

static bool ocrpt_postgresql_next(ocrpt_query *query) {
	ocrpt_postgresql_results *result = ocrpt_query_get_private(query);

	if (result->isdone)
		return false;

	if (result->res == NULL || (result->res != NULL && (result->row == PGFETCHSIZE - 1)))
		ocrpt_postgresql_fetch(query);

	result->row++;

	result->isdone = (result->row < PGFETCHSIZE && result->row == PQntuples(result->res));
	return ocrpt_postgresql_populate_result(query);
}

static bool ocrpt_postgresql_isdone(ocrpt_query *query) {
	ocrpt_postgresql_results *result = ocrpt_query_get_private(query);

	return result->isdone;
}

static void ocrpt_postgresql_free(ocrpt_query *query) {
	ocrpt_postgresql_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	PGconn *conn = ocrpt_datasource_get_private(source);
	PGresult *res;
	char *cursor;
	int32_t len;

	ocrpt_mem_free(result->fetchquery);
	ocrpt_mem_free(result->rewindquery);
	PQclear(result->desc);
	PQclear(result->res);

	len = snprintf(NULL, 0, "CLOSE \"%s\"", query->name);
	cursor = alloca(len + 1);
	len = snprintf(cursor, len + 1, "CLOSE \"%s\"", query->name);
	cursor[len] = 0;

	res = PQexec(conn, cursor);
	PQclear(res);

	ocrpt_mem_free(result);
	ocrpt_query_set_private(query, NULL);
}

static void ocrpt_postgresql_close(const ocrpt_datasource *ds) {
	PGconn *conn = ocrpt_datasource_get_private(ds);

	PQfinish(conn);
}

static const ocrpt_input_connect_parameter *ocrpt_postgresql_connect_methods[] = {
	ocrpt_postgresql_connect_method1,
	ocrpt_postgresql_connect_method2,
	ocrpt_postgresql_connect_method3,
	NULL
};
#endif

static const char *ocrpt_postgresql_input_names[] = { "postgresql", NULL };

const ocrpt_input ocrpt_postgresql_input = {
	.names = ocrpt_postgresql_input_names,
#ifdef HAVE_POSTGRESQL
	.connect_parameters = ocrpt_postgresql_connect_methods,
#endif
	.connect = ocrpt_postgresql_connect,
#ifdef HAVE_POSTGRESQL
	.query_add_sql = ocrpt_postgresql_query_add,
	.describe = ocrpt_postgresql_describe,
	.rewind = ocrpt_postgresql_rewind,
	.next = ocrpt_postgresql_next,
	.populate_result = ocrpt_postgresql_populate_result,
	.isdone = ocrpt_postgresql_isdone,
	.free = ocrpt_postgresql_free,
	.close = ocrpt_postgresql_close
#endif
};

struct ocrpt_mariadb_results {
	ocrpt_query_result *result;
	MYSQL_RES *res;
	int64_t rows;
	int64_t row;
	int32_t cols;
	bool isdone;
};
typedef struct ocrpt_mariadb_results ocrpt_mariadb_results;

static const ocrpt_input_connect_parameter ocrpt_mariadb_connect_method1[] = {
	{ .param_name = "group", { .optional = false } },
	{ .param_name = "optionfile", { .optional = true } },
	{ .param_name = NULL }
};

static const ocrpt_input_connect_parameter ocrpt_mariadb_connect_method2[] = {
	{ .param_name = "dbname", { .optional = false } },
	{ .param_name = "host", { .optional = true } },
	{ .param_name = "port", { .optional = true } },
	{ .param_name = "unix_socket", { .optional = true } },
	{ .param_name = "user", { .optional = true } },
	{ .param_name = "password", { .optional = true } },
	{ .param_name = NULL }
};

static const ocrpt_input_connect_parameter *ocrpt_mariadb_connect_methods[] = {
	ocrpt_mariadb_connect_method1,
	ocrpt_mariadb_connect_method2,
	NULL
};

static bool ocrpt_mariadb_connect(ocrpt_datasource *source, const ocrpt_input_connect_parameter *params) {
	if (!source || !params)
		return false;

	MYSQL *mysql0 = mysql_init(NULL);
	if (mysql0 == NULL)
		return NULL;

	char *dbname = NULL, *host = NULL, *port = NULL, *unix_socket = NULL, *user = NULL, *password = NULL;
	char *optionfile = NULL, *group = NULL;

	for (int32_t i = 0; params[i].param_name; i++) {
		if (strcasecmp(params[i].param_name, "dbname") == 0)
			dbname = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "optionfile") == 0)
			optionfile = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "group") == 0)
			group = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "host") == 0)
			host = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "port") == 0)
			port = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "unix_socket") == 0)
			unix_socket = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "port") == 0)
			port = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "user") == 0)
			user = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "password") == 0)
			password = params[i].param_value;
	}

	MYSQL *mysql = NULL;

	mysql_options(mysql0, MYSQL_READ_DEFAULT_FILE, "/etc/my.cnf");

	if (group) {
		if (optionfile)
			mysql_options(mysql0, MYSQL_READ_DEFAULT_FILE, optionfile);
		mysql_options(mysql0, MYSQL_READ_DEFAULT_GROUP, group);
		mysql_options(mysql0, MYSQL_SET_CHARSET_NAME, "utf8");

		mysql = mysql_real_connect(mysql0, NULL, NULL, NULL, NULL, -1, NULL, 0);
		if (!mysql) {
			mysql_close(mysql0);
			return NULL;
		}
	} else {
		int32_t port_i = -1;
		if (port)
			port_i = atoi(port);

		mysql = mysql_real_connect(mysql0, host, user, password, dbname, port_i, unix_socket, 0);
		if (!mysql) {
			mysql_close(mysql0);
			return NULL;
		}
	}

	ocrpt_datasource_set_private(source, mysql);

	return true;
}

static ocrpt_query_result *ocrpt_mariadb_describe_early(ocrpt_query *query) {
	ocrpt_mariadb_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	opencreport *o = ocrpt_datasource_get_opencreport(source);
	MYSQL *mysql = ocrpt_datasource_get_private(source);
	MYSQL_FIELD *field;
	ocrpt_query_result *qr;
	int32_t i;

	result->rows = mysql_num_rows(result->res);
	result->cols = mysql_field_count(mysql);
	result->row = -1LL;
	result->isdone = false;

	qr = ocrpt_mem_malloc(OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));
	if (!qr)
		return NULL;

	memset(qr, 0, OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));

	for (i = 0, field = mysql_fetch_field(result->res); i < result->cols && field; field = mysql_fetch_field(result->res), i++) {
		enum ocrpt_result_type type;

		switch (field->type) {
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_FLOAT:
		case MYSQL_TYPE_DOUBLE:
		case MYSQL_TYPE_LONGLONG:
		case MYSQL_TYPE_INT24:
		case MYSQL_TYPE_BIT: /* ?? */
		case MYSQL_TYPE_NEWDECIMAL:
			type = OCRPT_RESULT_NUMBER;
			break;
		case MYSQL_TYPE_VARCHAR:
		case MYSQL_TYPE_BLOB:
		case MYSQL_TYPE_VAR_STRING:
		case MYSQL_TYPE_STRING:
			type = OCRPT_RESULT_STRING;
			break;
		case MYSQL_TYPE_DATE:
		case MYSQL_TYPE_TIME:
		case MYSQL_TYPE_DATETIME:
		case MYSQL_TYPE_YEAR:
		case MYSQL_TYPE_NEWDATE:
			type = OCRPT_RESULT_DATETIME;
			break;
		default:
			ocrpt_err_printf("type %d\n", field->type);
			type = OCRPT_RESULT_STRING;
			break;
		}

		for (int j = 0; j < OCRPT_EXPR_RESULTS; j++) {
			int32_t idx = j * result->cols + i;

			if (j == 0) {
				qr[idx].name = ocrpt_mem_strdup(field->name);
				qr[idx].name_allocated = true;
			} else
				qr[idx].name = qr[i].name;

			qr[idx].result.o = o;
			qr[idx].result.type = type;
			qr[idx].result.orig_type = type;

			if (qr[idx].result.type == OCRPT_RESULT_NUMBER) {
				mpfr_init2(qr[idx].result.number, ocrpt_get_numeric_precision_bits(o));
				qr[idx].result.number_initialized = true;
			}

			qr[idx].result.isnull = true;
		}
	}

	return qr;
}

static ocrpt_query *ocrpt_mariadb_query_add(ocrpt_datasource *source, const char *name, const char *querystr) {
	if (!source || !name || !*name || !querystr || !*querystr)
		return NULL;

	MYSQL *mysql = ocrpt_datasource_get_private(source);

	int32_t ret = mysql_query(mysql, querystr);
	if (ret)
		return NULL;

	MYSQL_RES *res = mysql_store_result(mysql);
	if (!res)
		return NULL;

	ocrpt_query *query = ocrpt_query_alloc(source, name);
	if (!query) {
		mysql_free_result(res);
		return NULL;
	}

	struct ocrpt_mariadb_results *result = ocrpt_mem_malloc(sizeof(struct ocrpt_mariadb_results));
	if (!result) {
		mysql_free_result(res);
		ocrpt_query_free(query);
		return NULL;
	}

	memset(result, 0, sizeof(ocrpt_mariadb_results));

	result->res = res;
	ocrpt_query_set_private(query, result);
	result->result = ocrpt_mariadb_describe_early(query);
	if (!result->result) {
		mysql_free_result(res);
		ocrpt_query_free(query);
		return NULL;
	}

	return query;
}

static void ocrpt_mariadb_describe(ocrpt_query *query, ocrpt_query_result **qresult, int32_t *cols) {
	ocrpt_mariadb_results *result = ocrpt_query_get_private(query);

	if (qresult)
		*qresult = result->result;
	if (cols)
		*cols = (result->result ? result->cols : 0);
}

static void ocrpt_mariadb_rewind(ocrpt_query *query) {
	ocrpt_mariadb_results *result = ocrpt_query_get_private(query);
	result->row = -1LL;
	result->isdone = false;
}

static bool ocrpt_mariadb_populate_result(ocrpt_query *query) {
	ocrpt_mariadb_results *result = ocrpt_query_get_private(query);
	MYSQL_ROW row;
	unsigned long *len;
	int32_t i;

	if (result->isdone || result->row < 0LL) {
		ocrpt_query_result_set_values_null(query);
		return false;
	}

	mysql_data_seek(result->res, result->row);
	row = mysql_fetch_row(result->res);
	len = mysql_fetch_lengths(result->res);

	for (i = 0; i < result->cols; i++)
		ocrpt_query_result_set_value(query, i, (row[i] == NULL), (iconv_t)-1, row[i], len[i]);

	return true;
}

static bool ocrpt_mariadb_next(ocrpt_query *query) {
	ocrpt_mariadb_results *result = ocrpt_query_get_private(query);

	if (result->isdone)
		return false;

	result->row++;

	result->isdone = (result->row >= result->rows);
	return ocrpt_mariadb_populate_result(query);
}

static bool ocrpt_mariadb_isdone(ocrpt_query *query) {
	ocrpt_mariadb_results *result = ocrpt_query_get_private(query);

	return result->isdone;
}

static void ocrpt_mariadb_free(ocrpt_query *query) {
	ocrpt_mariadb_results *result = ocrpt_query_get_private(query);

	mysql_free_result(result->res);
	ocrpt_mem_free(result);
}

static void ocrpt_mariadb_close(const ocrpt_datasource *ds) {
	MYSQL *mysql = ocrpt_datasource_get_private(ds);

	mysql_close(mysql);
}

static const char *ocrpt_mariadb_input_names[] = { "mariadb", "mysql", NULL };

const ocrpt_input ocrpt_mariadb_input = {
	.names = ocrpt_mariadb_input_names,
	.connect_parameters = ocrpt_mariadb_connect_methods,
	.connect = ocrpt_mariadb_connect,
	.query_add_sql = ocrpt_mariadb_query_add,
	.describe = ocrpt_mariadb_describe,
	.rewind = ocrpt_mariadb_rewind,
	.next = ocrpt_mariadb_next,
	.populate_result = ocrpt_mariadb_populate_result,
	.isdone = ocrpt_mariadb_isdone,
	.free = ocrpt_mariadb_free,
	.close = ocrpt_mariadb_close
};

struct ocrpt_odbc_private {
	SQLHENV env;
	SQLHDBC dbc;
	iconv_t encoder;
};
typedef struct ocrpt_odbc_private ocrpt_odbc_private;

struct ocrpt_odbc_results {
	ocrpt_query_result *result;
	SQLHSTMT stmt;
	ocrpt_list *rows;
	ocrpt_list *cur_row;
	ocrpt_string *coldata;
	int32_t cols;
	bool atstart:1;
	bool isdone:1;
};
typedef struct ocrpt_odbc_results ocrpt_odbc_results;

static void ocrpt_odbc_print_diag(ocrpt_query *query, const char *stmt, SQLRETURN ret) __attribute__((unused));
static void ocrpt_odbc_print_diag(ocrpt_query *query, const char *stmt, SQLRETURN ret) {
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	ocrpt_odbc_private *priv = ocrpt_datasource_get_private(source);
	SQLINTEGER err;
	SQLSMALLINT mlen;
	char state[6];
	unsigned char buffer[256];
	SQLCHAR	stat[10];
	SQLCHAR	msg[200];

	ocrpt_err_printf("query \"%s\" %s ret %d\n", query->name, stmt, ret);

	SQLError(priv->env, priv->dbc, NULL, (SQLCHAR *)state, NULL, buffer, 256, NULL);
	SQLGetDiagRec(SQL_HANDLE_DBC, priv->dbc, 1, stat, &err, msg, 100, &mlen);

	ocrpt_err_printf("%s result %d: %6.6s \"%s\", %s \"%s\"\n", stmt, ret, state, msg, stat, msg);
}

static const ocrpt_input_connect_parameter ocrpt_odbc_connect_method1[] = {
	{ .param_name = "connstr", { .optional = false } },
	{ .param_name = NULL }
};

static const ocrpt_input_connect_parameter ocrpt_odbc_connect_method2[] = {
	{ .param_name = "dbname", { .optional = false } },
	{ .param_name = "user", { .optional = true } },
	{ .param_name = "password", { .optional = true } },
	{ .param_name = NULL }
};

static ocrpt_odbc_private *ocrpt_odbc_setup(void) {
	ocrpt_odbc_private *priv = ocrpt_mem_malloc(sizeof(ocrpt_odbc_private));
	SQLRETURN ret;

	if (!priv)
		return NULL;

	memset(priv, 0, sizeof(ocrpt_odbc_private));
	priv->encoder = (iconv_t)-1;

	ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &priv->env);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		ocrpt_mem_free(priv);
		return NULL;
	}

	ret = SQLSetEnvAttr(priv->env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		SQLFreeHandle(SQL_HANDLE_ENV, priv->env);
		ocrpt_mem_free(priv);
		return NULL;
	}

#if 0
	/*
	 * We expect UTF-8 strings from the database.
	 * Some database drivers do that implicitly and ignore this call,
	 * some of them comply with this call.
	 * But unfortunately, some of them do not comply and ignore it.
	 * For them, there's ocrpt_datasource_set_encoding().
	 */
	ret = SQLSetEnvAttr(priv->env, SQL_ATTR_APP_UNICODE_TYPE, (void *)SQL_DD_CP_UTF8, SQL_IS_INTEGER);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		SQLFreeHandle(SQL_HANDLE_ENV, priv->env);
		return NULL;
	}

	ret = SQLSetEnvAttr(priv->env, SQL_ATTR_APP_WCHAR_TYPE, (void *)SQL_DD_CP_UTF8, SQL_IS_INTEGER);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		SQLFreeHandle(SQL_HANDLE_ENV, priv->env);
		return NULL;
	}
#endif

	ret = SQLAllocHandle(SQL_HANDLE_DBC, priv->env, &priv->dbc);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		SQLFreeHandle(SQL_HANDLE_ENV, priv->env);
		ocrpt_mem_free(priv);
		return NULL;
	}

	SQLSetConnectAttr(priv->dbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);

	return priv;
}

static bool ocrpt_odbc_connect(ocrpt_datasource *source, const ocrpt_input_connect_parameter *params) {
	if (!source || !params)
		return false;

	char *connstr = NULL, *dbname = NULL, *user = NULL, *password = NULL;

	for (int32_t i = 0; params[i].param_name; i++) {
		if (strcasecmp(params[i].param_name, "connstr") == 0)
			connstr = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "dbname") == 0)
			dbname = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "user") == 0)
			user = params[i].param_value;
		else if (strcasecmp(params[i].param_name, "password") == 0)
			password = params[i].param_value;
	}

	ocrpt_odbc_private *priv = ocrpt_odbc_setup();
	if (!priv) {
		ocrpt_err_printf("ODBC private data setup failed\n");
		return false;
	}

	SQLRETURN ret;

	if (connstr) {
		ret = SQLDriverConnect(priv->dbc, NULL, (SQLCHAR *)connstr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
		if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
			ocrpt_err_printf("SQLDriverConnect failed\n");
			SQLFreeHandle(SQL_HANDLE_DBC, priv->dbc);
			SQLFreeHandle(SQL_HANDLE_ENV, priv->env);
			ocrpt_mem_free(priv);
			return NULL;
		}
	} else {
		ret = SQLConnect(priv->dbc, (SQLCHAR *)dbname, SQL_NTS, (SQLCHAR *)user, SQL_NTS, (SQLCHAR *)password, SQL_NTS);
		if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
			ocrpt_err_printf("SQLConnect failed\n");
			SQLFreeHandle(SQL_HANDLE_DBC, priv->dbc);
			SQLFreeHandle(SQL_HANDLE_ENV, priv->env);
			ocrpt_mem_free(priv);
			return NULL;
		}
	}

	ocrpt_datasource_set_private(source, priv);

	return true;
}

static ocrpt_query_result *ocrpt_odbc_describe_early(ocrpt_query *query) {
	ocrpt_odbc_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	opencreport *o = ocrpt_datasource_get_opencreport(source);
	ocrpt_query_result *qr;
	ocrpt_list *last_row;
	int32_t i;
	SQLSMALLINT cols;
	SQLRETURN ret;

	result->atstart = true;
	result->isdone = false;

	SQLNumResultCols(result->stmt, &cols);
	result->cols = cols;

	qr = ocrpt_mem_malloc(OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));
	if (!qr)
		return NULL;

	memset(qr, 0, OCRPT_EXPR_RESULTS * result->cols * sizeof(ocrpt_query_result));

	for (i = 0; i < result->cols; i++) {
		enum ocrpt_result_type type;
		SQLRETURN ret;
		SQLSMALLINT colname_len;
		SQLSMALLINT col_type;
		SQLULEN col_size;
		ocrpt_string *string;
		int32_t j;

		ret = SQLDescribeCol(result->stmt, i + 1, NULL, 0, &colname_len, NULL, NULL, NULL, NULL);
		if (!SQL_SUCCEEDED(ret))
			continue;

		for (j = 0; j < OCRPT_EXPR_RESULTS; j++) {
			int32_t idx = j * result->cols + i;

			if (j == 0) {
				qr[idx].name = ocrpt_mem_malloc(colname_len + 1);
				qr[idx].name_allocated = true;
			} else
				qr[idx].name = qr[i].name;
		}

		ret = SQLDescribeCol(result->stmt, i + 1, (SQLCHAR *)qr[i].name, colname_len + 1, NULL, &col_type, &col_size, NULL, NULL);
		if (!SQL_SUCCEEDED(ret))
			continue;

		string = ocrpt_mem_string_resize(result->coldata, col_size);
		if (string) {
			if (!result->coldata)
				result->coldata = string;
		}

		switch (col_type) {
		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
		case SQL_BIGINT:
		case SQL_REAL:
		case SQL_FLOAT:
		case SQL_DOUBLE:
		case SQL_DECIMAL:
		case SQL_NUMERIC:
			type = OCRPT_RESULT_NUMBER;
			break;
		case SQL_BIT:
		case SQL_BINARY:
		case SQL_VARBINARY:
		case SQL_LONGVARBINARY:
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_WCHAR:
		case SQL_WVARCHAR:
		case SQL_WLONGVARCHAR:
		case SQL_GUID:
			type = OCRPT_RESULT_STRING;
			break;
		case SQL_DATE:
		case SQL_TYPE_DATE:
		case SQL_TIME:
		case SQL_TYPE_TIME:
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
		case SQL_INTERVAL_MONTH:
		case SQL_INTERVAL_YEAR:
		case SQL_INTERVAL_YEAR_TO_MONTH:
		case SQL_INTERVAL_DAY:
		case SQL_INTERVAL_HOUR:
		case SQL_INTERVAL_MINUTE:
		case SQL_INTERVAL_SECOND:
		case SQL_INTERVAL_DAY_TO_HOUR:
		case SQL_INTERVAL_DAY_TO_MINUTE:
		case SQL_INTERVAL_DAY_TO_SECOND:
		case SQL_INTERVAL_HOUR_TO_MINUTE:
		case SQL_INTERVAL_HOUR_TO_SECOND:
		case SQL_INTERVAL_MINUTE_TO_SECOND:
			type = OCRPT_RESULT_DATETIME;
			break;
		default:
			ocrpt_err_printf("type %d\n", col_type);
			type = OCRPT_RESULT_STRING;
			break;
		}

		for (j = 0; j < OCRPT_EXPR_RESULTS; j++) {
			int32_t idx = j * result->cols + i;

			qr[idx].result.o = o;
			qr[idx].result.type = type;
			qr[idx].result.orig_type = type;

			if (qr[idx].result.type == OCRPT_RESULT_NUMBER) {
				mpfr_init2(qr[idx].result.number, ocrpt_get_numeric_precision_bits(o));
				qr[idx].result.number_initialized = true;
			}

			qr[idx].result.isnull = true;
		}
	}

	result->result = qr;

	while ((ret = SQLFetchScroll(result->stmt, SQL_FETCH_NEXT, 0)) == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
		ocrpt_list *col = NULL, *last_col;

		for (i = 0; i < result->cols; i++) {
			SQLLEN len_or_null;

			ret = SQLGetData(result->stmt, i + 1, SQL_C_CHAR, result->coldata->str, result->coldata->allocated_len, &len_or_null);
			if (ret == SQL_SUCCESS_WITH_INFO || ret == SQL_SUCCESS) {
				ocrpt_string *coldata;

				if (len_or_null == SQL_NULL_DATA)
					coldata = NULL;
				else
					coldata = ocrpt_mem_string_new_with_len(result->coldata->str, len_or_null);
				col = ocrpt_list_end_append(col, &last_col, coldata);
			} else
				ocrpt_odbc_print_diag(query, "SQLGetData", ret);
		}

		result->rows = ocrpt_list_end_append(result->rows, &last_row, col);
	}

	SQLFreeHandle(SQL_HANDLE_STMT, result->stmt);

	return qr;
}

static ocrpt_query *ocrpt_odbc_query_add(ocrpt_datasource *source, const char *name, const char *querystr) {
	if (!source || !name || !*name || !querystr || !*querystr)
		return NULL;

	ocrpt_odbc_private *priv = ocrpt_datasource_get_private(source);

	SQLHSTMT stmt;
	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, priv->dbc, &stmt);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		ocrpt_err_printf("allocation statement handle failed\n");
		return NULL;
	}

	ret = SQLExecDirect(stmt, (SQLCHAR *)querystr, SQL_NTS);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		ocrpt_err_printf("executing query failed: %s\n", querystr);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return NULL;
	}

	ocrpt_query *query = ocrpt_query_alloc(source, name);
	if (!query) {
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return NULL;
	}

	ocrpt_odbc_results *result = ocrpt_mem_malloc(sizeof(struct ocrpt_odbc_results));
	if (!result) {
		ocrpt_query_free(query);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return NULL;
	}

	memset(result, 0, sizeof(ocrpt_odbc_results));

	result->stmt = stmt;
	result->atstart = true;
	ocrpt_query_set_private(query, result);

	result->result = ocrpt_odbc_describe_early(query);

	if (!result->result) {
		ocrpt_query_free(query);
		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
		return NULL;
	}

	return query;
}

static void ocrpt_odbc_describe(ocrpt_query *query, ocrpt_query_result **qresult, int32_t *cols) {
	ocrpt_odbc_results *result = ocrpt_query_get_private(query);

	if (qresult)
		*qresult = result->result;
	if (cols)
		*cols = (result->result ? result->cols : 0);
}

static void ocrpt_odbc_rewind(ocrpt_query *query) {
	ocrpt_odbc_results *result = ocrpt_query_get_private(query);

	result->atstart = true;
	result->isdone = false;
	result->cur_row = NULL;
}

static bool ocrpt_odbc_populate_result(ocrpt_query *query) {
	ocrpt_odbc_results *result = ocrpt_query_get_private(query);
	ocrpt_datasource *source = ocrpt_query_get_source(query);
	ocrpt_odbc_private *priv = ocrpt_datasource_get_private(source);
	int32_t i;
	ocrpt_list *cur_col;

	if (result->atstart || result->isdone) {
		ocrpt_query_result_set_values_null(query);
		return !result->isdone;
	}

	for (i = 0, cur_col = (ocrpt_list *)result->cur_row->data; i < result->cols; i++, cur_col = cur_col->next) {
		ocrpt_string *str = (ocrpt_string *)cur_col->data;

		ocrpt_query_result_set_value(query, i, (str == NULL), priv->encoder, str ? str->str : NULL, str ? str->len : 0);
	}

	return true;
}

static bool ocrpt_odbc_next(ocrpt_query *query) {
	ocrpt_odbc_results *result = ocrpt_query_get_private(query);

	if (result->isdone)
		return false;

	if (result->atstart) {
		result->atstart = false;
		result->cur_row = result->rows;
	} else if (result->cur_row)
		result->cur_row = result->cur_row->next;
	result->isdone = (result->cur_row == NULL);
	return ocrpt_odbc_populate_result(query);
}

static bool ocrpt_odbc_isdone(ocrpt_query *query) {
	ocrpt_odbc_results *result = ocrpt_query_get_private(query);

	return result->isdone;
}

static void ocrpt_odbc_free(ocrpt_query *query) {
	ocrpt_odbc_results *result = ocrpt_query_get_private(query);
	ocrpt_list *row;

	for (row = (ocrpt_list *)result->rows; row; row = row->next) {
		ocrpt_list *col;

		for (col = (ocrpt_list *)row->data; col; col = col->next) {
			ocrpt_string *data = (ocrpt_string *)col->data;

			if (data)
				ocrpt_mem_string_free(data, true);
		}

		ocrpt_list_free((ocrpt_list *)row->data);
	}

	ocrpt_list_free((ocrpt_list *)result->rows);

	ocrpt_mem_string_free(result->coldata, true);
	ocrpt_mem_free(result);
}

static bool ocrpt_odbc_set_encoding(ocrpt_datasource *ds, const char *encoding) {
	ocrpt_odbc_private *priv = ocrpt_datasource_get_private(ds);

	if (priv->encoder != (iconv_t)-1)
		iconv_close(priv->encoder);
	priv->encoder = iconv_open("UTF-8", encoding);
	return (priv->encoder != (iconv_t)-1);
}

static void ocrpt_odbc_close(const ocrpt_datasource *ds) {
	ocrpt_odbc_private *priv = ocrpt_datasource_get_private(ds);

	SQLDisconnect(priv->dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, priv->dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, priv->env);
	if (priv->encoder != (iconv_t)-1)
		iconv_close(priv->encoder);
	ocrpt_mem_free(priv);
	ocrpt_datasource_set_private((ocrpt_datasource *)ds, NULL);
}

static const char *ocrpt_odbc_input_names[] = { "odbc", NULL };

static const ocrpt_input_connect_parameter *ocrpt_odbc_connect_methods[] = {
	ocrpt_odbc_connect_method1,
	ocrpt_odbc_connect_method2,
	NULL
};

const ocrpt_input ocrpt_odbc_input = {
	.names = ocrpt_odbc_input_names,
	.connect_parameters = ocrpt_odbc_connect_methods,
	.connect = ocrpt_odbc_connect,
	.query_add_sql = ocrpt_odbc_query_add,
	.describe = ocrpt_odbc_describe,
	.rewind = ocrpt_odbc_rewind,
	.next = ocrpt_odbc_next,
	.populate_result = ocrpt_odbc_populate_result,
	.isdone = ocrpt_odbc_isdone,
	.free = ocrpt_odbc_free,
	.set_encoding = ocrpt_odbc_set_encoding,
	.close = ocrpt_odbc_close
};
