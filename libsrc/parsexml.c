/*
 * OpenCReports main header
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
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

#include "opencreport.h"
#include "datasource.h"
#include "exprutil.h"

#ifndef O_BINARY
#define O_BINARY (0)
#endif

extern char cwdpath[PATH_MAX];

int32_t ocrpt_add_report_from_buffer_internal(opencreport *o, const char *buffer, bool allocated, const char *report_path) {
	struct ocrpt_part *part = ocrpt_mem_malloc(sizeof(struct ocrpt_part));
	int partsold;

	if (!part)
		return -1;

#if 0
	part->xmlbuf = buffer;
	part->allocated = allocated;
	part->parsed = 0;
#endif
	part->path = ocrpt_mem_strdup(report_path);
	if (!part->path) {
		ocrpt_part_free(o, part);
		return -1;
	}

	partsold = ocrpt_list_length(o->parts);
	o->parts = ocrpt_list_append(o->parts, part);
	if (ocrpt_list_length(o->parts) != partsold + 1) {
		ocrpt_part_free(o, part);
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

static void processNode(xmlTextReaderPtr reader) __attribute__((unused));
static void processNode(xmlTextReaderPtr reader) {
	xmlChar *name, *value;

	name = xmlTextReaderName(reader);
	if (name == NULL)
		name = xmlStrdup(BAD_CAST "--");
	value = xmlTextReaderValue(reader);

	fprintf(stderr, "%d %d %s %d",
			xmlTextReaderDepth(reader),
			xmlTextReaderNodeType(reader),
			name,
			xmlTextReaderIsEmptyElement(reader));
	xmlFree(name);
	if (value == NULL)
		fprintf(stderr, "\n");
	else {
		fprintf(stderr, " %s\n", value);
		xmlFree(value);
	}
}

static ocrpt_expr *ocrpt_xml_expr_parse(opencreport *o, ocrpt_report *r, xmlChar *expr, bool report, bool create_string) {
	ocrpt_expr *e;
	char *err = NULL;

	if (!expr)
		return NULL;

	e = ocrpt_expr_parse(o, r, (char *)expr, &err);

	if (!e) {
		if (report)
			fprintf(stderr, "Cannot parse: %s\n", expr);

		if (create_string)
			e = ocrpt_newstring(o, r, (char *)expr);
	}
	ocrpt_strfree(err);

	return e;
}

static ocrpt_expr *ocrpt_xml_const_expr_parse(opencreport *o, xmlChar *expr, bool fake_vars_expected, bool report) {
	ocrpt_expr *e;
	char *err;

	if (!expr)
		return NULL;

	e = ocrpt_expr_parse(o, NULL, (char *)expr, &err);
	if (e) {
		if (fake_vars_expected) {
			ocrpt_expr_resolve_exclude(o, NULL, e, OCRPT_VARREF_RVAR | OCRPT_VARREF_IDENT | OCRPT_VARREF_VVAR);
			ocrpt_expr_optimize(o, NULL, e);
		} else {
			uint32_t var_mask;
			if (ocrpt_expr_references(o, NULL, e, OCRPT_VARREF_RVAR | OCRPT_VARREF_IDENT | OCRPT_VARREF_VVAR, &var_mask)) {
				char vartypes[64] = "";

				if ((var_mask & OCRPT_VARREF_RVAR))
					strcat(vartypes, "RVAR");
				if ((var_mask & OCRPT_VARREF_IDENT)) {
					if (*vartypes)
						strcat(vartypes, " ");
					strcat(vartypes, "IDENT");
				}
				if ((var_mask & OCRPT_VARREF_VVAR)) {
					if (*vartypes)
						strcat(vartypes, " ");
					strcat(vartypes, "VVAR");
				}

				fprintf(stderr, "constant expression expected, %s references found: %s\n", vartypes, (char *)expr);
				ocrpt_expr_free(o, NULL, e);
				e = NULL;
			} else
				ocrpt_expr_optimize(o, NULL, e);
		}
	} else {
		if (report)
			fprintf(stderr, "Cannot parse: %s\n", expr);
		ocrpt_strfree(err);
	}

	return e;
}

#define ocrpt_xml_const_expr_parse_get_value_with_fallback(o, expr) { \
					expr##_e = ocrpt_xml_const_expr_parse(o, expr, true, true); \
					expr##_s = (char *)ocrpt_expr_get_string_value(o, expr##_e); \
					if (!expr##_s) \
						expr##_s = (char *)expr; \
				}

#define ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, expr) { \
					expr##_e = ocrpt_xml_const_expr_parse(o, expr, true, false); \
					expr##_s = (char *)ocrpt_expr_get_string_value(o, expr##_e); \
					if (!expr##_s) \
						expr##_s = (char *)expr; \
				}

#define ocrpt_xml_const_expr_parse_get_int_value_with_fallback(o, expr) { \
					expr##_e = ocrpt_xml_const_expr_parse(o, expr, false, true); \
					expr##_i = ocrpt_expr_get_long_value(o, expr##_e); \
				}

#define ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, expr) { \
					expr##_e = ocrpt_xml_const_expr_parse(o, expr, false, false); \
					expr##_i = ocrpt_expr_get_long_value(o, expr##_e); \
				}

#define ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, expr) { \
					expr##_e = ocrpt_xml_const_expr_parse(o, expr, false, true); \
					expr##_d = ocrpt_expr_get_double_value(o, expr##_e); \
				}

#define ocrpt_xml_const_expr_parse_get_double_value_with_fallback_noreport(o, expr) { \
					expr##_e = ocrpt_xml_const_expr_parse(o, expr, false, false); \
					expr##_d = ocrpt_expr_get_double_value(o, expr##_e); \
				}

static void ocrpt_ignore_child_nodes(opencreport *o, xmlTextReaderPtr reader, int depth, const char *leaf_name) {
	int nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	if (depth < 0)
		depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, leaf_name)) {
			xmlFree(name);
			break;
		}

		xmlFree(name);
	}
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
	char *name_s, *value_s, *datasource_s, *follower_for_s, *follower_expr_s, *coltypes_s;
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

	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, name);
	ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, value);
	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, datasource);
	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, follower_for);
	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, follower_expr);

	ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, cols);
	ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, rows);

	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, coltypes);

	ds = ocrpt_datasource_get(o, datasource_s);
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
		case OCRPT_INPUT_ODBC:
			q = ocrpt_query_add_odbc(o, ds, name_s, value_s);
			break;
		default:
			break;
		}
	}

	if (q) {
		if (follower_for)
			lq = ocrpt_query_get(o, follower_for_s);

		if (lq) {
			if (follower_expr) {
				char *err = NULL;
				ocrpt_expr *e = ocrpt_expr_parse(o, NULL, follower_expr_s, &err);

				if (e)
					ocrpt_query_add_follower_n_to_1(o, lq, q, e);
				else
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
	ocrpt_expr_free(o, NULL, name_e);
	ocrpt_expr_free(o, NULL, datasource_e);
	ocrpt_expr_free(o, NULL, value_e);
	ocrpt_expr_free(o, NULL, follower_for_e);
	ocrpt_expr_free(o, NULL, follower_expr_e);
	ocrpt_expr_free(o, NULL, cols_e);
	ocrpt_expr_free(o, NULL, rows_e);
	ocrpt_expr_free(o, NULL, coltypes_e);
}

static void ocrpt_parse_queries_node(opencreport *o, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Queries")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Query"))
				ocrpt_parse_query_node(o, reader);
		}

		xmlFree(name);
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

	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, name);
	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, type);

	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, host);
	ocrpt_xml_const_expr_parse_get_value_with_fallback(o, unix_socket);

	port_s = NULL;
	ocrpt_xml_const_expr_parse_get_int_value_with_fallback(o, port);
	if (port_i) {
		port_s = alloca(32);
		sprintf(port_s, "%d", port_i);
	}

	/*
	 * Don't report parse errors for database connection details,
	 * they are sensitive information.
	 */
	ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, dbname);
	ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, user);
	ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, password);
	ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, connstr);
	ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, optionfile);
	ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, group);

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
		} else if (!strcmp(type_s, "odbc")) {
			if (connstr_s)
				ocrpt_datasource_add_odbc2(o, name_s, connstr_s);
			else
				ocrpt_datasource_add_odbc(o, name_s, dbname_s, user_s, password_s);
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
	ocrpt_expr_free(o, NULL, name_e);
	ocrpt_expr_free(o, NULL, type_e);
	ocrpt_expr_free(o, NULL, host_e);
	ocrpt_expr_free(o, NULL, unix_socket_e);
	ocrpt_expr_free(o, NULL, port_e);
	ocrpt_expr_free(o, NULL, dbname_e);
	ocrpt_expr_free(o, NULL, user_e);
	ocrpt_expr_free(o, NULL, password_e);
	ocrpt_expr_free(o, NULL, connstr_e);
	ocrpt_expr_free(o, NULL, optionfile_e);
	ocrpt_expr_free(o, NULL, group_e);

	ocrpt_ignore_child_nodes(o, reader, -1, "Datasource");
}

