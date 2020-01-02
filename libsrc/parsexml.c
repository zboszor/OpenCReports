/*
 * OpenCReports main header
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <alloca.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>

#include "opencreport-private.h"
#include "datasource.h"

#ifndef O_BINARY
#define O_BINARY (0)
#endif

void ocrpt_free_part(const struct opencreport_part *part) {
	ocrpt_mem_free(part->path);
	if (part->allocated)
		ocrpt_mem_free(part->xmlbuf);
	ocrpt_mem_free(part);
}

int32_t ocrpt_add_report_from_buffer_internal(opencreport *o, const char *buffer, bool allocated, const char *report_path) {
	struct opencreport_part *part = ocrpt_mem_malloc(sizeof(struct opencreport_part));
	int partsold;

	if (!part)
		return -1;

	part->xmlbuf = buffer;
	part->allocated = allocated;
	part->parsed = 0;
	part->path = ocrpt_mem_strdup(report_path);
	if (!part->path) {
		ocrpt_free_part(part);
		return -1;
	}

	partsold = ocrpt_list_length(o->parts);
	o->parts = ocrpt_list_append(o->parts, part);
	if (ocrpt_list_length(o->parts) != partsold + 1) {
		ocrpt_free_part(part);
		return -1;
	}

	return 0;
}

int32_t ocrpt_add_report_from_buffer(opencreport *o, const char *buffer) {
	return ocrpt_add_report_from_buffer_internal(o, buffer, 0, cwdpath);
}

int32_t ocrpt_add_report(opencreport *o, const char *filename) {
	struct stat st;
	char *str, *fnamecopy, *dir;
	int fd;
	ssize_t len, orig;

	if (!filename)
		return -1;

	if (stat(filename, &st) == -1)
		return -1;

	fd = open(filename, O_RDONLY | O_BINARY);
	if (fd < 0)
		return -1;

	str = ocrpt_mem_malloc(st.st_size + 1);
	if (!str) {
		close(fd);
		return -1;
	}

	orig = st.st_size;
	do {
		len = read(fd, str, orig);
		if (len < 0)
			break;
		orig -= len;
	} while (orig > 0);

	close(fd);

	if (len < 0) {
		ocrpt_mem_free(str);
		return -1;
	}

	fnamecopy = ocrpt_mem_strdup(filename);
	dir = dirname(fnamecopy);
	ocrpt_mem_free(fnamecopy);

	return ocrpt_add_report_from_buffer_internal(o, str, 1, dir);
}

void ocrpt_free_parts(opencreport *o) {
	ocrpt_list_free_deep(o->parts, (ocrpt_mem_free_t)ocrpt_free_part);
	o->parts = NULL;
}

static void processNode(xmlTextReaderPtr reader) __attribute__((unused));
static void processNode(xmlTextReaderPtr reader) {
	xmlChar *name, *value;

	name = xmlTextReaderName(reader);
	if (name == NULL)
		name = xmlStrdup(BAD_CAST "--");
	value = xmlTextReaderValue(reader);

	printf("%d %d %s %d",
			xmlTextReaderDepth(reader),
			xmlTextReaderNodeType(reader),
			name,
			xmlTextReaderIsEmptyElement(reader));
	xmlFree(name);
	if (value == NULL)
		printf("\n");
	else {
		printf(" %s\n", value);
		xmlFree(value);
	}
}

static void ocrpt_xml_expr_get_value(opencreport *o, ocrpt_expr *e, char **s, int32_t *i) {
	ocrpt_result *r;

	if (s)
		*s = NULL;
	if (i)
		*i = 0;
	if (!e)
		return;

	r = ocrpt_expr_eval(o, e);
	if (r) {
		if (s && r->type == OCRPT_RESULT_STRING)
			*s = r->string->str;
		if (i && OCRPT_RESULT_NUMBER)
			*i = mpfr_get_si(r->number, o->rndmode);
	}
}

static ocrpt_expr *ocrpt_xml_expr_parse(opencreport *o, xmlChar *expr, bool report) {
	ocrpt_expr *e;
	char *err;

	if (!expr)
		return NULL;

	e = ocrpt_expr_parse(o, (char *)expr, &err);
	if (e) {
		ocrpt_expr_resolve_exclude(o, e, OCRPT_VARREF_RVAR | OCRPT_VARREF_IDENT | OCRPT_VARREF_VVAR);
		ocrpt_expr_optimize(o, e);
	} else {
		if (report)
			fprintf(stderr, "Cannot parse: %s\n", expr);
		ocrpt_strfree(err);
	}

	return e;
}

#define ocrpt_xml_expr_parse_get_value_with_fallback(o, expr) { \
					expr##_e = ocrpt_xml_expr_parse(o, expr, true); \
					ocrpt_xml_expr_get_value(o, expr##_e, &expr##_s, NULL); \
					if (!expr##_s) \
						expr##_s = (char *)expr; \
				}

#define ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, expr) { \
					expr##_e = ocrpt_xml_expr_parse(o, expr, false); \
					ocrpt_xml_expr_get_value(o, expr##_e, &expr##_s, NULL); \
					if (!expr##_s) \
						expr##_s = (char *)expr; \
				}

static void ocrpt_parse_query_node(opencreport *o, xmlTextReaderPtr reader) {
	xmlChar *name = xmlTextReaderGetAttribute(reader, (const xmlChar *)"name");
	xmlChar *value_att = xmlTextReaderGetAttribute(reader, (const xmlChar *)"value");
	xmlChar *value = NULL;
	xmlChar *datasource = xmlTextReaderGetAttribute(reader, (const xmlChar *)"datasource");
	xmlChar *follower_for = xmlTextReaderGetAttribute(reader, (const xmlChar *)"follower_for");
	xmlChar *follower_expr = xmlTextReaderGetAttribute(reader, (const xmlChar *)"follower_expr");
	xmlChar *cols = xmlTextReaderGetAttribute(reader, (const xmlChar *)"cols");
	xmlChar *rows = xmlTextReaderGetAttribute(reader, (const xmlChar *)"rows");
	xmlChar *coltypes = xmlTextReaderGetAttribute(reader, (const xmlChar *)"coltypes");
	ocrpt_expr *name_e, *value_e, *datasource_e, *follower_for_e, *follower_expr_e, *cols_e, *rows_e, *coltypes_e;
	char *name_s, *value_s, *datasource_s, *follower_for_s, *follower_expr_s, *cols_s, *rows_s, *coltypes_s;
	int32_t cols_i, rows_i;
	void *arrayptr, *coltypesptr;
	ocrpt_datasource *ds;
	ocrpt_query *q = NULL, *lq = NULL;
	int ret, depth, nodetype;

	depth = xmlTextReaderDepth(reader);

	if (!xmlTextReaderIsEmptyElement(reader)) {
		ret = xmlTextReaderRead(reader);
		if (ret == 1) {
			nodetype = xmlTextReaderNodeType(reader);
			if (nodetype == XML_READER_TYPE_TEXT && depth == (xmlTextReaderDepth(reader) - 1))
				value = xmlTextReaderReadString(reader);
		}
	}

	/*
	 * A query's value (i.e. an SQL query string) is preferred
	 * to be in the text element but it's accepted as a value="..."
	 * attribute, too.
	 */
	if (!value)
		value = value_att;

	ocrpt_xml_expr_parse_get_value_with_fallback(o, name);
	ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, value);
	ocrpt_xml_expr_parse_get_value_with_fallback(o, datasource);
	ocrpt_xml_expr_parse_get_value_with_fallback(o, follower_for);
	ocrpt_xml_expr_parse_get_value_with_fallback(o, follower_expr);

	cols_e = ocrpt_xml_expr_parse(o, cols, true);
	ocrpt_xml_expr_get_value(o, cols_e, &cols_s, &cols_i);
	if (cols_s && !cols_i)
		cols_i = atoi(cols_s);

	rows_e = ocrpt_xml_expr_parse(o, rows, true);
	ocrpt_xml_expr_get_value(o, rows_e, &rows_s, &rows_i);
	if (rows_s && !cols_i)
		rows_i = atoi(rows_s);

	ocrpt_xml_expr_parse_get_value_with_fallback(o, coltypes);

	ds = ocrpt_datasource_find(o, datasource_s);
	if (ds) {
		switch (ds->input->type) {
		case OCRPT_INPUT_ARRAY:
			ocrpt_query_discover_array(value_s, &arrayptr, coltypes_s, &coltypesptr);
			if (arrayptr)
				q = ocrpt_query_add_array(o, ds, name_s, arrayptr, rows_i, cols_i, coltypesptr);
			else
				fprintf(stderr, "Cannot determine array pointer for array query\n");
			break;
		case OCRPT_INPUT_CSV:
			ocrpt_query_discover_array(NULL, NULL, coltypes_s, &coltypesptr);
			q = ocrpt_query_add_csv(o, ds, name_s, value_s, coltypesptr);
			break;
		case OCRPT_INPUT_JSON:
			ocrpt_query_discover_array(NULL, NULL, coltypes_s, &coltypesptr);
			q = ocrpt_query_add_json(o, ds, name_s, value_s, coltypesptr);
			break;
		case OCRPT_INPUT_XML:
			ocrpt_query_discover_array(NULL, NULL, coltypes_s, &coltypesptr);
			q = ocrpt_query_add_xml(o, ds, name_s, value_s, coltypesptr);
			break;
		case OCRPT_INPUT_POSTGRESQL:
			q = ocrpt_query_add_postgresql(o, ds, name_s, value_s);
			break;
		case OCRPT_INPUT_MARIADB:
			q = ocrpt_query_add_mariadb(o, ds, name_s, value_s);
			break;
		default:
			abort();
			break;
		}
	}

	/*
	 * TODO: implement adding MariaDB and ODBC queries
	 */

	if (q) {
		if (follower_for)
			lq = ocrpt_query_find(o, follower_for_s);

		if (lq) {
			if (follower_expr) {
				char *err = NULL;
				ocrpt_expr *e = ocrpt_expr_parse(o, follower_expr_s, &err);

				if (e) {
					ocrpt_query_add_follower_n_to_1(o, lq, q, e);
				} else
					fprintf(stderr, "Cannot parse matching expression between queries \"%s\" and \"%s\": \"%s\"\n", follower_for_s, name_s, follower_expr);
			} else
				ocrpt_query_add_follower(o, lq, q);
		}
	} else
		fprintf(stderr, "cannot add query \"%s\"\n", name_s);

	xmlFree(name);
	xmlFree(datasource);
	xmlFree(value_att);
	if (value != value_att)
		xmlFree(value);
	xmlFree(follower_for);
	xmlFree(follower_expr);
	xmlFree(rows);
	xmlFree(cols);
	xmlFree(coltypes);
	ocrpt_expr_free(name_e);
	ocrpt_expr_free(datasource_e);
	ocrpt_expr_free(value_e);
	ocrpt_expr_free(follower_for_e);
	ocrpt_expr_free(follower_expr_e);
	ocrpt_expr_free(cols_e);
	ocrpt_expr_free(rows_e);
	ocrpt_expr_free(coltypes_e);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Query")) {
			xmlFree(name);
			break;
		}

		xmlFree(name);

		ret = xmlTextReaderRead(reader);
	}
}

