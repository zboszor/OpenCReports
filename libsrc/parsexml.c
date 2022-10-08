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
#include "ocrpt-private.h"
#include "listutil.h"
#include "datasource.h"
#include "exprutil.h"
#include "breaks.h"
#include "parts.h"

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
	xmlChar *name, *value_att, *value = NULL;
	xmlChar *datasource, *follower_for, *follower_expr;
	xmlChar *cols, *rows, *coltypes;

	ocrpt_expr *name_e, *value_e, *datasource_e;
	ocrpt_expr *follower_for_e, *follower_expr_e;
	ocrpt_expr *cols_e, *rows_e, *coltypes_e;

	char *name_s, *value_s, *datasource_s;
	char *follower_for_s, *follower_expr_s, *coltypes_s;
	int32_t cols_i, rows_i;

	void *arrayptr, *coltypesptr;
	ocrpt_datasource *ds;
	ocrpt_query *q = NULL, *lq = NULL;
	int ret, depth, nodetype;

	struct {
		char *attrs;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "name", &name },
		{ "value", &value_att },
		{ "datasource", &datasource },
		{ "follower_for", &follower_for },
		{ "follower_expr", &follower_expr },
		{ "cols", &cols },
		{ "rows", &rows },
		{ "coltypes", &coltypes },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs);

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


	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	if (value != value_att)
		xmlFree(value);

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
	xmlChar *name, *type, *host, *unix_socket;
	xmlChar *port, *dbname, *user, *password;
	xmlChar *connstr, *optionfile, *group;
	ocrpt_expr *name_e, *type_e, *host_e, *unix_socket_e;
	ocrpt_expr *port_e, *dbname_e, *user_e, *password_e;
	ocrpt_expr *connstr_e, *optionfile_e, *group_e;
	char *name_s, *type_s, *host_s, *unix_socket_s;
	char *port_s, *dbname_s, *user_s, *password_s;
	char *connstr_s, *optionfile_s, *group_s;
	int32_t port_i, i;

	struct {
		char *attrs;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "name", &name },
		{ "type", &type },
		{ "host", &host },
		{ "unix_socket", &unix_socket },
		{ "port", &port },
		{ "dbname", &dbname },
		{ "user", &user },
		{ "password", &password },
		{ "connstr", &connstr },
		{ "optionfile", &optionfile },
		{ "group", &group },
		{ NULL, NULL },
	};

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs);

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

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

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
	xmlChar *name, *baseexpr, *intermedexpr, *intermed2expr, *resultexpr;
	xmlChar *type, *basetype, *resetonbreak, *precalculate;
	ocrpt_var_type vtype = OCRPT_VARIABLE_EXPRESSION; /* default if left out */
	enum ocrpt_result_type rtype = OCRPT_RESULT_NUMBER;
	int32_t i, j;

	struct {
		char *attrs[3];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "name" }, &name },
		{ { "baseexpr", "value" }, &baseexpr },
		{ { "intermedexpr" }, &intermedexpr },
		{ { "intermed2expr" }, &intermed2expr },
		{ { "resultexpr" }, &resultexpr },
		{ { "type" }, &type },
		{ { "basetype" }, &basetype },
		{ { "resetonbreak" }, &resetonbreak },
		{ { "precalculate", "delayed" }, &precalculate },
		{ { NULL }, NULL },
	};

	for (i = 0; xmlattrs[i].attrp; i++) {
		for (j = 0; xmlattrs[i].attrs[j]; j++) {
			*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs[j]);
			if (*xmlattrs[i].attrp)
				break;
		}
	}

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
		if (strcasecmp((char *)basetype, "number") == 0 || strcasecmp((char *)basetype, "numeric") == 0)
			rtype = OCRPT_RESULT_NUMBER;
		else if (strcasecmp((char *)basetype, "string") == 0)
			rtype = OCRPT_RESULT_STRING;
		else if (strcasecmp((char *)basetype, "datetime") == 0)
			rtype = OCRPT_RESULT_DATETIME;
		else
			fprintf(stderr, "invalid result type for custom variable declaration for v.'%s'\n", name);
	}

	if (baseexpr) {
		ocrpt_var *v;

		if (!intermedexpr && !intermed2expr && !resultexpr && vtype == OCRPT_VARIABLE_CUSTOM)
			vtype = OCRPT_VARIABLE_EXPRESSION;

		if (vtype == OCRPT_VARIABLE_CUSTOM)
			v = ocrpt_variable_new_full(o, r, rtype, (char *)name, (char *)baseexpr, (char *)intermedexpr, (char *)intermed2expr, (char *)resultexpr, (char *)resetonbreak);
		else
			v = ocrpt_variable_new(o, r, vtype, (char *)name, (char *)baseexpr, (char *)resetonbreak);

		if (precalculate) {
			ocrpt_expr *precalculate_e;
			int32_t precalculate_i = 0;

			ocrpt_xml_const_expr_parse_get_int_value_with_fallback(o, precalculate);
			ocrpt_variable_set_precalculate(v, !!precalculate_i);
			ocrpt_expr_free(o, r, precalculate_e);
		}
	}

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

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

	if (align && (strcasecmp((char *)align, "left") == 0 || strcasecmp((char *)align, "right") == 0 || strcasecmp((char *)align, "center") == 0 || strcasecmp((char *)align, "justified") == 0))
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

	elem->font_size = ocrpt_xml_expr_parse(o, r, font_size, true, false);
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

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	if (run_ignore_child)
		ocrpt_ignore_child_nodes(o, reader, depth, literal ? "literal" : "field");
}