static void ocrpt_parse_datasources_node(opencreport *o, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Datasources")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Datasource"))
				ocrpt_parse_datasource_node(o, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_variable_node(opencreport *o, ocrpt_report *r, xmlTextReaderPtr reader) {
	xmlChar *name = xmlTextReaderGetAttribute(reader, (const xmlChar *)"name");
	xmlChar *value = xmlTextReaderGetAttribute(reader, (const xmlChar *)"value");
	xmlChar *baseexpr = xmlTextReaderGetAttribute(reader, (const xmlChar *)"baseexpr");
	xmlChar *intermedexpr = xmlTextReaderGetAttribute(reader, (const xmlChar *)"intermedexpr");
	xmlChar *intermed2expr = xmlTextReaderGetAttribute(reader, (const xmlChar *)"intermedexpr");
	xmlChar *resultexpr = xmlTextReaderGetAttribute(reader, (const xmlChar *)"resultexpr");
	xmlChar *type = xmlTextReaderGetAttribute(reader, (const xmlChar *)"type");
	xmlChar *basetype = xmlTextReaderGetAttribute(reader, (const xmlChar *)"basetype");
	xmlChar *resetonbreak = xmlTextReaderGetAttribute(reader, (const xmlChar *)"resetonbreak");
	xmlChar *precalculate = xmlTextReaderGetAttribute(reader, (const xmlChar *)"precalculate");
	xmlChar *delayed = xmlTextReaderGetAttribute(reader, (const xmlChar *)"delayed");
	ocrpt_var_type vtype = OCRPT_VARIABLE_EXPRESSION; /* default if left out */
	enum ocrpt_result_type rtype = OCRPT_RESULT_NUMBER;

	if (type) {
		if (strcasecmp((char *)type, "count") == 0)
			vtype = OCRPT_VARIABLE_COUNT;
		else if (strcasecmp((char *)type, "countall") == 0)
			vtype = OCRPT_VARIABLE_COUNTALL;
		else if (strcasecmp((char *)type, "expression") == 0)
			vtype = OCRPT_VARIABLE_EXPRESSION;
		else if (strcasecmp((char *)type, "sum") == 0)
			vtype = OCRPT_VARIABLE_SUM;
		else if (strcasecmp((char *)type, "average") == 0)
			vtype = OCRPT_VARIABLE_AVERAGE;
		else if (strcasecmp((char *)type, "averageall") == 0)
			vtype = OCRPT_VARIABLE_AVERAGEALL;
		else if (strcasecmp((char *)type, "lowest") == 0)
			vtype = OCRPT_VARIABLE_LOWEST;
		else if (strcasecmp((char *)type, "highest") == 0)
			vtype = OCRPT_VARIABLE_HIGHEST;
		else if (strcasecmp((char *)type, "custom") == 0)
			vtype = OCRPT_VARIABLE_CUSTOM;
		else
			fprintf(stderr, "invalid type for variable declaration for v.'%s', using \"expression\"\n", name);
	}

	if (basetype) {
		if (strcasecmp((char *)basetype, "number") == 0)
			rtype = OCRPT_RESULT_NUMBER;
		else if (strcasecmp((char *)basetype, "string") == 0)
			rtype = OCRPT_RESULT_STRING;
		else if (strcasecmp((char *)basetype, "datetime") == 0)
			rtype = OCRPT_RESULT_DATETIME;
		else
			fprintf(stderr, "invalid result type for custom variable declaration for v.'%s'\n", name);
	}

	if (!baseexpr && value) {
		baseexpr = value;
		value = NULL;
	}

	if (baseexpr) {
		ocrpt_var *v;

		if (!intermedexpr && !intermed2expr && !resultexpr && vtype == OCRPT_VARIABLE_CUSTOM)
			vtype = OCRPT_VARIABLE_EXPRESSION;

		if (vtype == OCRPT_VARIABLE_CUSTOM)
			v = ocrpt_variable_new_full(o, r, rtype, (char *)name, (char *)baseexpr, (char *)intermedexpr, (char *)intermed2expr, (char *)resultexpr, (char *)resetonbreak);
		else
			v = ocrpt_variable_new(o, r, vtype, (char *)name, (char *)baseexpr, (char *)resetonbreak);

		if (precalculate || delayed) {
			xmlChar *p;
			ocrpt_expr *p_e;
			int32_t p_i = 0;

			if (precalculate) {
				p = precalculate;
				if (delayed)
					fprintf(stderr, "\"precalculate\" and \"delayed\" are set for for variable declaration for v.'%s', using \"precalculate\"\n", name);
			} else
				p = delayed;

			ocrpt_xml_const_expr_parse_get_int_value_with_fallback(o, p);
			ocrpt_variable_set_precalculate(v, !!p_i);
			ocrpt_expr_free(o, r, p_e);
		}
	}

	xmlFree(name);
	xmlFree(value);
	xmlFree(baseexpr);
	xmlFree(intermedexpr);
	xmlFree(intermed2expr);
	xmlFree(resultexpr);
	xmlFree(type);
	xmlFree(basetype);
	xmlFree(resetonbreak);
	xmlFree(precalculate);
	xmlFree(delayed);

	ocrpt_ignore_child_nodes(o, reader, -1, "Variable");
}

static void ocrpt_parse_variables_node(opencreport *o, ocrpt_report *r, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Variables")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Variable"))
				ocrpt_parse_variable_node(o, r, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_output_line_element_node(opencreport *o, ocrpt_report *r, ocrpt_line *line, bool literal, xmlTextReaderPtr reader) {
	ocrpt_line_element *elem;
	int ret;

	elem = ocrpt_mem_malloc(sizeof(ocrpt_line_element));
	memset(elem, 0, sizeof(ocrpt_line_element));
	line->elements = ocrpt_list_append(line->elements, elem);

	xmlChar *value, *delayed, *format, *width, *align, *color, *bgcolor, *font_name, *font_size;
	xmlChar *bold, *italic, *link, *memo, *memo_wrap_chars, *memo_max_lines, *col;
	struct {
		char *attrs[3];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "value"}, &value },
		{ { "delayed", "precalculate" }, &delayed },
		{ { "format" }, &format },
		{ { "width" }, &width },
		{ { "align" }, &align },
		{ { "color", "colour" }, &color },
		{ { "bgcolor", "bgcolour" }, &bgcolor },
		{ { "font_name", "fontName" }, &font_name },
		{ { "font_size", "fontSize" }, &font_size },
		{ { "bold" }, &bold },
		{ { "italic", "italics" }, &italic },
		{ { "link" }, &link },
		{ { "memo" }, &memo },
		{ { "memo_wrap_chars" }, &memo_wrap_chars },
		{ { "memo_max_lines" }, &memo_max_lines },
		{ { "col" }, &col },
		{ { NULL }, NULL },
	};
	int32_t i, j, delayed_i = 0;
	bool run_ignore_child = true;
	bool literal_string = false;

	for (i = 0; xmlattrs[i].attrp; i++) {
		for (j = 0; xmlattrs[i].attrs[j]; j++) {
			*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs[j]);
			if (*xmlattrs[i].attrp)
				break;
		}
	}

	int depth = xmlTextReaderDepth(reader);

	if (!value && !xmlTextReaderIsEmptyElement(reader)) {
		ret = xmlTextReaderRead(reader);
		if (ret == 1) {
			int nodetype = xmlTextReaderNodeType(reader);
			if (nodetype == XML_READER_TYPE_TEXT && depth == (xmlTextReaderDepth(reader) - 1)) {
				value = xmlTextReaderReadString(reader);
				literal_string = true;
				/* Advance the reader position to </literal> */
				xmlTextReaderRead(reader);
				run_ignore_child = false;
			}
		}
	}

	if (delayed) {
		ocrpt_expr *delayed_e;
		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, delayed);
		ocrpt_expr_free(o, r, delayed_e);
	}

	if (literal_string)
		elem->value = ocrpt_newstring(o, r, (char *)value);
	else
		elem->value = ocrpt_xml_expr_parse(o, r, value, true, false);
	ocrpt_expr_set_delayed(o, elem->value, !!delayed_i);

	elem->format = ocrpt_xml_expr_parse(o, r, format, true, false);
	if (elem->format)
		elem->format->rvalue = elem->value;

	elem->width = ocrpt_xml_expr_parse(o, r, width, true, false);
	if (elem->width)
		elem->width->rvalue = elem->value;

	if (align && (strcasecmp((char *)align, "left") == 0 || strcasecmp((char *)align, "right") == 0 || strcasecmp((char *)align, "center") == 0))
		elem->align = ocrpt_newstring(o, r, (char *)align);
	else
		elem->align = ocrpt_xml_expr_parse(o, r, align, true, false);
	if (elem->align)
		elem->align->rvalue = elem->value;

	elem->color = ocrpt_xml_expr_parse(o, r, color, true, false);
	if (elem->color)
		elem->color->rvalue = elem->value;

	elem->bgcolor = ocrpt_xml_expr_parse(o, r, bgcolor, true, false);
	if (elem->bgcolor)
		elem->bgcolor->rvalue = elem->value;

	elem->font_name = ocrpt_xml_expr_parse(o, r, font_name, false, true);
	if (elem->font_name)
		elem->font_name->rvalue = elem->value;

	elem->font_size = ocrpt_xml_expr_parse(o, r, font_name, true, false);
	if (elem->font_size)
		elem->font_size->rvalue = elem->value;

	elem->bold = ocrpt_xml_expr_parse(o, r, bold, true, false);
	if (elem->bold)
		elem->bold->rvalue = elem->value;

	elem->italic = ocrpt_xml_expr_parse(o, r, italic, true, false);
	if (elem->italic)
		elem->italic->rvalue = elem->value;

	elem->link = ocrpt_xml_expr_parse(o, r, link, true, false);
	if (elem->link)
		elem->link->rvalue = elem->value;

	if (memo) {
		ocrpt_expr *memo_e;
		int32_t memo_i;
		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, memo);
		elem->memo = !!memo_i;
		ocrpt_expr_free(o, r, memo_e);
	}

	if (memo_wrap_chars) {
		ocrpt_expr *memo_wrap_chars_e;
		int32_t memo_wrap_chars_i;
		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, memo_wrap_chars);
		elem->memo_wrap_chars = !!memo_wrap_chars_i;
		ocrpt_expr_free(o, r, memo_wrap_chars_e);
	}

	if (memo_max_lines) {
		ocrpt_expr *memo_max_lines_e;
		int32_t memo_max_lines_i;
		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, memo_max_lines);
		elem->memo_max_lines = memo_max_lines_i;
		ocrpt_expr_free(o, r, memo_max_lines_e);
	}

	if (col) {
		ocrpt_expr *col_e;
		int32_t col_i;
		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, col);
		elem->col = col_i;
		ocrpt_expr_free(o, r, col_e);
	}

	xmlFree(value);
	xmlFree(delayed);
	xmlFree(format);
	xmlFree(width);
	xmlFree(align);
	xmlFree(color);
	xmlFree(bgcolor);
	xmlFree(font_name);
	xmlFree(font_size);
    xmlFree(bold);
	xmlFree(italic);
	xmlFree(link);
	xmlFree(memo);
	xmlFree(memo_wrap_chars);
	xmlFree(memo_max_lines);
	xmlFree(col);

	if (run_ignore_child)
		ocrpt_ignore_child_nodes(o, reader, depth, literal ? "literal" : "field");
}

