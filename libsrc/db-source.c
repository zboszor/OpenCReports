/*
 * OpenCReports array data source
 *
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libpq-fe.h>

#include "opencreport.h"
#include "datasource.h"

static void ocrpt_postgresql_close(const ocrpt_datasource *ds) {
	PGconn *conn = ds->priv;

	PQfinish(conn);
}

static const ocrpt_input ocrpt_postgresql_input = {
	.type = OCRPT_INPUT_POSTGRESQL,
#if 0
	.describe = ocrpt_postgresql_describe,
	.rewind = ocrpt_postgresql_rewind,
	.next = ocrpt_postgresql_next,
	.populate_result = ocrpt_postgresql_populate_result,
	.isdone = ocrpt_postgresql_isdone,
	.free = ocrpt_postgresql_free,
#endif
	.close = ocrpt_postgresql_close
};

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_add_postgresql(opencreport *o, const char *source_name,
																const char *host, const char *port, const char *dbname,
																const char *user, const char *password) {
	
	ocrpt_datasource *ds;
	const char *keywords[8] = { "host", "port" , "dbname", "user", "password", "client_encoding", "application_name", NULL };
	const char *values[8] = { host, port, dbname, user, password, "UTF-8", NULL, NULL };
	char *appname;
	int32_t len;

	len = snprintf(NULL, 0, PACKAGE_STRING " datasource %s", source_name);
	appname = alloca(len + 1);
	snprintf(appname, len, PACKAGE_STRING " datasource %s", source_name);
	values[6] = appname;

	PGconn *conn = PQconnectdbParams(keywords, values, 1);
	if (PQstatus(conn) != CONNECTION_OK) {
		PQfinish(conn);
		return NULL;
	}

	ds = ocrpt_datasource_add(o, source_name, &ocrpt_postgresql_input);
	ds->priv = conn;
	return ds;
}

DLL_EXPORT_SYM ocrpt_datasource *ocrpt_datasource_add_postgresql2(opencreport *o, const char *source_name, const char *conninfo) {
	ocrpt_datasource *ds;
	const char *keywords[4] = { "dbname", "client_encoding", "application_name", NULL };
	const char *values[4] = { conninfo, "UTF-8", NULL, NULL };
	char *appname;
	int32_t len;

	len = snprintf(NULL, 0, PACKAGE_STRING " datasource %s", source_name);
	appname = alloca(len + 1);
	snprintf(appname, len, PACKAGE_STRING " datasource %s", source_name);
	values[2] = appname;

	PGconn *conn = PQconnectdbParams(keywords, values, 1);
	if (PQstatus(conn) != CONNECTION_OK) {
		PQfinish(conn);
		return NULL;
	}

	ds = ocrpt_datasource_add(o, source_name, &ocrpt_postgresql_input);
	ds->priv = conn;
	return ds;
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_postgresql(opencreport *o, ocrpt_datasource *source, const char *name, const char *query) {
	return NULL;
}