static void ocrpt_parse_output_line_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, xmlTextReaderPtr reader) {
	ocrpt_line *line;

	line = ocrpt_mem_malloc(sizeof(ocrpt_line));
	memset(line, 0, sizeof(ocrpt_line));
	line->type = OCRPT_OUTPUT_LINE;

	output->output_list = ocrpt_list_append(output->output_list, line);

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

	if (font_name)
		line->font_name = ocrpt_xml_expr_parse(o, r, font_name, false, true);
	if (font_size)
		line->font_size = ocrpt_xml_expr_parse(o, r, font_size, true, false);
	if (bold)
		line->bold = ocrpt_xml_expr_parse(o, r, bold, true, false);
	if (italic)
		line->italic = ocrpt_xml_expr_parse(o, r, italic, true, false);
	if (suppress)
		line->suppress = ocrpt_xml_expr_parse(o, r, suppress, true, false);
	if (color)
		line->color = ocrpt_xml_expr_parse(o, r, color, true, false);
	if (bgcolor)
		line->bgcolor = ocrpt_xml_expr_parse(o, r, bgcolor, true, false);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	if (xmlTextReaderIsEmptyElement(reader))
		return;

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

static void ocrpt_parse_output_hline_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, xmlTextReaderPtr reader) {
	ocrpt_hline *hline;
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

	hline = ocrpt_mem_malloc(sizeof(ocrpt_hline));
	memset(hline, 0, sizeof(ocrpt_hline));
	hline->type = OCRPT_OUTPUT_HLINE;

	if (size)
		hline->size = ocrpt_xml_expr_parse(o, r, size, true, false);
	if (indent)
		hline->indent = ocrpt_xml_expr_parse(o, r, indent, true, false);
	if (length)
		hline->length =  ocrpt_xml_expr_parse(o, r, length, true, false);
	if (font_size)
		hline->font_size = ocrpt_xml_expr_parse(o, r, font_size, true, false);
	if (suppress)
		hline->suppress = ocrpt_xml_expr_parse(o, r, suppress, true, false);
	if (color)
		hline->color = ocrpt_xml_expr_parse(o, r, color, false, true);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	output->output_list = ocrpt_list_append(output->output_list, hline);

	ocrpt_ignore_child_nodes(o, reader, -1, "HorizontalLine");
}

