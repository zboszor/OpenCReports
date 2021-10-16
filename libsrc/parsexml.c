/*
 * OpenCReports main header
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
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

static ocrpt_expr *ocrpt_xml_expr_parse(opencreport *o, xmlChar *expr, bool report) {
	ocrpt_expr *e;
	char *err;

	if (!expr)
		return NULL;

	e = ocrpt_expr_parse(o, NULL, (char *)expr, &err);
	if (e) {
		ocrpt_expr_resolve_exclude(o, NULL, e, OCRPT_VARREF_RVAR | OCRPT_VARREF_IDENT | OCRPT_VARREF_VVAR);
		ocrpt_expr_optimize(o, NULL, e);
	} else {
		if (report)
			fprintf(stderr, "Cannot parse: %s\n", expr);
		ocrpt_strfree(err);
	}

	return e;
}

#define ocrpt_xml_expr_parse_get_value_with_fallback(o, expr) { \
					expr##_e = ocrpt_xml_expr_parse(o, expr, true); \
					ocrpt_expr_get_value(o, expr##_e, &expr##_s, NULL); \
					if (!expr##_s) \
						expr##_s = (char *)expr; \
				}

#define ocrpt_xml_expr_parse_get_value_with_fallback_noreport(o, expr) { \
					expr##_e = ocrpt_xml_expr_parse(o, expr, false); \
					ocrpt_expr_get_value(o, expr##_e, &expr##_s, NULL); \
					if (!expr##_s) \
						expr##_s = (char *)expr; \
				}

static void ocrpt_ignore_child_nodes(opencreport *o, xmlTextReaderPtr reader, const char *leaf_name) {
	int depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

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
	ocrpt_expr_get_value(o, cols_e, &cols_s, &cols_i);
	if (cols_s && !cols_i)
		cols_i = atoi(cols_s);

	rows_e = ocrpt_xml_expr_parse(o, rows, true);
	ocrpt_expr_get_value(o, rows_e, &rows_s, &rows_i);
	if (rows_s && !cols_i)
		rows_i = atoi(rows_s);

	ocrpt_xml_expr_parse_get_value_with_fallback(o, coltypes);

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
	int depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
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

	ocrpt_xml_expr_parse_get_value_with_fallback(o, name);
	ocrpt_xml_expr_parse_get_value_with_fallback(o, type);

	ocrpt_xml_expr_parse_get_value_with_fallback(o, host);
	ocrpt_xml_expr_parse_get_value_with_fallback(o, unix_socket);

	port_e = ocrpt_xml_expr_parse(o, port, true);
	ocrpt_expr_get_value(o, port_e, &port_s, &port_i);
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

	ocrpt_ignore_child_nodes(o, reader, "Datasource");
}

static void ocrpt_parse_datasources_node(opencreport *o, xmlTextReaderPtr reader) {
	int depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
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
	}
}

static void ocrpt_parse_variable_node(opencreport *o, ocrpt_report *r, xmlTextReaderPtr reader) {
	xmlChar *name = xmlTextReaderGetAttribute(reader, (const xmlChar *)"name");
	xmlChar *value = xmlTextReaderGetAttribute(reader, (const xmlChar *)"value");
	xmlChar *type = xmlTextReaderGetAttribute(reader, (const xmlChar *)"type");
	xmlChar *resetonbreak = xmlTextReaderGetAttribute(reader, (const xmlChar *)"resetonbreak");
	xmlChar *precalculate = xmlTextReaderGetAttribute(reader, (const xmlChar *)"precalculate");
	ocrpt_var_type vtype = OCRPT_VARIABLE_EXPRESSION; /* default if left out */

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
	else
		fprintf(stderr, "unset or invalid type for variable declaration for v.'%s', using \"expression\"\n", name);

	if (value) {
		ocrpt_var *v = ocrpt_variable_new(o, r, vtype, (char *)name, (char *)value, (char *)resetonbreak);

		if (precalculate) {
			ocrpt_expr *p;
			char *p_s = NULL;
			int32_t p_i = 0;

			p = ocrpt_xml_expr_parse(o, precalculate, true);
			ocrpt_expr_get_value(o, p, &p_s, &p_i);
			if (p_s && !p_i)
				p_i = atoi(p_s);
			ocrpt_variable_set_precalculate(v, !!p_i);
			ocrpt_expr_free(o, r, p);
		}
	}

	xmlFree(name);
	xmlFree(value);
	xmlFree(type);
	xmlFree(resetonbreak);
	xmlFree(precalculate);

	ocrpt_ignore_child_nodes(o, reader, "Variable");
}