static void ocrpt_parse_output_line_node(opencreport *o, ocrpt_report *r, ocrpt_list **list_p, xmlTextReaderPtr reader) {
	ocrpt_line *line;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	line = ocrpt_mem_malloc(sizeof(ocrpt_line));
	memset(line, 0, sizeof(ocrpt_line));
	line->type = OCRPT_OUTPUT_LINE;

	*list_p = ocrpt_list_append(*list_p, line);

	xmlChar *font_name, *font_size, *bold, *italic, *suppress, *color, *bgcolor;
	struct {
		char *attrs[3];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "font_name", "fontName" }, &font_name },
		{ { "font_size", "fontSize" }, &font_size },
		{ { "bold" }, &bold },
		{ { "italic", "italics" }, &italic },
		{ { "suppress" }, &suppress },
		{ { "color", "colour" }, &color },
		{ { "bgcolor", "bgcolour" }, &bgcolor },
		{ { NULL }, NULL },
	};
	int32_t i, j;

	for (i = 0; xmlattrs[i].attrp; i++) {
		for (j = 0; xmlattrs[i].attrs[j]; j++) {
			*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs[j]);
			if (*xmlattrs[i].attrp)
				break;
		}
	}

	line->font_name = ocrpt_xml_expr_parse(o, r, font_name, false, true);
	line->font_size = ocrpt_xml_expr_parse(o, r, font_size, true, false);
	line->bold = ocrpt_xml_expr_parse(o, r, bold, true, false);
	line->italic = ocrpt_xml_expr_parse(o, r, italic, true, false);
	line->suppress = ocrpt_xml_expr_parse(o, r, suppress, true, false);
	line->color = ocrpt_xml_expr_parse(o, r, color, true, false);
	line->bgcolor = ocrpt_xml_expr_parse(o, r, bgcolor, true, false);

	xmlFree(font_name);
	xmlFree(font_size);
	xmlFree(bold);
	xmlFree(italic);
	xmlFree(suppress);
	xmlFree(color);
	xmlFree(bgcolor);

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Line")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "field"))
				ocrpt_parse_output_line_element_node(o, r, line, false, reader);
			else if (!strcmp((char *)name, "literal"))
				ocrpt_parse_output_line_element_node(o, r, line, true, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_output_hline_node(opencreport *o, ocrpt_report *r, ocrpt_list **list_p, xmlTextReaderPtr reader) {
	ocrpt_hline *hline = ocrpt_mem_malloc(sizeof(ocrpt_hline));
	xmlChar *size, *indent, *length, *font_size, *suppress, *color;
	struct {
		char *attrs[5];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "size" }, &size },
		{ { "indent" }, &indent },
		{ { "length" }, &length },
		{ { "font_size", "fontSize" }, &font_size },
		{ { "suppress" }, &suppress },
		{ { "color", "colour", "bgcolor", "bgcolour" }, &color },
		{ { NULL }, NULL },
	};
	int32_t i, j;

	for (i = 0; xmlattrs[i].attrp; i++) {
		for (j = 0; xmlattrs[i].attrs[j]; j++) {
			*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs[j]);
			if (*xmlattrs[i].attrp)
				break;
		}
	}

	hline->type = OCRPT_OUTPUT_HLINE;

	hline->size = ocrpt_xml_expr_parse(o, r, size, true, false);
	hline->indent = ocrpt_xml_expr_parse(o, r, indent, true, false);
	hline->length =  ocrpt_xml_expr_parse(o, r, length, true, false);
	hline->font_size = ocrpt_xml_expr_parse(o, r, font_size, true, false);
	hline->suppress = ocrpt_xml_expr_parse(o, r, suppress, true, false);
	hline->color = ocrpt_xml_expr_parse(o, r, color, false, true);

	xmlFree(size);
	xmlFree(indent);
	xmlFree(length);
	xmlFree(font_size);
	xmlFree(suppress);
	xmlFree(color);

	*list_p = ocrpt_list_append(*list_p, hline);

	ocrpt_ignore_child_nodes(o, reader, -1, "HorizontalLine");
}