static void ocrpt_parse_output_image_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, xmlTextReaderPtr reader) {
	ocrpt_image *img = ocrpt_mem_malloc(sizeof(ocrpt_image));
	xmlChar *value, *suppress, *type, *width, *height;
	struct {
		char *attrs;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "value", &value },
		{ "suppress", &suppress },
		{ "type", &type },
		{ "width", &width },
		{ "height", &height },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs);

	memset(img, 0, sizeof(ocrpt_image));
	img->type = OCRPT_OUTPUT_IMAGE;
	output->output_list = ocrpt_list_append(output->output_list, img);

	if (value)
		img->value = ocrpt_xml_expr_parse(o, r, value, true, false);
	if (suppress)
		img->suppress = ocrpt_xml_expr_parse(o, r, suppress, true, false);
	if (type)
		img->imgtype = ocrpt_xml_expr_parse(o, r, type, true, false);
	if (width)
		img->width = ocrpt_xml_expr_parse(o, r, width, true, false);
	if (height)
		img->height = ocrpt_xml_expr_parse(o, r, height, true, false);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	ocrpt_ignore_child_nodes(o, reader, -1, "Image");
}

static void ocrpt_parse_output_imageend_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, xmlTextReaderPtr reader) {
	ocrpt_output_element *elem = ocrpt_mem_malloc(sizeof(ocrpt_output_element));

	elem->type = OCRPT_OUTPUT_IMAGEEND;
	output->output_list = ocrpt_list_append(output->output_list, elem);

	ocrpt_ignore_child_nodes(o, reader, -1, "ImageEnd");
}

