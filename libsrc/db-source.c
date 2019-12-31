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

static void ocrpt_postgresql_describe(ocrpt_query *query, ocrpt_query_result **qresult, int32_t *cols) {
	ocrpt_postgresql_results *result = query->priv;

	if (!result->result) {
		PGresult *res;
		ocrpt_query_result *qr;
		int32_t i;

		res = PQdescribePortal(query->source->priv, query->name);
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			PQclear(res);
			if (qresult)
				*qresult = NULL;
			if (cols)
				*cols = 0;
			return;
		}

		result->cols = PQnfields(res);
		qr = ocrpt_mem_malloc(2 * result->cols * sizeof(ocrpt_query_result));

		if (!qr) {
			PQclear(res);
			if (qresult)
				*qresult = NULL;
			if (cols)
				*cols = 0;
			return;
		}

		memset(qr, 0, 2 * result->cols * sizeof(ocrpt_query_result));

		for (i = 0; i < result->cols; i++) {
			enum ocrpt_result_type type;

			qr[i].name = PQfname(res, i);
			qr[result->cols + i].name = PQfname(res, i);

			/* TODO: Handle date/time/interval types */
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
			default:
				fprintf(stderr, "%s:%d: type %d\n", __func__, __LINE__, PQftype(res, i));
				type = OCRPT_RESULT_STRING;
				break;
			}

			qr[i].result.type = type;
			qr[result->cols + i].result.type = type;

			if (qr[i].result.type == OCRPT_RESULT_NUMBER) {
				mpfr_init2(qr[i].result.number, query->source->o->prec);
				qr[i].result.number_initialized = true;
				mpfr_init2(qr[result->cols + i].result.number, query->source->o->prec);
				qr[result->cols + i].result.number_initialized = true;
			}

			qr[i].result.isnull = true;
			qr[result->cols + i].result.isnull = true;
		}

		result->result = qr;
		result->desc = res;
	}

	if (qresult)
		*qresult = result->result;
	if (cols)
		*cols = result->cols;
}

static PGresult *ocrpt_postgresql_fetch(ocrpt_query *query) {
	ocrpt_postgresql_results *result = query->priv;
	PGresult *res;

	if (result->res) {
		PQclear(result->res);
		result->res = NULL;
	}

	res = PQexec(query->source->priv, result->fetchquery);
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
	ocrpt_postgresql_results *result = query->priv;

	if (result->chunk > 0) {
		if (!result->rewindquery) {
			int len = snprintf(NULL, 0, "MOVE ABSOLUTE 0 IN \"%s\"", query->name);

			result->rewindquery = ocrpt_mem_malloc(len + 1);
			if (result->rewindquery) {
				len = snprintf(result->rewindquery, len, "MOVE ABSOLUTE 0 IN \"%s\"", query->name);
				result->rewindquery[len] = 0;
			}
		}

		if (result->rewindquery) {
			PGresult *res = PQexec(query->source->priv, result->rewindquery);
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
	opencreport *o = query->source->o;
	struct ocrpt_postgresql_results *result = query->priv;
	int i, base;

	base = (o->residx ? query->cols: 0);

	if (result->isdone || result->row < 0) {
		for (i = 0; i < query->cols; i++)
			query->result[base + i].result.isnull = true;
		return false;
	}

	for (i = 0; i < query->cols; i++) {
		ocrpt_result *r = &query->result[base + i].result;

		r->isnull = PQgetisnull(result->res, result->row, i);
		if (!r->isnull) {
			const char *str = PQgetvalue(result->res, result->row, i);

			ocrpt_mem_string_free(r->string, r->string_owned);
			r->string = ocrpt_mem_string_new(str, true);
			r->string_owned = true;

			if (r->type == OCRPT_RESULT_NUMBER) {
				if (!r->number_initialized)
					mpfr_init2(r->number, o->prec);
				if (!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "t"))
					str = "1";
				if (!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "f"))
					str = "0";
				mpfr_set_str(r->number, str, 10, o->rndmode);
			}
		} else {
			if (r->number_initialized)
				mpfr_set_ui(r->number, 0, o->rndmode);
		}
	}

	return true;
}