static void ocrpt_parse_output_image_node(opencreport *o, ocrpt_report *r, ocrpt_list **list_p, xmlTextReaderPtr reader) {
	ocrpt_image *img = ocrpt_mem_malloc(sizeof(ocrpt_image));
	memset(img, 0, sizeof(ocrpt_image));
	img->type = OCRPT_OUTPUT_IMAGE;
	*list_p = ocrpt_list_append(*list_p, img);
}

static void ocrpt_parse_output_node(opencreport *o, ocrpt_report *r, ocrpt_list **list_p, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Output")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Line"))
				ocrpt_parse_output_line_node(o, r, list_p, reader);
			else if (!strcmp((char *)name, "HorizontalLine"))
				ocrpt_parse_output_hline_node(o, r, list_p, reader);
			else if (!strcmp((char *)name, "Image"))
				ocrpt_parse_output_image_node(o, r, list_p, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_output_parent_node(opencreport *o, ocrpt_report *r, const char *nodename, ocrpt_list **list_p, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, nodename)) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Output"))
				ocrpt_parse_output_node(o, r, list_p, reader);
		}

		xmlFree(name);
	}
}

static bool ocrpt_parse_breakfield_node(opencreport *o, ocrpt_report *r, ocrpt_break *br, xmlTextReaderPtr reader) {
	xmlChar *value = xmlTextReaderGetAttribute(reader, (const xmlChar *)"value");
	bool have_breakfield = false;

	if (value) {
		ocrpt_expr *e = ocrpt_expr_parse(o, r, (char *)value, NULL);
		have_breakfield = ocrpt_break_add_breakfield(o, r, br, e);
	}

	xmlFree(value);

	ocrpt_ignore_child_nodes(o, reader, -1, "BreakField");

	return have_breakfield;
}