static void ocrpt_parse_output_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, xmlTextReaderPtr reader) {
	xmlChar *suppress;
	struct {
		char *attr;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "suppress", &suppress },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attr);

	if (suppress)
		output->suppress = ocrpt_xml_expr_parse(o, r, suppress, true, false);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

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
				ocrpt_parse_output_line_node(o, r, output, reader);
			else if (!strcmp((char *)name, "HorizontalLine"))
				ocrpt_parse_output_hline_node(o, r, output, reader);
			else if (!strcmp((char *)name, "Image"))
				ocrpt_parse_output_image_node(o, r, output, reader);
			else if (!strcmp((char *)name, "ImageEnd"))
				ocrpt_parse_output_imageend_node(o, r, output, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_output_parent_node(opencreport *o, ocrpt_report *r, const char *nodename, ocrpt_output *output, xmlTextReaderPtr reader) {
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
				ocrpt_parse_output_node(o, r, output, reader);
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

static ocrpt_report *ocrpt_parse_report_node(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return NULL;

	bool create_parents = false;

	if (!p) {
		p = ocrpt_part_new(o);
		create_parents = true;
	}

	if (create_parents || !pr)
		pr = ocrpt_part_new_row(o, p);

	if (create_parents || !pd)
		pd = ocrpt_part_row_new_data(o, p, pr);

	ocrpt_report *r = ocrpt_report_new(o);
	ocrpt_part_append_report(o, p, pr, pd, r);

	xmlChar *font_name, *font_size;
	xmlChar *size_unit, *noquery_show_nodata, *report_height_after_last;
	xmlChar *orientation, *top_margin, *bottom_margin, *left_margin, *right_margin;
	xmlChar *paper_type, *iterations, *suppress, *suppress_pageheader_firstpage;
	xmlChar *query, *field_header_priority, *height;
	xmlChar *border_width, *border_color, *detail_columns, *column_pad;
	struct {
		char *attrs[3];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "font_name", "fontName" }, &font_name },
		{ { "font_size", "fontSize" }, &font_size },
		{ { "size_unit" }, &size_unit },
		{ { "noquery_show_nodata" }, &noquery_show_nodata },
		{ { "report_height_after_last" }, &report_height_after_last },
		{ { "orientation" }, &orientation },
		{ { "top_margin", "topMargin" }, &top_margin },
		{ { "bottom_margin", "bottomMargin" }, &bottom_margin },
		{ { "left_margin", "leftMargin" }, &left_margin },
		{ { "right_margin", "rightMargin" }, &right_margin },
		{ { "paper_type", "paperType" }, &paper_type },
		{ { "iterations" }, &iterations },
		{ { "height" }, &height},
		{ { "suppress" }, &suppress },
		{ { "suppressPageHeaderFirstPage" }, &suppress_pageheader_firstpage },
		{ { "query" }, &query },
		{ { "field_header_priority" }, &field_header_priority },
		{ { "border_width" }, &border_width },
		{ { "border_color" }, &border_color },
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

	if (!o->noquery_show_nodata_set) {
		o->noquery_show_nodata = false;
		if (noquery_show_nodata) {
			ocrpt_expr *noquery_show_nodata_e;
			int32_t noquery_show_nodata_i;
			ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, noquery_show_nodata);
			ocrpt_expr_free(o, NULL, noquery_show_nodata_e);
			o->noquery_show_nodata = !!noquery_show_nodata_i;
		}
		o->noquery_show_nodata_set = true;
	}

	if (!o->report_height_after_last_set) {
		o->report_height_after_last = false;
		if (report_height_after_last) {
			ocrpt_expr *report_height_after_last_e;
			int32_t report_height_after_last_i;
			ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, report_height_after_last);
			ocrpt_expr_free(o, NULL, report_height_after_last_e);
			o->report_height_after_last = !!report_height_after_last_i;
		}
		o->report_height_after_last_set = true;
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
		if (font_size_d > 0.0) {
			r->font_size_set = true;
			r->font_size = font_size_d;
		}
		ocrpt_expr_free(o, NULL, font_size_e);
	}

	if (height) {
		ocrpt_expr *height_e;
		double height_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, height);
		if (height_d > 1.0) {
			r->height_set = true;
			r->height = height_d;
		}
		ocrpt_expr_free(o, NULL, height_e);
	}

	if (!o->size_unit_set && size_unit) {
		ocrpt_expr *size_unit_e;
		char *size_unit_s;
		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, size_unit);
		o->size_in_points = false;
		if (size_unit_s && strcasecmp(size_unit_s, "points") == 0)
			o->size_in_points = true;
		ocrpt_expr_free(o, NULL, size_unit_e);
		o->size_unit_set = true;
	}

	if (!p->orientation_set && orientation) {
		ocrpt_expr *orientation_e;
		char *orientation_s;

		p->landscape = false;
		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, orientation);
		if (!p->orientation_set && orientation_s && strcasecmp(orientation_s, "portrait") == 0)
			p->landscape = false;
		else if (!p->orientation_set && orientation_s && strcasecmp((char *)orientation, "landscape") == 0)
			p->landscape = true;
		ocrpt_expr_free(o, NULL, orientation_e);
		p->orientation_set = true;
	}

	if (!p->top_margin_set && top_margin) {
		ocrpt_expr *top_margin_e;
		double top_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, top_margin);
		if (top_margin_d > 0.0) {
			p->top_margin_set = true;
			p->top_margin = top_margin_d;
		}
		ocrpt_expr_free(o, NULL, top_margin_e);
	}

	if (!p->bottom_margin_set && bottom_margin) {
		ocrpt_expr *bottom_margin_e;
		double bottom_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, bottom_margin);
		if (bottom_margin_d > 0.0) {
			p->bottom_margin_set = true;
			p->bottom_margin = bottom_margin_d;
		}
		ocrpt_expr_free(o, NULL, bottom_margin_e);
	}

	if (!p->left_margin_set && left_margin) {
		ocrpt_expr *left_margin_e;
		double left_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, left_margin);
		if (left_margin_d > 0.0) {
			p->left_margin_set = true;
			p->left_margin = left_margin_d;
		}
		ocrpt_expr_free(o, NULL, left_margin_e);
	}

	if (!p->right_margin_set && right_margin) {
		ocrpt_expr *right_margin_e;
		double right_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, right_margin);
		if (right_margin_d > 0.0) {
			p->right_margin_set = true;
			p->right_margin = right_margin_d;
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

	if (!p->suppress_pageheader_firstpage_set && suppress_pageheader_firstpage) {
		ocrpt_expr *suppress_pageheader_firstpage_e;
		int32_t suppress_pageheader_firstpage_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, suppress_pageheader_firstpage);
		p->suppress_pageheader_firstpage_set = true;
		p->suppress_pageheader_firstpage = !!suppress_pageheader_firstpage_i;
		ocrpt_expr_free(o, NULL, suppress_pageheader_firstpage_e);
	}

	if (suppress) {
		ocrpt_expr *suppress_e;
		int32_t suppress_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, suppress);
		r->suppress = !!suppress_i;
		ocrpt_expr_free(o, NULL, suppress_e);
	}

	if (query) {
		ocrpt_expr *query_e;
		char *query_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, query);
		r->query = ocrpt_query_get(o, query_s);
		ocrpt_expr_free(o, NULL, query_e);
	} else if (o->queries)
		r->query = (ocrpt_query *)o->queries->data;

	r->fieldheader_high_priority = true;
	if (field_header_priority) {
		ocrpt_expr *field_header_priority_e;
		char *field_header_priority_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, field_header_priority);
		if (field_header_priority_s && strcasecmp(field_header_priority_s, "low") == 0)
			r->fieldheader_high_priority = false;

		ocrpt_expr_free(o, NULL, field_header_priority_e);
	}

	if (!pd->border_width_set_from_pd) {
		pd->border_width = 0.0;
		pd->border_width_set = false;
		if (border_width) {
			ocrpt_expr *border_width_e;
			double border_width_d;

			ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, border_width);
			ocrpt_expr_free(o, NULL, border_width_e);
			if (border_width_d > 0.0) {
				pd->border_width = border_width_d;
				pd->border_width_set = true;
			}
		}
		pd->border_width_set_from_pd = true;

		if (border_color) {
			ocrpt_expr *border_color_e;
			char *border_color_s;

			ocrpt_xml_const_expr_parse_get_value_with_fallback(o, border_color);
			ocrpt_get_color(o, border_color_s, &pd->border_color, false);
			ocrpt_expr_free(o, NULL, border_color_e);
		} else
			ocrpt_get_color(o, NULL, &pd->border_color, false);
	}

	if (!pd->detail_columns_set) {
		pd->detail_columns = 1;
		if (detail_columns) {
			ocrpt_expr *detail_columns_e;
			int32_t detail_columns_i;

			ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, detail_columns);
			if (detail_columns_i < 1)
				detail_columns_i = 1;
			pd->detail_columns = detail_columns_i;
			ocrpt_expr_free(o, NULL, detail_columns_e);
		}

		pd->column_pad = 0.0;
		if (column_pad) {
			ocrpt_expr *column_pad_e;
			double column_pad_d;

			ocrpt_xml_const_expr_parse_get_double_value_with_fallback_noreport(o, column_pad);
			if (column_pad_d < 0.0)

				column_pad_d = 0.0;
			pd->column_pad = column_pad_d;
			ocrpt_expr_free(o, NULL, column_pad_e);
		}

		pd->detail_columns_set = true;
	}

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

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
			else if (!strcmp((char *)name, "PageHeader")) {
				if (p->pageheader.output_list)
					ocrpt_ignore_child_nodes(o, reader, -1, "PageHeader");
				else
					ocrpt_parse_output_parent_node(o, NULL, "PageHeader", &p->pageheader, reader);
			} else if (!strcmp((char *)name, "PageFooter")) {
				if (p->pagefooter.output_list)
					ocrpt_ignore_child_nodes(o, reader, -1, "PageFooter");
				else
					ocrpt_parse_output_parent_node(o, NULL, "PageFooter", &p->pagefooter, reader);
			} else if (!strcmp((char *)name, "ReportHeader"))
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

	return r;
}