static void ocrpt_parse_queries_node(opencreport *o, xmlTextReaderPtr reader) {
	int ret, depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Queries")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Query"))
				ocrpt_parse_query_node(o, reader);
		}

		xmlFree(name);

		ret = xmlTextReaderRead(reader);
	}
}

static void ocrpt_parse_datasource_node(opencreport *o, xmlTextReaderPtr reader) {
	xmlChar *name = xmlTextReaderGetAttribute(reader, (const xmlChar *)"name");
	xmlChar *type = xmlTextReaderGetAttribute(reader, (const xmlChar *)"type");
	xmlChar *host = xmlTextReaderGetAttribute(reader, (const xmlChar *)"host");
	xmlChar *unix_socket = xmlTextReaderGetAttribute(reader, (const xmlChar *)"unix_socket");
	xmlChar *port = xmlTextReaderGetAttribute(reader, (const xmlChar *)"port");
	xmlChar *dbname = xmlTextReaderGetAttribute(reader, (const xmlChar *)"dbname");
	xmlChar *user = xmlTextReaderGetAttribute(reader, (const xmlChar *)"user");
	xmlChar	*password = xmlTextReaderGetAttribute(reader, (const xmlChar *)"password");
	xmlChar *connstr = xmlTextReaderGetAttribute(reader, (const xmlChar *)"connstr");
	xmlChar *optionfile = xmlTextReaderGetAttribute(reader, (const xmlChar *)"optionfile");
	xmlChar *group = xmlTextReaderGetAttribute(reader, (const xmlChar *)"group");
	ocrpt_expr *name_e, *type_e, *host_e, *unix_socket_e, *port_e, *dbname_e, *user_e, *password_e, *connstr_e, *optionfile_e, *group_e;
	char *name_s, *type_s, *host_s, *unix_socket_s, *port_s, *dbname_s, *user_s, *password_s, *connstr_s, *optionfile_s, *group_s;
	int32_t port_i;
	int ret, depth, nodetype;

	ocrpt_xml_expr_parse_get_value_with_fallback(o, name);
	ocrpt_xml_expr_parse_get_value_with_fallback(o, type);

	ocrpt_xml_expr_parse_get_value_with_fallback(o, host);
	ocrpt_xml_expr_parse_get_value_with_fallback(o, unix_socket);

	port_e = ocrpt_xml_expr_parse(o, port, true);
	ocrpt_xml_expr_get_value(o, port_e, &port_s, &port_i);
	if (!port_s && port_i > 0) {
		port_s = alloca(32);
		sprintf(port_s, "%d", port_i);
	}

	/*
	 * Don't report parse errors for database connection details,
	 * they are sensitive information.
	 */
	ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, dbname);
	ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, user);
	ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, password);
	ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, connstr);
	ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, optionfile);
	ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, group);

	/*
	 * TODO: implement:
	 * - connecting to PostgreSQL, MariaDB and ODBC datasources
	 */
	if (name_s && type_s) {
		if (!strcmp(type_s, "array"))
			ocrpt_datasource_add_array(o, name_s);
		else if (!strcmp(type_s, "csv"))
			ocrpt_datasource_add_csv(o, name_s);
		else if (!strcmp(type_s, "json"))
			ocrpt_datasource_add_json(o, name_s);
		else if (!strcmp(type_s, "xml"))
			ocrpt_datasource_add_xml(o, name_s);
		else if (!strcmp(type_s, "postgresql")) {
			if (connstr_s)
				ocrpt_datasource_add_postgresql2(o, name_s, connstr_s);
			else
				ocrpt_datasource_add_postgresql(o, name_s, unix_socket_s ? unix_socket_s : host_s, port_s, dbname_s, user_s, password_s);
		} else if (!strcmp(type_s, "mariadb") || !strcmp(type_s, "mysql")) {
			if (group_s)
				ocrpt_datasource_add_mariadb2(o, name_s, optionfile_s, group_s);
			else
				ocrpt_datasource_add_mariadb(o, name_s, host_s, port_s, dbname_s, user_s, password_s, unix_socket_s);
		}
	}

	xmlFree(name);
	xmlFree(type);
	xmlFree(host);
	xmlFree(unix_socket);
	xmlFree(port);
	xmlFree(dbname);
	xmlFree(user);
	xmlFree(password);
	xmlFree(connstr);
	xmlFree(optionfile);
	xmlFree(group);
	ocrpt_expr_free(name_e);
	ocrpt_expr_free(type_e);
	ocrpt_expr_free(host_e);
	ocrpt_expr_free(unix_socket_e);
	ocrpt_expr_free(port_e);
	ocrpt_expr_free(dbname_e);
	ocrpt_expr_free(user_e);
	ocrpt_expr_free(password_e);
	ocrpt_expr_free(connstr_e);
	ocrpt_expr_free(optionfile_e);
	ocrpt_expr_free(group_e);

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Datasource")) {
			xmlFree(name);
			break;
		}

		xmlFree(name);

		ret = xmlTextReaderRead(reader);
	}
}