static bool ocrpt_parse_breakfields_node(opencreport *o, ocrpt_report *r, ocrpt_break *br, xmlTextReaderPtr reader) {
	bool have_breakfield = false;

	/* No BreakField sub-element */
	if (xmlTextReaderIsEmptyElement(reader))
		return false;

	int depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "BreakFields")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "BreakField"))
				have_breakfield = ocrpt_parse_breakfield_node(o, r, br, reader) || have_breakfield;
		}

		xmlFree(name);
	}

	return have_breakfield;
}

static void ocrpt_break_node(opencreport *o, ocrpt_report *r, xmlTextReaderPtr reader) {
	xmlChar *brname, *newpage, *headernewpage, *suppressblank;
	ocrpt_break *br;
	bool have_breakfield = false;

	brname = xmlTextReaderGetAttribute(reader, (const xmlChar *)"name");

	if (!brname) {
		fprintf(stderr, "nameless break is useless, not adding to report\n");
		return;
	}

	/* There's no BreakFields sub-element, useless break. */
	if (xmlTextReaderIsEmptyElement(reader)) {
		fprintf(stderr, "break '%s' is useless, not adding to report\n", (char *)brname);
		xmlFree(brname);
		return;
	}

	br = ocrpt_break_new(o, r, (char *)brname);

	newpage = xmlTextReaderGetAttribute(reader, (const xmlChar *)"newpage");
	headernewpage = xmlTextReaderGetAttribute(reader, (const xmlChar *)"headernewpage");
	suppressblank = xmlTextReaderGetAttribute(reader, (const xmlChar *)"suppressblank");

	int depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Break")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "BreakFields"))
				have_breakfield = ocrpt_parse_breakfields_node(o, r, br, reader) || have_breakfield;
			else if (!strcmp((char *)name, "BreakHeader"))
				ocrpt_parse_output_parent_node(o, r, "BreakHeader", &br->header, reader);
			else if (!strcmp((char *)name, "BreakFooter"))
				ocrpt_parse_output_parent_node(o, r, "BreakFooter", &br->footer, reader);
		}

		xmlFree(name);
	}

	if (have_breakfield) {
		struct {
			char *name;
			xmlChar *value;
			ocrpt_break_attr_type type;
		} attribs[OCRPT_BREAK_ATTRS_COUNT] = {
			{ "newpage", newpage, OCRPT_BREAK_ATTR_NEWPAGE },
			{ "headernewpage", headernewpage, OCRPT_BREAK_ATTR_HEADERNEWPAGE },
			{ "suppressblank", suppressblank, OCRPT_BREAK_ATTR_SUPPRESSBLANK },
		};
		int i;

		for (i = 0; i < OCRPT_BREAK_ATTRS_COUNT; i++) {
			if (attribs[i].name && attribs[i].value) {
				ocrpt_expr *e = ocrpt_xml_const_expr_parse(o, attribs[i].value, false, true);
				ocrpt_break_set_attribute_from_expr(o, r, br, attribs[i].type, e);
			}
		}
	}

	/* There's no BreakFields sub-element, useless break. */
	if (!have_breakfield) {
		fprintf(stderr, "break '%s' is useless, not adding to report\n", (char *)brname);
		ocrpt_break_free(o, r, br);
		r->breaks = ocrpt_list_remove(r->breaks, br);
	}

	xmlFree(brname);
	xmlFree(newpage);
	xmlFree(headernewpage);
	xmlFree(suppressblank);
}

