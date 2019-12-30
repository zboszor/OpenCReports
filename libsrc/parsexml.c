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

static ocrpt_expr *ocrpt_xml_expr_parse(opencreport *o, xmlChar *expr) {
	ocrpt_expr *e;
	char *err;

	if (!expr)
		return NULL;

	e = ocrpt_expr_parse(o, (char *)expr, &err);
	if (e) {
		ocrpt_expr_resolve_exclude(o, e, OCRPT_VARREF_RVAR | OCRPT_VARREF_IDENT | OCRPT_VARREF_VVAR);
		ocrpt_expr_optimize(o, e);
	} else {
		fprintf(stderr, "Cannot parse: %s\n", expr);
		ocrpt_strfree(err);
	}

	return e;
}

static void ocrpt_parse_query_node(opencreport *o, xmlTextReaderPtr reader) {
	xmlChar *name = xmlTextReaderGetAttribute(reader, (const xmlChar *)"name");
	xmlChar *datasource = xmlTextReaderGetAttribute(reader, (const xmlChar *)"datasource");
	xmlChar *follower_for = xmlTextReaderGetAttribute(reader, (const xmlChar *)"follower_for");
	xmlChar *follower_expr = xmlTextReaderGetAttribute(reader, (const xmlChar *)"follower_expr");
	xmlChar *value = xmlTextReaderReadString(reader);
	ocrpt_expr *value_e;
	char *value_s;
	ocrpt_datasource *ds;
	ocrpt_query *q = NULL, *lq = NULL;
	int ret, depth, nodetype;

	/*
	 * A query's value (i.e. an SQL query string) is preferred
	 * to be in the text element but it's accepted as a value="..."
	 * attribute, too.
	 */
	if (!value)
		value = xmlTextReaderGetAttribute(reader, (const xmlChar *)"value");

	value_e = ocrpt_xml_expr_parse(o, value);
	ocrpt_xml_expr_get_value(o, value_e, &value_s, NULL);

	ds = ocrpt_datasource_find(o, (char *)datasource);
	if (ds) {
		switch (ds->input->type) {
			case OCRPT_INPUT_ARRAY: {
				xmlChar *cols = xmlTextReaderGetAttribute(reader, (const xmlChar *)"cols");
				xmlChar *rows = xmlTextReaderGetAttribute(reader, (const xmlChar *)"rows");
				xmlChar *coltypes = xmlTextReaderGetAttribute(reader, (const xmlChar *)"coltypes");
				ocrpt_expr *cols_e, *rows_e, *coltypes_e;
				char *cols_s, *rows_s, *coltypes_s;
				void *arrayptr, *coltypesptr;
				int32_t cols_i, rows_i;

				cols_e = ocrpt_xml_expr_parse(o, cols);
				ocrpt_xml_expr_get_value(o, cols_e, &cols_s, &cols_i);
				if (cols_s && !cols_i)
					cols_i = atoi(cols_s);
				rows_e = ocrpt_xml_expr_parse(o, rows);
				ocrpt_xml_expr_get_value(o, rows_e, &rows_s, &rows_i);
				if (rows_s && !cols_i)
					rows_i = atoi(rows_s);
				coltypes_e = ocrpt_xml_expr_parse(o, coltypes);
				ocrpt_xml_expr_get_value(o, coltypes_e, &coltypes_s, NULL);

				ocrpt_query_discover_array((char *)value, &arrayptr, (char *)coltypes_s, &coltypesptr);
				if (arrayptr)
					q = ocrpt_query_add_array(o, ds, (char *)name, (const char **)arrayptr, rows_i, cols_i, coltypesptr);
				else
					fprintf(stderr, "Cannot determine array pointer for array query\n");

				xmlFree(rows);
				xmlFree(cols);
				xmlFree(coltypes);
				ocrpt_expr_free(cols_e);
				ocrpt_expr_free(rows_e);
				ocrpt_expr_free(coltypes_e);

				break;
			}
			case OCRPT_INPUT_CSV: {
				xmlChar *coltypes = xmlTextReaderGetAttribute(reader, (const xmlChar *)"coltypes");
				ocrpt_expr *coltypes_e;
				char *coltypes_s;
				void *coltypesptr;

				coltypes_e = ocrpt_xml_expr_parse(o, coltypes);
				ocrpt_xml_expr_get_value(o, coltypes_e, &coltypes_s, NULL);

				ocrpt_query_discover_array(NULL, NULL, (char *)coltypes_s, &coltypesptr);
				q = ocrpt_query_add_csv(o, ds, (char *)name, (const char *)value, coltypesptr);

				xmlFree(coltypes);
				ocrpt_expr_free(coltypes_e);
				break;
			}
			case OCRPT_INPUT_JSON: {
				xmlChar *coltypes = xmlTextReaderGetAttribute(reader, (const xmlChar *)"coltypes");
				ocrpt_expr *coltypes_e;
				char *coltypes_s;
				void *coltypesptr;

				coltypes_e = ocrpt_xml_expr_parse(o, coltypes);
				ocrpt_xml_expr_get_value(o, coltypes_e, &coltypes_s, NULL);

				ocrpt_query_discover_array(NULL, NULL, (char *)coltypes, &coltypesptr);
				q = ocrpt_query_add_json(o, ds, (char *)name, (const char *)value, coltypesptr);

				xmlFree(coltypes);
				ocrpt_expr_free(coltypes_e);
				break;
			}
			case OCRPT_INPUT_XML: {
				xmlChar *coltypes = xmlTextReaderGetAttribute(reader, (const xmlChar *)"coltypes");
				ocrpt_expr *coltypes_e;
				char *coltypes_s;
				void *coltypesptr;

				coltypes_e = ocrpt_xml_expr_parse(o, coltypes);
				ocrpt_xml_expr_get_value(o, coltypes_e, &coltypes_s, NULL);
				ocrpt_query_discover_array(NULL, NULL, (char *)coltypes, &coltypesptr);
				q = ocrpt_query_add_xml(o, ds, (char *)name, (const char *)value, coltypesptr);

				xmlFree(coltypes);
				break;
			}
			case OCRPT_INPUT_POSTGRESQL: {
				break;
			}
			default:
				abort();
				break;
		}
	}

	/*
	 * TODO: implement adding XML, PostgreSQL,
	 * MariaDB and ODBC queries
	 */

	if (q) {
		if (follower_for)
			lq = ocrpt_query_find(o, (char *)follower_for);

		if (lq) {
			if (follower_expr) {
				char *err = NULL;

				ocrpt_expr *e = ocrpt_expr_parse(o, (char *)follower_expr, &err);
				if (e) {
					ocrpt_query_add_follower_n_to_1(o, lq, q, e);
				} else
					fprintf(stderr, "Cannot parse matching expression between queries \"%s\" and \"%s\": \"%s\"\n", follower_for, name, follower_expr);
			} else
				ocrpt_query_add_follower(o, lq, q);
		}
	} else
		fprintf(stderr, "cannot add query \"%s\"\n", name);

	xmlFree(name);
	xmlFree(datasource);
	xmlFree(value);
	xmlFree(follower_for);
	xmlFree(follower_expr);

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		processNode(reader);

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
	ocrpt_expr *type_e;
	char *type_s = NULL;
	int ret, depth, nodetype;

	type_e = ocrpt_xml_expr_parse(o, type);
	ocrpt_xml_expr_get_value(o, type_e, &type_s, NULL);

	/*
	 * TODO: implement:
	 * - connecting to PostgreSQL, MariaDB and ODBC datasources
	 */
	if (name && type_s) {
		if (!strcmp((char *)type_s, "array"))
			ocrpt_datasource_add_array(o, (char *)name);
		else if (!strcmp((char *)type_s, "csv"))
			ocrpt_datasource_add_csv(o, (char *)name);
		else if (!strcmp((char *)type_s, "json"))
			ocrpt_datasource_add_json(o, (char *)name);
		else if (!strcmp((char *)type_s, "xml"))
			ocrpt_datasource_add_xml(o, (char *)name);
		else if (!strcmp((char *)type_s, "postgresql")) {
			xmlChar *host = xmlTextReaderGetAttribute(reader, (const xmlChar *)"host");
			xmlChar *port = xmlTextReaderGetAttribute(reader, (const xmlChar *)"port");
			xmlChar *dbname = xmlTextReaderGetAttribute(reader, (const xmlChar *)"dbname");
			xmlChar *user = xmlTextReaderGetAttribute(reader, (const xmlChar *)"user");
			xmlChar	*password = xmlTextReaderGetAttribute(reader, (const xmlChar *)"password");
			xmlChar *connstr = xmlTextReaderGetAttribute(reader, (const xmlChar *)"connstr");
			ocrpt_expr *host_e, *port_e, *dbname_e, *user_e, *password_e, *connstr_e;
			char *host_s, *port_s, *dbname_s, *user_s, *password_s, *connstr_s;
			int32_t port_i;

			host_e = ocrpt_xml_expr_parse(o, host);
			ocrpt_xml_expr_get_value(o, host_e, &host_s, NULL);
			port_e = ocrpt_xml_expr_parse(o, port);
			ocrpt_xml_expr_get_value(o, port_e, &port_s, &port_i);
			if (!port_s && port_i > 0) {
				port_s = alloca(32);
				sprintf(port_s, "%d", port_i);
			}
			dbname_e = ocrpt_xml_expr_parse(o, dbname);
			ocrpt_xml_expr_get_value(o, dbname_e, &dbname_s, NULL);
			user_e = ocrpt_xml_expr_parse(o, user);
			ocrpt_xml_expr_get_value(o, user_e, &user_s, NULL);
			password_e = ocrpt_xml_expr_parse(o, password);
			ocrpt_xml_expr_get_value(o, password_e, &password_s, NULL);
			connstr_e = ocrpt_xml_expr_parse(o, connstr);
			ocrpt_xml_expr_get_value(o, connstr_e, &connstr_s, NULL);

			if (connstr_s)
				ocrpt_datasource_add_postgresql2(o, (char *)name, connstr_s);
			else
				ocrpt_datasource_add_postgresql(o, (char *)name, host_s, port_s, dbname_s, user_s, password_s);

			xmlFree(host);
			xmlFree(port);
			xmlFree(dbname);
			xmlFree(user);
			xmlFree(password);
			xmlFree(connstr);
			ocrpt_expr_free(host_e);
			ocrpt_expr_free(port_e);
			ocrpt_expr_free(dbname_e);
			ocrpt_expr_free(user_e);
			ocrpt_expr_free(password_e);
			ocrpt_expr_free(connstr_e);
		}
	}

	ocrpt_expr_free(type_e);
	xmlFree(name);
	xmlFree(type);

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