static void ocrpt_parse_variables_node(opencreport *o, ocrpt_report *r, xmlTextReaderPtr reader) {
	int depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		nodetype = xmlTextReaderNodeType(reader);

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

static bool ocrpt_parse_breakfield_node(opencreport *o, ocrpt_report *r, ocrpt_break *br, xmlTextReaderPtr reader) {
	xmlChar *value = xmlTextReaderGetAttribute(reader, (const xmlChar *)"value");
	bool have_breakfield = false;

	if (value) {
		ocrpt_expr *e = ocrpt_expr_parse(o, r, (char *)value, NULL);
		have_breakfield = ocrpt_break_add_breakfield(o, r, br, e);
	}

	xmlFree(value);

	ocrpt_ignore_child_nodes(o, reader, "BreakField");

	return have_breakfield;
}

static bool ocrpt_parse_breakfields_node(opencreport *o, ocrpt_report *r, ocrpt_break *br, xmlTextReaderPtr reader) {
	int depth, nodetype;
	bool have_breakfield = false;

	/* No BreakField sub-element */
	if (xmlTextReaderIsEmptyElement(reader))
		return false;

	depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		nodetype = xmlTextReaderNodeType(reader);

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
	int depth, nodetype;
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

	depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Break")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "BreakFields"))
				have_breakfield = ocrpt_parse_breakfields_node(o, r, br, reader) || have_breakfield;
#if 0
			else if (!strcmp((char *)name, "BreakHeader"))
				ocrpt_parse_alternate_node(o, r, reader);
			else if (!strcmp((char *)name, "BreakFooter"))
				ocrpt_parse_nodata_node(o, r, reader);
#endif
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
				ocrpt_expr *e = ocrpt_xml_expr_parse(o, attribs[i].value, true);
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
	int depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		nodetype = xmlTextReaderNodeType(reader);

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

static void ocrpt_parse_report_node(opencreport *o, ocrpt_part *p, xmlTextReaderPtr reader) {
	/* TODO: parse and process Report node attributes; make it possible for multiple pd nodes to exist in the same pr node */
	ocrpt_report *r = ocrpt_report_new(o);
	int depth, nodetype;

	ocrpt_part_append_report(o, p, r);

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Report")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Variables"))
				ocrpt_parse_variables_node(o, r, reader);
			else if (!strcmp((char *)name, "Breaks"))
				ocrpt_parse_breaks_node(o, r, reader);
#if 0
			if (!strcmp((char *)name, "Alternate"))
				ocrpt_parse_alternate_node(o, r, reader);
			else if (!strcmp((char *)name, "NoData"))
				ocrpt_parse_nodata_node(o, r, reader);
			else if (!strcmp((char *)name, "PageHeader"))
				ocrpt_parse_pageheader_node(o, p, r, reader);
			else if (!strcmp((char *)name, "PageFooter"))
				ocrpt_parse_pagefooter_node(o, p, r, reader);
			else if (!strcmp((char *)name, "ReportHeader"))
				ocrpt_parse_reportheader_node(o, p, r, reader);
			else if (!strcmp((char *)name, "ReportFooter"))
				ocrpt_parse_reportfooter_node(o, p, r, reader);
			else if (!strcmp((char *)name, "Detail"))
				ocrpt_parse_detail_node(o, r, reader);
			else if (!strcmp((char *)name, "Graph"))
				ocrpt_parse_graph_node(o, r, reader);
			else if (!strcmp((char *)name, "Chart"))
				ocrpt_parse_chart_node(o, r, reader);
#endif
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_pd_node(opencreport *o, ocrpt_part *p, xmlTextReaderPtr reader) {
	/* TODO: parse and process pd node attributes */
	int depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "pd")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, p, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_partrow_node(opencreport *o, ocrpt_part *p, xmlTextReaderPtr reader) {
	/* TODO: parse and process pr node attributes */
	int depth, nodetype;

	ocrpt_part_new_row(o, p);

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "pr")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "pd"))
				ocrpt_parse_pd_node(o, p, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_part_node(opencreport *o, xmlTextReaderPtr reader) {
	/* TODO: parse and process Part node attributes */
	ocrpt_part *p = ocrpt_part_new(o);
	int depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Part")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "pr"))
				ocrpt_parse_partrow_node(o, p, reader);
#if 0
			else if (!strcmp((char *)name, "PageHeader"))
				ocrpt_parse_pageheader_node(o, p, NULL, reader);
			else if (!strcmp((char *)name, "PageFooter"))
				ocrpt_parse_pagefooter_node(o, p, NULL, reader);
			else if (!strcmp((char *)name, "ReportHeader"))
				ocrpt_parse_reportheader_node(o, p, NULL, reader);
			else if (!strcmp((char *)name, "ReportFooter"))
				ocrpt_parse_reportfooter_node(o, p, NULL, reader);
#endif
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_opencreport_node(opencreport *o, xmlTextReaderPtr reader) {
	int depth, nodetype;

	if (xmlTextReaderIsEmptyElement(reader))
		return;

	depth = xmlTextReaderDepth(reader);

	while (xmlTextReaderRead(reader) == 1) {
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
			else if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, NULL, reader);
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
				ocrpt_parse_report_node(o, NULL, reader);
			else if (!strcmp((char *)name, "Part"))
				ocrpt_parse_part_node(o, reader);
		}

		xmlFree(name);
	}

	xmlFreeTextReader(reader);

	return retval;
}