static void ocrpt_parse_breaks_node(opencreport *o, ocrpt_report *r, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Breaks")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Break"))
				ocrpt_break_node(o, r, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_alternate_node(opencreport *o, ocrpt_report *r, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Alternate")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "NoData"))
				ocrpt_parse_output_parent_node(o, r, "NoData", &r->nodata, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_detail_node(opencreport *o, ocrpt_report *r, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Detail")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "FieldHeaders"))
				ocrpt_parse_output_parent_node(o, r, "FieldHeaders", &r->fieldheader, reader);
			else if (!strcmp((char *)name, "FieldDetails"))
				ocrpt_parse_output_parent_node(o, r, "FieldDetails", &r->fielddetails, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_report_node(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	ocrpt_report *r = ocrpt_report_new(o);

	p = ocrpt_part_append_report(o, p, pr, pd, r);

	xmlChar *font_name, *font_size, *size_unit, *orientation;
	xmlChar *top_margin, *bottom_margin, *left_margin, *right_margin;
	xmlChar *paper_type, *iterations, *suppress_pageheader_firstpage, *query;
	xmlChar *detail_columns, *column_pad;
	struct {
		char *attrs[3];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "font_name", "fontName" }, &font_name },
		{ { "font_size", "fontSize" }, &font_size },
		{ { "size_unit" }, &size_unit },
		{ { "orientation" }, &orientation },
		{ { "top_margin", "topMargin" }, &top_margin },
		{ { "bottom_margin", "bottomMargin" }, &bottom_margin },
		{ { "left_margin", "leftMargin" }, &left_margin },
		{ { "right_margin", "rightMargin" }, &right_margin },
		{ { "paper_type", "paperType" }, &paper_type },
		{ { "iterations" }, &iterations },
		{ { "suppressPageHeaderFirstPage" }, &suppress_pageheader_firstpage },
		{ { "query" }, &query },
		{ { "detail_columns" }, &detail_columns },
		{ { "column_pad" }, &column_pad },
		{ { NULL }, NULL },
	};
	int32_t i, j;

	for (i = 0; xmlattrs[i].attrp; i++) {
		for (j = 0; xmlattrs[i].attrs[j]; j++) {
			*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs[j]);
			if (*xmlattrs[i].attrp)
				break;
		}
	}

	if (font_name) {
		ocrpt_expr *font_name_e;
		char *font_name_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, font_name);
		r->font_name = ocrpt_mem_strdup((char *)font_name);
		ocrpt_expr_free(o, NULL, font_name_e);
	}

	if (font_size) {
		ocrpt_expr *font_size_e;
		double font_size_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, font_size);
		if (font_size_d > 1.0) {
			r->font_size_set = true;
			r->font_size = font_size_d;
		}
		ocrpt_expr_free(o, NULL, font_size_e);
	}

	if (orientation) {
		ocrpt_expr *orientation_e;
		char *orientation_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, orientation);
		if (orientation_s && strcasecmp(orientation_s, "portrait") == 0) {
			r->orientation_set = true;
			r->landscape = false;
		} else if (orientation_s && strcasecmp((char *)orientation, "landscape") == 0) {
			r->orientation_set = true;
			r->landscape = true;
		}
		ocrpt_expr_free(o, NULL, orientation_e);
	}

	if (top_margin) {
		ocrpt_expr *top_margin_e;
		double top_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, top_margin);
		if (top_margin_d > 0.0) {
			r->top_margin_set = true;
			r->top_margin = top_margin_d;
		}
		ocrpt_expr_free(o, NULL, top_margin_e);
	}

	if (bottom_margin) {
		ocrpt_expr *bottom_margin_e;
		double bottom_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, bottom_margin);
		if (bottom_margin_d > 0.0) {
			r->bottom_margin_set = true;
			r->bottom_margin = bottom_margin_d;
		}
		ocrpt_expr_free(o, NULL, bottom_margin_e);
	}

	if (left_margin) {
		ocrpt_expr *left_margin_e;
		double left_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, left_margin);
		if (left_margin_d > 0.0) {
			r->left_margin_set = true;
			r->left_margin = left_margin_d;
		}
		ocrpt_expr_free(o, NULL, left_margin_e);
	}

	if (right_margin) {
		ocrpt_expr *right_margin_e;
		double right_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, right_margin);
		if (right_margin_d > 0.0) {
			r->right_margin_set = true;
			r->right_margin = right_margin_d;
		}
		ocrpt_expr_free(o, NULL, right_margin_e);
	}

	if (paper_type && !p->paper) {
		ocrpt_expr *paper_type_e;
		char *paper_type_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, paper_type);
		p->paper = ocrpt_get_paper_by_name(paper_type_s);
		ocrpt_expr_free(o, NULL, paper_type_e);
	}

	if (iterations) {
		ocrpt_expr *iterations_e;
		int32_t iterations_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, iterations);
		ocrpt_expr_free(o, NULL, iterations_e);
		if (iterations_i <= 0)
			iterations_i = 1;
		r->iterations = iterations_i;
	} else
		r->iterations = 1;

	if (suppress_pageheader_firstpage) {
		ocrpt_expr *suppress_pageheader_firstpage_e;
		int32_t suppress_pageheader_firstpage_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, suppress_pageheader_firstpage);
		r->suppress_pageheader_firstpage = !!suppress_pageheader_firstpage_i;
		ocrpt_expr_free(o, NULL, suppress_pageheader_firstpage_e);
	}

	if (query) {
		ocrpt_expr *query_e;
		char *query_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, query);
		r->query = ocrpt_query_get(o, query_s);
		ocrpt_expr_free(o, NULL, query_e);
	}

	if (detail_columns) {
		ocrpt_expr *detail_columns_e;
		int32_t detail_columns_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, detail_columns);
		r->detail_columns = detail_columns_i;
		ocrpt_expr_free(o, NULL, detail_columns_e);
	}

	if (column_pad) {
		ocrpt_expr *column_pad_e;
		double column_pad_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback_noreport(o, column_pad);
		r->column_pad = column_pad_d;
		ocrpt_expr_free(o, NULL, column_pad_e);
	}

	xmlFree(font_name);
	xmlFree(font_size);
	xmlFree(size_unit);
	xmlFree(orientation);
	xmlFree(top_margin);
	xmlFree(bottom_margin);
	xmlFree(left_margin);
	xmlFree(right_margin);
	xmlFree(paper_type);
	xmlFree(iterations);
	xmlFree(suppress_pageheader_firstpage);
	xmlFree(query);
	xmlFree(detail_columns);
	xmlFree(column_pad);

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Report")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Variables"))
				ocrpt_parse_variables_node(o, r, reader);
			else if (!strcmp((char *)name, "Breaks"))
				ocrpt_parse_breaks_node(o, r, reader);
			else if (!strcmp((char *)name, "Alternate"))
				ocrpt_parse_alternate_node(o, r, reader);
			else if (!strcmp((char *)name, "NoData"))
				ocrpt_parse_output_parent_node(o, r, "NoData", &r->nodata, reader);
			else if (!strcmp((char *)name, "PageHeader"))
				ocrpt_parse_output_parent_node(o, r, "PageHeader", &p->pageheader, reader);
			else if (!strcmp((char *)name, "PageFooter"))
				ocrpt_parse_output_parent_node(o, r, "PageFooter", &p->pagefooter, reader);
			else if (!strcmp((char *)name, "ReportHeader"))
				ocrpt_parse_output_parent_node(o, r, "ReportHeader", &r->reportheader, reader);
			else if (!strcmp((char *)name, "ReportFooter"))
				ocrpt_parse_output_parent_node(o, r, "ReportFooter", &r->reportfooter, reader);
			else if (!strcmp((char *)name, "Detail"))
				ocrpt_parse_detail_node(o, r, reader);