static void ocrpt_parse_load(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, xmlTextReaderPtr reader_parent) {
	xmlChar *filename, *query, *iterations;
	struct {
		char *attr;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "name", &filename },
		{ "query", &query },
		{ "iterations", &iterations },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader_parent, (const xmlChar *)xmlattrs[i].attr);

	if (filename) {
		ocrpt_expr *filename_e, *query_e = NULL;
		char *filename_s, *real_filename, *query_s = NULL;
		ocrpt_report *r = NULL;
		int32_t iterations_i;

		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, filename);

		real_filename = ocrpt_find_file(o, filename_s);
		if (!real_filename) {
			fprintf(stderr, "ocrpt_parse_load: can't find file %s\n", filename_s);
			return;
		}

		if (query)
			ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, query);

		iterations_i = 1;
		if (iterations) {
			ocrpt_expr *iterations_e;

			ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, iterations);
			if (iterations_i < 1)
				iterations_i = 1;
			ocrpt_expr_free(o, NULL, iterations_e);
		}

		xmlTextReaderPtr reader;

		reader = xmlReaderForFile(real_filename, NULL, XML_PARSE_RECOVER |
									XML_PARSE_NOENT | XML_PARSE_NOBLANKS |
									XML_PARSE_XINCLUDE | XML_PARSE_NOXINCNODE);
		ocrpt_mem_free(real_filename);
		if (!reader) {
			fprintf(stderr, "ocrpt_parse_load: invalid XML file name or invalid contents\n");
			return;
		}

		while (xmlTextReaderRead(reader) == 1) {
			xmlChar *name = xmlTextReaderName(reader);
			int nodeType = xmlTextReaderNodeType(reader);
			int depth = xmlTextReaderDepth(reader);

			//processNode(reader);

			if (nodeType == XML_READER_TYPE_DOCUMENT_TYPE) {
				/* ignore - xmllint validation is enough */
			} else if (nodeType == XML_READER_TYPE_ELEMENT && depth == 0) {
				if (!strcmp((char *)name, "Report"))
					r = ocrpt_parse_report_node(o, p, pr, pd, reader);
			}

			xmlFree(name);
		}

		xmlFreeTextReader(reader);

		/* If there was a query specified in <load> then set it for the report */
		if (r) {
			if (query_s)
				r->query = ocrpt_query_get(o, query_s);
			else if (o->queries)
				r->query = (ocrpt_query *)o->queries->data;

			r->iterations = iterations_i;
		}

		ocrpt_expr_free(o, NULL, filename_e);
		ocrpt_expr_free(o, NULL, query_e);
	}

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);
}