static void ocrpt_parse_datasources_node(opencreport *o, xmlTextReaderPtr reader) {
	int ret, depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Datasources")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Datasource"))
				ocrpt_parse_datasource_node(o, reader);
		}

		xmlFree(name);

		ret = xmlTextReaderRead(reader);
	}
}

static void ocrpt_parse_opencreport_node(opencreport *o, xmlTextReaderPtr reader) {
	int ret, depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "OpenCReport")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Datasources"))
				ocrpt_parse_datasources_node(o, reader);
			else if (!strcmp((char *)name, "Queries"))
				ocrpt_parse_queries_node(o, reader);
#if 0
			else if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, reader);
			else if (!strcmp((char *)name, "Part"))
				ocrpt_parse_part_node(o, reader);
#endif
		}

		xmlFree(name);

		ret = xmlTextReaderRead(reader);
	}
}

DLL_EXPORT_SYM bool ocrpt_parse_xml(opencreport *o, const char *filename) {
	xmlTextReaderPtr reader;
	bool retval = true;
	int ret;

	reader = xmlReaderForFile(filename, NULL, XML_PARSE_RECOVER |
								XML_PARSE_NOENT | XML_PARSE_NOBLANKS |
								XML_PARSE_XINCLUDE | XML_PARSE_NOXINCNODE);

	if (!reader)
		return false;

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name = xmlTextReaderName(reader);
		int nodeType = xmlTextReaderNodeType(reader);
		int depth = xmlTextReaderDepth(reader);

		//processNode(reader);

		if (nodeType == XML_READER_TYPE_DOCUMENT_TYPE) {
			/* ignore - xmllint validation is enough */
		} else if (nodeType == XML_READER_TYPE_ELEMENT && depth == 0) {
			if (!strcmp((char *)name, "OpenCReport"))
				ocrpt_parse_opencreport_node(o, reader);
			else if (!strcmp((char *)name, "Datasources"))
				ocrpt_parse_datasources_node(o, reader);
			else if (!strcmp((char *)name, "Datasource"))
				ocrpt_parse_datasource_node(o, reader);
			else if (!strcmp((char *)name, "Queries"))
				ocrpt_parse_queries_node(o, reader);
			else if (!strcmp((char *)name, "Query"))
				ocrpt_parse_query_node(o, reader);
#if 0
			else if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, reader);
			else if (!strcmp((char *)name, "Part"))
				ocrpt_parse_part_node(o, reader);
#endif
		}

		xmlFree(name);

		if (ret == 1)
			ret = xmlTextReaderRead(reader);
	}

	xmlFreeTextReader(reader);

	return retval;
}