#if 0
			else if (!strcmp((char *)name, "Graph"))
				ocrpt_parse_graph_node(o, r, reader);
			else if (!strcmp((char *)name, "Chart"))
				ocrpt_parse_chart_node(o, r, reader);
#endif
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_pd_node(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	ocrpt_part_row_data *pd = ocrpt_part_row_new_data(o, p, pr);

	xmlChar *width, *height, *border_width, *border_color;
	struct {
		char *attr;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "width", &width },
		{ "height", &height },
		{ "border_width", &border_width },
		{ "border_color", &border_color },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attr);

	if (width) {
		ocrpt_expr *width_e;
		double width_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, width);
		pd->width = width_d;
		pd->width_set = true;
		ocrpt_expr_free(o, NULL, width_e);
	}

	if (height) {
		ocrpt_expr *height_e;
		double height_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, height);
		pd->height = height_d;
		pd->height_set = true;
		ocrpt_expr_free(o, NULL, height_e);
	}

	if (border_width) {
		ocrpt_expr *border_width_e;
		double border_width_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, border_width);
		pd->border_width = border_width_d;
		pd->border_width_set = true;
		ocrpt_expr_free(o, NULL, border_width_e);
	}

	char *border_color_s = NULL;
	if (border_color) {
		ocrpt_expr *border_color_e;
		char *border_color_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, border_color);
		ocrpt_expr_free(o, NULL, border_color_e);
	}
	ocrpt_get_color(o, border_color_s, &pd->border_color, true);

	xmlFree(width);
	xmlFree(height);
	xmlFree(border_width);
	xmlFree(border_color);

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "pd")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, p, pr, pd, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_partrow_node(opencreport *o, ocrpt_part *p, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	ocrpt_part_row *pr = ocrpt_part_new_row(o, p);

	xmlChar *layout, *newpage;
	struct {
		char *attr;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "layout", &layout },
		{ "newpage", &newpage },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attr);

	if (layout) {
		ocrpt_expr *layout_e;
		char *layout_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, layout);
		/* TODO: we only know what "flow" is meant to do */
		if (strcasecmp(layout_s, "flow") == 0) {
			pr->layout_set = true;
			pr->fixed = false;
		} else if (strcasecmp(layout_s, "fixed") == 0) {
			pr->layout_set = true;
			pr->fixed = true;
		}
		ocrpt_expr_free(o, NULL, layout_e);
	}

	if (newpage) {
		ocrpt_expr *newpage_e;
		int32_t newpage_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback(o, newpage);
		pr->newpage_set = true;
		pr->newpage = !!newpage_i;
		ocrpt_expr_free(o, NULL, newpage_e);
	}

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "pr")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "pd"))
				ocrpt_parse_pd_node(o, p, pr, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_part_node(opencreport *o, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	ocrpt_part *p = ocrpt_part_new(o);

	xmlChar *layout, *font_name, *font_size, *size_unit, *orientation;
	xmlChar *top_margin, *bottom_margin, *left_margin, *right_margin;
	xmlChar *paper_type, *iterations, *suppress_pageheader_firstpage;
	struct {
		char *attrs[3];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "layout" }, &layout },
		{ { "font_name", "fontName" }, &font_name },
		{ { "font_size", "fontSize" }, &font_size },
		{ { "size_unit" }, &size_unit },
		{ { "orientation" }, &orientation },
		{ { "top_margin", "topMargin" }, &top_margin },
		{ { "bottom_margin", "bottomMargin" }, &bottom_margin },
		{ { "left_margin", "leftMargin" }, &left_margin },
		{ { "right_margin", "rightMargin" }, &right_margin },
		{ { "paper_type", "paperType" }, &paper_type },
		{ { "iterations" }, &iterations },
		{ { "suppressPageHeaderFirstPage" }, &suppress_pageheader_firstpage },
		{ { NULL }, NULL },
	};
	int32_t i, j;

	for (i = 0; xmlattrs[i].attrp; i++) {
		for (j = 0; xmlattrs[i].attrs[j]; j++) {
			*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs[j]);
			if (*xmlattrs[i].attrp)
				break;
		}
	}

	if (layout) {
		ocrpt_expr *layout_e;
		char *layout_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, layout);
		/* TODO: we only know what "flow" is meant to do */
		if (strcasecmp(layout_s, "flow") == 0) {
			p->layout_set = true;
			p->fixed = false;
		} else if (strcasecmp(layout_s, "fixed") == 0) {
			p->layout_set = true;
			p->fixed = true;
		}
		ocrpt_expr_free(o, NULL, layout_e);
	}

	if (font_name) {
		ocrpt_expr *font_name_e;
		char *font_name_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, font_name);
		p->font_name = ocrpt_mem_strdup((char *)font_name);
		ocrpt_expr_free(o, NULL, font_name_e);
	}

	if (font_size) {
		ocrpt_expr *font_size_e;
		double font_size_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, font_size);
		if (font_size_d > 1.0) {
			p->font_size_set = true;
			p->font_size = font_size_d;
		}
		ocrpt_expr_free(o, NULL, font_size_e);
	}

	if (orientation) {
		ocrpt_expr *orientation_e;
		char *orientation_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, orientation);
		if (orientation_s && strcasecmp(orientation_s, "portrait") == 0) {
			p->orientation_set = true;
			p->landscape = false;
		} else if (orientation_s && strcasecmp((char *)orientation, "landscape") == 0) {
			p->orientation_set = true;
			p->landscape = true;
		}
		ocrpt_expr_free(o, NULL, orientation_e);
	}

	if (top_margin) {
		ocrpt_expr *top_margin_e;
		double top_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, top_margin);
		if (top_margin_d > 0.0) {
			p->top_margin_set = true;
			p->top_margin = top_margin_d;
		}
		ocrpt_expr_free(o, NULL, top_margin_e);
	}

	if (bottom_margin) {
		ocrpt_expr *bottom_margin_e;
		double bottom_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, bottom_margin);
		if (bottom_margin_d > 0.0) {
			p->bottom_margin_set = true;
			p->bottom_margin = bottom_margin_d;
		}
		ocrpt_expr_free(o, NULL, bottom_margin_e);
	}

	if (left_margin) {
		ocrpt_expr *left_margin_e;
		double left_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, left_margin);
		if (left_margin_d > 0.0) {
			p->left_margin_set = true;
			p->left_margin = left_margin_d;
		}
		ocrpt_expr_free(o, NULL, left_margin_e);
	}

	if (right_margin) {
		ocrpt_expr *right_margin_e;
		double right_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, right_margin);
		if (right_margin_d > 0.0) {
			p->right_margin_set = true;
			p->right_margin = right_margin_d;
		}
		ocrpt_expr_free(o, NULL, right_margin_e);
	}

	if (paper_type) {
		ocrpt_expr *paper_type_e;
		char *paper_type_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, paper_type);
		p->paper = ocrpt_get_paper_by_name(paper_type_s);
		ocrpt_expr_free(o, NULL, paper_type_e);
	}

	if (iterations) {
		ocrpt_expr *iterations_e;
		int32_t iterations_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, iterations);
		ocrpt_expr_free(o, NULL, iterations_e);
		if (iterations_i <= 0)
			iterations_i = 1;
		p->iterations = iterations_i;
	} else
		p->iterations = 1;

	if (suppress_pageheader_firstpage) {
		ocrpt_expr *suppress_pageheader_firstpage_e;
		int32_t suppress_pageheader_firstpage_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, suppress_pageheader_firstpage);
		p->suppress_pageheader_firstpage = !!suppress_pageheader_firstpage_i;
		ocrpt_expr_free(o, NULL, suppress_pageheader_firstpage_e);
	}

	xmlFree(font_name);
	xmlFree(font_size);
	xmlFree(size_unit);
	xmlFree(orientation);
	xmlFree(top_margin);
	xmlFree(bottom_margin);
	xmlFree(left_margin);
	xmlFree(right_margin);
	xmlFree(paper_type);
	xmlFree(iterations);
	xmlFree(suppress_pageheader_firstpage);

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Part")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "pr"))
				ocrpt_parse_partrow_node(o, p, reader);
			else if (!strcmp((char *)name, "PageHeader"))
				ocrpt_parse_output_parent_node(o, NULL, "PageHeader", &p->pageheader, reader);
			else if (!strcmp((char *)name, "PageFooter"))
				ocrpt_parse_output_parent_node(o, NULL, "PageFooter", &p->pagefooter, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_opencreport_node(opencreport *o, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "OpenCReport")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Datasources"))
				ocrpt_parse_datasources_node(o, reader);
			else if (!strcmp((char *)name, "Queries"))
				ocrpt_parse_queries_node(o, reader);
			else if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, NULL, NULL, NULL, reader);
			else if (!strcmp((char *)name, "Part"))
				ocrpt_parse_part_node(o, reader);
		}

		xmlFree(name);
	}
}

DLL_EXPORT_SYM bool ocrpt_parse_xml(opencreport *o, const char *filename) {
	xmlTextReaderPtr reader;
	bool retval = true;

	reader = xmlReaderForFile(filename, NULL, XML_PARSE_RECOVER |
								XML_PARSE_NOENT | XML_PARSE_NOBLANKS |
								XML_PARSE_XINCLUDE | XML_PARSE_NOXINCNODE);

	if (!reader)
		return false;

	while (xmlTextReaderRead(reader) == 1) {
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
			else if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, NULL, NULL, NULL, reader);
			else if (!strcmp((char *)name, "Part"))
				ocrpt_parse_part_node(o, reader);
		}

		xmlFree(name);
	}

	xmlFreeTextReader(reader);

	return retval;
}