static void ocrpt_parse_pd_node(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, xmlTextReaderPtr reader) {
	ocrpt_part_row_data *pd = ocrpt_part_row_new_data(o, p, pr);

	xmlChar *width, *height, *border_width, *border_color;
	xmlChar *detail_columns, *column_pad, *suppress;
	struct {
		char *attr;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "width", &width },
		{ "height", &height },
		{ "border_width", &border_width },
		{ "border_color", &border_color },
		{ "detail_columns", &detail_columns },
		{ "column_pad", &column_pad },
		{ "suppress", &suppress },
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

	pd->border_width = 0.0;
	pd->border_width_set = false;
	if (border_width) {
		ocrpt_expr *border_width_e;
		double border_width_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, border_width);
		ocrpt_expr_free(o, NULL, border_width_e);
		if (border_width_d > 0.0) {
			pd->border_width = border_width_d;
			pd->border_width_set = true;
		}
	}
	pd->border_width_set_from_pd = true;

	if (border_color) {
		ocrpt_expr *border_color_e;
		char *border_color_s;

		ocrpt_xml_const_expr_parse_get_value_with_fallback(o, border_color);
		ocrpt_get_color(o, border_color_s, &pd->border_color, false);
		ocrpt_expr_free(o, NULL, border_color_e);
	} else
		ocrpt_get_color(o, NULL, &pd->border_color, false);

	pd->detail_columns = 1;
	if (detail_columns) {
		ocrpt_expr *detail_columns_e;
		int32_t detail_columns_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, detail_columns);
		if (detail_columns_i < 1)
			detail_columns_i = 1;
		pd->detail_columns = detail_columns_i;
		ocrpt_expr_free(o, NULL, detail_columns_e);
	}
	pd->detail_columns_set = true;

	pd->column_pad = 0.0;
	if (column_pad) {
		ocrpt_expr *column_pad_e;
		double column_pad_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback_noreport(o, column_pad);
		if (column_pad_d < 0.0)

			column_pad_d = 0.0;
		pd->column_pad = column_pad_d;
		ocrpt_expr_free(o, NULL, column_pad_e);
	}

	if (suppress) {
		ocrpt_expr *suppress_e;
		int32_t suppress_i;
		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, suppress);
		pd->suppress = !!suppress_i;
		ocrpt_expr_free(o, NULL, suppress_e);
	}

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	if (xmlTextReaderIsEmptyElement(reader))
		return;

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
			else if (!strcmp((char *)name, "load"))
				ocrpt_parse_load(o, p, pr, pd, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_partrow_node(opencreport *o, ocrpt_part *p, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	ocrpt_part_row *pr = ocrpt_part_new_row(o, p);

	xmlChar *layout, *newpage, *suppress;
	struct {
		char *attr;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "layout", &layout },
		{ "newpage", &newpage },
		{ "suppress", &suppress },
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

	if (suppress) {
		ocrpt_expr *suppress_e;
		int32_t suppress_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, suppress);
		pr->suppress = !!suppress_i;
		ocrpt_expr_free(o, NULL, suppress_e);
	}

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

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

	xmlChar *font_name, *font_size;
	xmlChar *size_unit, *noquery_show_nodata, *report_height_after_last;
	xmlChar *orientation, *top_margin, *bottom_margin, *left_margin, *right_margin;
	xmlChar *paper_type, *iterations, *suppress, *suppress_pageheader_firstpage;
	struct {
		char *attrs[3];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "font_name", "fontName" }, &font_name },
		{ { "font_size", "fontSize" }, &font_size },
		{ { "size_unit" }, &size_unit },
		{ { "noquery_show_nodata" }, &noquery_show_nodata },
		{ { "report_height_after_last" }, &report_height_after_last },
		{ { "orientation" }, &orientation },
		{ { "top_margin", "topMargin" }, &top_margin },
		{ { "bottom_margin", "bottomMargin" }, &bottom_margin },
		{ { "left_margin", "leftMargin" }, &left_margin },
		{ { "right_margin", "rightMargin" }, &right_margin },
		{ { "paper_type", "paperType" }, &paper_type },
		{ { "iterations" }, &iterations },
		{ { "suppress" }, &suppress },
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

	if (!o->noquery_show_nodata_set) {
		o->noquery_show_nodata = false;
		if (noquery_show_nodata) {
			ocrpt_expr *noquery_show_nodata_e;
			int32_t noquery_show_nodata_i;
			ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, noquery_show_nodata);
			ocrpt_expr_free(o, NULL, noquery_show_nodata_e);
			o->noquery_show_nodata = !!noquery_show_nodata_i;
		}
		o->noquery_show_nodata_set = true;
	}

	if (!o->report_height_after_last_set) {
		o->report_height_after_last = false;
		if (report_height_after_last) {
			ocrpt_expr *report_height_after_last_e;
			int32_t report_height_after_last_i;
			ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, report_height_after_last);
			ocrpt_expr_free(o, NULL, report_height_after_last_e);
			o->report_height_after_last = !!report_height_after_last_i;
		}
		o->report_height_after_last_set = true;
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

	if (!o->size_unit_set && size_unit) {
		ocrpt_expr *size_unit_e;
		char *size_unit_s;
		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, size_unit);
		o->size_in_points = false;
		if (size_unit_s && strcasecmp(size_unit_s, "points") == 0)
			o->size_in_points = true;
		ocrpt_expr_free(o, NULL, size_unit_e);
	}
	o->size_unit_set = true;

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

	p->top_margin = OCRPT_DEFAULT_TOP_MARGIN;
	if (top_margin) {
		ocrpt_expr *top_margin_e;
		double top_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, top_margin);
		if (top_margin_d > 0.0)
			p->top_margin = top_margin_d;

		ocrpt_expr_free(o, NULL, top_margin_e);
	}
	p->top_margin_set = true;

	p->bottom_margin = OCRPT_DEFAULT_BOTTOM_MARGIN;
	if (bottom_margin) {
		ocrpt_expr *bottom_margin_e;
		double bottom_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, bottom_margin);
		if (bottom_margin_d > 0.0)
			p->bottom_margin = bottom_margin_d;
		ocrpt_expr_free(o, NULL, bottom_margin_e);
	}
	p->bottom_margin_set = true;

	p->left_margin = OCRPT_DEFAULT_LEFT_MARGIN;
	if (left_margin) {
		ocrpt_expr *left_margin_e;
		double left_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, left_margin);
		if (left_margin_d > 0.0)
			p->left_margin = left_margin_d;
		ocrpt_expr_free(o, NULL, left_margin_e);
	}
	p->left_margin_set = true;

	p->right_margin = OCRPT_DEFAULT_LEFT_MARGIN;
	if (right_margin) {
		ocrpt_expr *right_margin_e;
		double right_margin_d;

		ocrpt_xml_const_expr_parse_get_double_value_with_fallback(o, right_margin);
		if (right_margin_d > 0.0)
			p->right_margin = right_margin_d;
		ocrpt_expr_free(o, NULL, right_margin_e);
	}
	p->right_margin_set = true;

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

	if (suppress) {
		ocrpt_expr *suppress_e;
		int32_t suppress_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, suppress);
		p->suppress = !!suppress_i;
		ocrpt_expr_free(o, NULL, suppress_e);
	}

	if (suppress_pageheader_firstpage) {
		ocrpt_expr *suppress_pageheader_firstpage_e;
		int32_t suppress_pageheader_firstpage_i;

		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, suppress_pageheader_firstpage);
		p->suppress_pageheader_firstpage_set = true;
		p->suppress_pageheader_firstpage = !!suppress_pageheader_firstpage_i;
		ocrpt_expr_free(o, NULL, suppress_pageheader_firstpage_e);
	}

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

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
	xmlChar *size_unit = xmlTextReaderGetAttribute(reader, (const xmlChar *)"size_unit");
	xmlChar *noquery_show_nodata = xmlTextReaderGetAttribute(reader, (const xmlChar *)"noquery_show_nodata");
	xmlChar *report_height_after_last = xmlTextReaderGetAttribute(reader, (const xmlChar *)"report_height_after_last");

	o->noquery_show_nodata = true;
	if (noquery_show_nodata) {
		ocrpt_expr *noquery_show_nodata_e;
		int32_t noquery_show_nodata_i;
		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, noquery_show_nodata);
		ocrpt_expr_free(o, NULL, noquery_show_nodata_e);
		o->noquery_show_nodata = !!noquery_show_nodata_i;
	}
	o->noquery_show_nodata_set = true;

	o->report_height_after_last = true;
	if (report_height_after_last) {
		ocrpt_expr *report_height_after_last_e;
		int32_t report_height_after_last_i;
		ocrpt_xml_const_expr_parse_get_int_value_with_fallback_noreport(o, report_height_after_last);
		ocrpt_expr_free(o, NULL, report_height_after_last_e);
		o->report_height_after_last = !!report_height_after_last_i;
	}
	o->report_height_after_last_set = true;

	o->size_in_points = false;
	if (size_unit) {
		ocrpt_expr *size_unit_e;
		char *size_unit_s;
		ocrpt_xml_const_expr_parse_get_value_with_fallback_noreport(o, size_unit);

		if (size_unit_s && strcasecmp(size_unit_s, "points") == 0)
			o->size_in_points = true;
		ocrpt_expr_free(o, NULL, size_unit_e);
	}
	o->size_unit_set = true;

	xmlFree(size_unit);
	xmlFree(noquery_show_nodata);
	xmlFree(report_height_after_last);

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