static bool ocrpt_postgresql_next(ocrpt_query *query) {
	ocrpt_postgresql_results *result = query->priv;

	if (result->isdone)
		return false;

	if (result->res == NULL || (result->res != NULL && (result->row == PGFETCHSIZE - 1)))
		ocrpt_postgresql_fetch(query);

	result->row++;

	result->isdone = (result->row < PGFETCHSIZE && result->row == PQntuples(result->res));
	return ocrpt_postgresql_populate_result(query);
}

static bool ocrpt_postgresql_isdone(ocrpt_query *query) {
	ocrpt_postgresql_results *result = query->priv;

	return result->isdone;
}

static void ocrpt_postgresql_free(ocrpt_query *query) {
	ocrpt_postgresql_results *result = query->priv;
	PGresult *res;
	char *cursor;
	int32_t len;

	ocrpt_mem_free(result->fetchquery);
	ocrpt_mem_free(result->rewindquery);
	PQclear(result->desc);
	PQclear(result->res);

	len = snprintf(NULL, 0, "CLOSE \"%s\"", query->name);
	cursor = alloca(len + 1);
	len = snprintf(cursor, len, "CLOSE \"%s\"", query->name);
	cursor[len] = 0;

	res = PQexec(query->source->priv, cursor);
	PQclear(res);

	ocrpt_mem_free(result);
	query->priv = NULL;
}

static void ocrpt_postgresql_close(const ocrpt_datasource *ds) {
	PGconn *conn = ds->priv;

	PQfinish(conn);
}

static const ocrpt_input ocrpt_postgresql_input = {
	.type = OCRPT_INPUT_POSTGRESQL,
	.describe = ocrpt_postgresql_describe,
	.rewind = ocrpt_postgresql_rewind,
	.next = ocrpt_postgresql_next,
	.populate_result = ocrpt_postgresql_populate_result,
	.isdone = ocrpt_postgresql_isdone,
	.free = ocrpt_postgresql_free,
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

DLL_EXPORT_SYM ocrpt_query *ocrpt_query_add_postgresql(opencreport *o, ocrpt_datasource *source, const char *name, const char *querystr) {
	PGresult *res;
	ocrpt_query *query;
	struct ocrpt_postgresql_results *result;
	char *cursor;
	int len;

	if (!ocrpt_datasource_validate(o, source)) {
		fprintf(stderr, "%s:%d: datasource is not for this opencreport structure\n", __func__, __LINE__);
		return NULL;
	}

	if (source->input->type != OCRPT_INPUT_POSTGRESQL) {
		fprintf(stderr, "%s:%d: datasource is not a PostgreSQL source\n", __func__, __LINE__);
		return NULL;
	}

	len = snprintf(NULL, 0, "DECLARE \"%s\" SCROLL CURSOR WITH HOLD FOR %s", name, querystr);
	cursor = alloca(len + 1);
	snprintf(cursor, len, "DECLARE \"%s\" SCROLL CURSOR WITH HOLD FOR %s", name, querystr);
	cursor[len] = 0;

	res = PQexec(source->priv, cursor);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		PQclear(res);
		return NULL;
	}
	PQclear(res);

	query = ocrpt_query_alloc(o, source, name);
	if (!query)
		goto out_error;

	result = ocrpt_mem_malloc(sizeof(struct ocrpt_postgresql_results));
	if (!result) {
		ocrpt_query_free(o, query);
		goto out_error;
	}

	memset(result, 0, sizeof(ocrpt_postgresql_results));

	snprintf(cursor, len, "FETCH %d FROM \"%s\"", PGFETCHSIZE, name);
	result->fetchquery = ocrpt_mem_strdup(cursor);
	result->chunk = -1;
	result->row = -1;
	query->priv = result;

	return query;

	out_error:

	snprintf(cursor, len, "CLOSE \"%s\"", name);
	res = PQexec(source->priv, cursor);
	PQclear(res);
	return NULL;
}
