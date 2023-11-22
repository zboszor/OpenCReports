/*
 * OpenCReports main header
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
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

static void ocrpt_parse_output_image_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, xmlTextReaderPtr reader);

static void processNode(xmlTextReaderPtr reader) __attribute__((unused));
static void processNode(xmlTextReaderPtr reader) {
	xmlChar *name, *value;

	name = xmlTextReaderName(reader);
	if (name == NULL)
		name = xmlStrdup(BAD_CAST "--");
	value = xmlTextReaderValue(reader);

	ocrpt_err_printf("%d %d %s %d",
			xmlTextReaderDepth(reader),
			xmlTextReaderNodeType(reader),
			name,
			xmlTextReaderIsEmptyElement(reader));
	xmlFree(name);
	if (value == NULL)
		ocrpt_err_printf("\n");
	else {
		ocrpt_err_printf(" %s\n", value);
		xmlFree(value);
	}
}

#define get_string(o, expr) { \
					expr##_e = ocrpt_expr_parse(o, (char *)expr, NULL); \
					ocrpt_expr_resolve_nowarn(expr##_e); \
					const ocrpt_string *expr##_ss = ocrpt_expr_get_string(expr##_e); \
					expr##_s = expr##_ss ? expr##_ss->str : (char *)expr; \
				}

#define get_int(o, expr) { \
					expr##_e = ocrpt_expr_parse(o, (char *)expr, NULL); \
					ocrpt_expr_resolve_nowarn(expr##_e); \
					expr##_i = ocrpt_expr_get_long(expr##_e); \
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

	get_string(o, name);
	get_string(o, value);
	get_string(o, datasource);
	get_string(o, follower_for);
	get_string(o, follower_expr);

	get_int(o, cols);
	get_int(o, rows);

	get_string(o, coltypes);

	ds = ocrpt_datasource_get(o, datasource_s);
	if (ds) {
		int32_t ct_cols_i = cols_i;

		if (ds->input == &ocrpt_array_input) {
			ocrpt_query_discover_array(value_s, &arrayptr, &rows_i, &cols_i, coltypes_s, &coltypesptr, &ct_cols_i);
			if (arrayptr)
				q = ocrpt_query_add_array(ds, name_s, arrayptr, rows_i, cols_i, coltypesptr, ct_cols_i);
			else
				ocrpt_err_printf("Cannot determine array pointer for array query\n");
		} else if (ds->input == &ocrpt_csv_input) {
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, coltypes_s, &coltypesptr, &ct_cols_i);
			q = ocrpt_query_add_csv(ds, name_s, value_s, coltypesptr, ct_cols_i);
		} else if (ds->input == &ocrpt_json_input) {
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, coltypes_s, &coltypesptr, &ct_cols_i);
			q = ocrpt_query_add_json(ds, name_s, value_s, coltypesptr, ct_cols_i);
		} else if (ds->input == &ocrpt_xml_input) {
			ocrpt_query_discover_array(NULL, NULL, NULL, NULL, coltypes_s, &coltypesptr, &ct_cols_i);
			q = ocrpt_query_add_xml(ds, name_s, value_s, coltypesptr, ct_cols_i);
		} else if (ds->input == &ocrpt_postgresql_input)
			q = ocrpt_query_add_postgresql(ds, name_s, value_s);
		else if (ds->input == &ocrpt_mariadb_input)
			q = ocrpt_query_add_mariadb(ds, name_s, value_s);
		else if (ds->input == &ocrpt_odbc_input)
			q = ocrpt_query_add_odbc(ds, name_s, value_s);
		else {
			/* TODO: externally defined  input driver */
		}
	}

	if (q) {
		if (follower_for)
			lq = ocrpt_query_get(o, follower_for_s);

		if (lq) {
			if (follower_expr) {
				char *err = NULL;
				ocrpt_expr *e = ocrpt_expr_parse(o, follower_expr_s, &err);

				if (e)
					ocrpt_query_add_follower_n_to_1(lq, q, e);
				else
					ocrpt_err_printf("Cannot parse matching expression between queries \"%s\" and \"%s\": \"%s\"\n", follower_for_s, name_s, follower_expr);
			} else
				ocrpt_query_add_follower(lq, q);
		}
	} else
		ocrpt_err_printf("cannot add query \"%s\"\n", name_s);


	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	if (value != value_att)
		xmlFree(value);

	ocrpt_expr_free(name_e);
	ocrpt_expr_free(datasource_e);
	ocrpt_expr_free(value_e);
	ocrpt_expr_free(follower_for_e);
	ocrpt_expr_free(follower_expr_e);
	ocrpt_expr_free(cols_e);
	ocrpt_expr_free(rows_e);
	ocrpt_expr_free(coltypes_e);
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
	xmlChar *connstr, *optionfile, *group, *encoding;
	ocrpt_expr *name_e, *type_e, *host_e, *unix_socket_e;
	ocrpt_expr *port_e, *dbname_e, *user_e, *password_e;
	ocrpt_expr *connstr_e, *optionfile_e, *group_e, *encoding_e;
	char *name_s, *type_s, *host_s, *unix_socket_s;
	char *port_s, *dbname_s, *user_s, *password_s;
	char *connstr_s, *optionfile_s, *group_s, *encoding_s;
	int32_t port_i, i;
	ocrpt_datasource *ds = NULL;

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
		{ "encoding", &encoding },
		{ NULL, NULL },
	};

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs);

	get_string(o, name);
	get_string(o, type);

	get_string(o, host);
	get_string(o, unix_socket);

	port_s = NULL;
	get_int(o, port);
	if (port_i) {
		port_s = alloca(32);
		sprintf(port_s, "%d", port_i);
	}

	/*
	 * Don't report parse errors for database connection details,
	 * they are sensitive information.
	 */
	get_string(o, dbname);
	get_string(o, user);
	get_string(o, password);
	get_string(o, connstr);
	get_string(o, optionfile);
	get_string(o, group);
	get_string(o, encoding);

	if (name_s && type_s) {
		if (!strcmp(type_s, "array"))
			ds = ocrpt_datasource_add_array(o, name_s);
		else if (!strcmp(type_s, "csv"))
			ds = ocrpt_datasource_add_csv(o, name_s);
		else if (!strcmp(type_s, "json"))
			ds = ocrpt_datasource_add_json(o, name_s);
		else if (!strcmp(type_s, "xml"))
			ds = ocrpt_datasource_add_xml(o, name_s);
		else if (!strcmp(type_s, "postgresql")) {
			if (connstr_s)
				ds = ocrpt_datasource_add_postgresql2(o, name_s, connstr_s);
			else
				ds = ocrpt_datasource_add_postgresql(o, name_s, unix_socket_s ? unix_socket_s : host_s, port_s, dbname_s, user_s, password_s);
		} else if (!strcmp(type_s, "mariadb") || !strcmp(type_s, "mysql")) {
			if (group_s)
				ds = ocrpt_datasource_add_mariadb2(o, name_s, optionfile_s, group_s);
			else
				ds = ocrpt_datasource_add_mariadb(o, name_s, host_s, port_s, dbname_s, user_s, password_s, unix_socket_s);
		} else if (!strcmp(type_s, "odbc")) {
			if (connstr_s)
				ds = ocrpt_datasource_add_odbc2(o, name_s, connstr_s);
			else
				ds = ocrpt_datasource_add_odbc(o, name_s, dbname_s, user_s, password_s);
		}
	}

	if (encoding && encoding_s && ds)
		ocrpt_datasource_set_encoding(ds, encoding_s);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

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
	ocrpt_expr_free(encoding_e);

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
			ocrpt_err_printf("invalid type for variable declaration for v.'%s', using \"expression\"\n", name);
	}

	if (basetype) {
		if (strcasecmp((char *)basetype, "number") == 0 || strcasecmp((char *)basetype, "numeric") == 0)
			rtype = OCRPT_RESULT_NUMBER;
		else if (strcasecmp((char *)basetype, "string") == 0)
			rtype = OCRPT_RESULT_STRING;
		else if (strcasecmp((char *)basetype, "datetime") == 0)
			rtype = OCRPT_RESULT_DATETIME;
		else
			ocrpt_err_printf("invalid result type for custom variable declaration for v.'%s'\n", name);
	}

	if (baseexpr) {
		ocrpt_var *v;

		if (!intermedexpr && !intermed2expr && !resultexpr && vtype == OCRPT_VARIABLE_CUSTOM)
			vtype = OCRPT_VARIABLE_EXPRESSION;

		if (vtype == OCRPT_VARIABLE_CUSTOM)
			v = ocrpt_variable_new_full(r, rtype, (char *)name, (char *)baseexpr, (char *)intermedexpr, (char *)intermed2expr, (char *)resultexpr, (char *)resetonbreak);
		else
			v = ocrpt_variable_new(r, vtype, (char *)name, (char *)baseexpr, (char *)resetonbreak);

		if (precalculate)
			ocrpt_variable_set_precalculate(v, (char *)precalculate);
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
	ocrpt_text *elem = ocrpt_line_add_text(line);
	int ret;

	xmlChar *value, *delayed, *format, *width, *align;
	xmlChar *color, *bgcolor, *font_name, *font_size;
	xmlChar *bold, *italic, *link, *memo, *memo_wrap_chars, *memo_max_lines;
	xmlChar *translate;
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
		{ { "translate" }, &translate },
#if 0
		/* Ignored, accepted in opencreport.dtd for RLIB compatibility. */
		{ { "col" }, &col },
#endif
		{ { NULL }, NULL },
	};
	int32_t i, j;
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

	if (literal_string)
		ocrpt_text_set_value_string(elem, (char *)value);
	else
		ocrpt_text_set_value_expr(elem, (char *)value);

	if (delayed)
		ocrpt_text_set_value_delayed(elem, (char *)delayed);

	ocrpt_text_set_format(elem, (char *)format);
	ocrpt_text_set_width(elem, (char *)width);
	ocrpt_text_set_alignment(elem, (char *)align);
	ocrpt_text_set_color(elem, (char *)color);
	ocrpt_text_set_bgcolor(elem, (char *)bgcolor);
	ocrpt_text_set_font_name(elem, (char *)font_name);
	ocrpt_text_set_font_size(elem, (char *)font_size);
	ocrpt_text_set_bold(elem, (char *)bold);
	ocrpt_text_set_italic(elem, (char *)italic);
	ocrpt_text_set_link(elem, (char *)link);
	ocrpt_text_set_translate(elem, (char *)translate);

	ocrpt_text_set_memo(elem, (char *)memo);
	ocrpt_text_set_memo_wrap_chars(elem, (char *)memo_wrap_chars);
	ocrpt_text_set_memo_max_lines(elem, (char *)memo_max_lines);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	if (run_ignore_child)
		ocrpt_ignore_child_nodes(o, reader, depth, literal ? "literal" : "field");
}

static void ocrpt_parse_output_line_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, xmlTextReaderPtr reader) {
	ocrpt_line *line = ocrpt_output_add_line(output);

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

	ocrpt_line_set_font_name(line, (char *)font_name);
	ocrpt_line_set_font_size(line, (char *)font_size);
	ocrpt_line_set_bold(line, (char *)bold);
	ocrpt_line_set_italic(line, (char *)italic);
	ocrpt_line_set_suppress(line, (char *)suppress);
	ocrpt_line_set_color(line, (char *)color);
	ocrpt_line_set_bgcolor(line, (char *)bgcolor);

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
			else if (!strcmp((char *)name, "Image"))
				ocrpt_parse_output_image_node(o, r, output, line, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_output_hline_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, xmlTextReaderPtr reader) {
	ocrpt_hline *hline = ocrpt_output_add_hline(output);
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

	ocrpt_hline_set_size(hline, (char *)size);
	ocrpt_hline_set_indent(hline, (char *)indent);
	ocrpt_hline_set_length(hline, (char *)length);
	ocrpt_hline_set_font_size(hline, (char *)font_size);
	ocrpt_hline_set_suppress(hline, (char *)suppress);
	ocrpt_hline_set_color(hline, (char *)color);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	ocrpt_ignore_child_nodes(o, reader, -1, "HorizontalLine");
}

static void ocrpt_parse_output_image_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, ocrpt_line *line, xmlTextReaderPtr reader) {
	ocrpt_image *img;
	xmlChar *value, *suppress, *type, *width, *height, *align, *bgcolor, *text_width;
	struct {
		char *attrs[3];
		xmlChar **attrp;
	} xmlattrs[] = {
		{ { "value" }, &value },
		{ { "suppress" }, &suppress },
		{ { "type" }, &type },
		{ { "width" }, &width },
		{ { "height" }, &height },
		{ { "align" }, &align },
		{ { "bgcolor" }, &bgcolor },
		{ { "text_width", "textWidth" }, &text_width },
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

	if (line)
		img = ocrpt_line_add_image(line);
	else
		img = ocrpt_output_add_image(output);

	ocrpt_image_set_value(img, (char *)value);
	ocrpt_image_set_suppress(img, (char *)suppress);
	ocrpt_image_set_type(img, (char *)type);
	ocrpt_image_set_width(img, (char *)width);
	ocrpt_image_set_height(img, (char *)height);
	ocrpt_image_set_alignment(img, (char *)align);
	ocrpt_image_set_bgcolor(img, (char *)bgcolor);
	ocrpt_image_set_text_width(img, (char *)text_width);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	ocrpt_ignore_child_nodes(o, reader, -1, "Image");
}

static void ocrpt_parse_output_imageend_node(opencreport *o, ocrpt_report *r, ocrpt_output *output, xmlTextReaderPtr reader) {
	ocrpt_output_add_image_end(output);

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

	ocrpt_output_set_suppress(output, (char *)suppress);

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
				ocrpt_parse_output_image_node(o, r, output, NULL, reader);
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
		ocrpt_expr *e = ocrpt_report_expr_parse(r, (char *)value, NULL);
		have_breakfield = ocrpt_break_add_breakfield(br, e);
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
	xmlChar *brname, *headernewpage, *suppressblank;
	ocrpt_break *br;
	bool have_breakfield = false;
	struct {
		char *attr;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "name", &brname },
#if 0
		/*
		 * "newpage" is a write-only setting in RLIB.
		 * OpenCReports accepts it in opencreport.dtd
		 * and just ignore it here, leaving this as
		 * code documentation.
		 */
		{ "newpage", &newpage },
#endif
		{ "headernewpage", &headernewpage },
		{ "suppressblank", &suppressblank },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attr);

	if (!brname) {
		ocrpt_err_printf("nameless break is useless, not adding to report\n");
		goto out;
	}

	/* There's no BreakFields sub-element, useless break. */
	if (xmlTextReaderIsEmptyElement(reader)) {
		ocrpt_err_printf("break '%s' is useless, not adding to report\n", (char *)brname);
		goto out;
	}

	br = ocrpt_break_new(r, (char *)brname);
	if (!br) {
		ocrpt_err_printf("Out of memory while allocating break '%s', not adding to report\n", (char *)brname);
		goto out;
	}

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
		ocrpt_break_set_headernewpage(br, (char *)headernewpage);
		ocrpt_break_set_suppressblank(br, (char *)suppressblank);
	} else {
		/* There's no BreakFields sub-element, useless break. */
		ocrpt_err_printf("break '%s' is useless, not adding to report\n", (char *)brname);
		ocrpt_break_free(br);
		r->breaks = ocrpt_list_remove(r->breaks, br);
	}

	out:
	for (i = 0; xmlattrs[i].attr; i++)
		xmlFree(*xmlattrs[i].attrp);
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

static ocrpt_report *ocrpt_parse_report_node(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, xmlTextReaderPtr reader, bool called_from_ocrpt_node) {
	if (xmlTextReaderIsEmptyElement(reader))
		return NULL;

	if (!pd) {
		if (!p)
			p = ocrpt_part_new(o);

		if (!pr)
			pr = ocrpt_part_new_row(p);

		pd = ocrpt_part_row_new_column(pr);
	}

	if (!r)
		r = ocrpt_part_column_new_report(pd);

	r->noquery_show_nodata = called_from_ocrpt_node;

	xmlChar *font_name, *font_size;
	xmlChar *size_unit, *noquery_show_nodata, *report_height_after_last, *follower_match_single;
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
		{ { "follower_match_single" }, &follower_match_single },
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

	if (!o->size_unit_expr) {
		if (!called_from_ocrpt_node)
			ocrpt_set_size_unit(o, size_unit ? (char *)size_unit : "'rlib'");
		else if (size_unit)
			ocrpt_set_size_unit(o, (char *)size_unit);
	}

	if (!o->noquery_show_nodata_expr) {
		if (!called_from_ocrpt_node)
			ocrpt_set_noquery_show_nodata(o, noquery_show_nodata ? (char *)noquery_show_nodata : "no");
		else if (noquery_show_nodata)
			ocrpt_set_noquery_show_nodata(o, (char *)noquery_show_nodata);
	}

	if (!o->report_height_after_last_expr) {
		if (!called_from_ocrpt_node)
			ocrpt_set_report_height_after_last(o, report_height_after_last ? (char *)report_height_after_last : "no");
		else if (report_height_after_last)
			ocrpt_set_report_height_after_last(o, (char *)report_height_after_last);
	}

	/* Simulate/approximate RLIB's buggy behaviour */
	if (!o->follower_match_single_expr) {
		if (!called_from_ocrpt_node)
			ocrpt_set_follower_match_single(o, follower_match_single ? (char *)follower_match_single : "yes");
		else if (follower_match_single)
			ocrpt_set_follower_match_single(o, (char *)follower_match_single);
	}

	if (font_name)
		ocrpt_report_set_font_name(r, (char *)font_name);

	if (font_size)
		ocrpt_report_set_font_size(r, (char *)font_size);

	if (height)
		ocrpt_report_set_height(r, (char *)height);

	if (!p->orientation_expr && orientation)
		ocrpt_part_set_orientation(p, (char *)orientation);

	if (!p->top_margin_expr && top_margin)
		ocrpt_part_set_top_margin(p, (char *)top_margin);

	if (!p->bottom_margin_expr && bottom_margin)
		ocrpt_part_set_bottom_margin(p, (char *)bottom_margin);

	if (!p->left_margin_expr && left_margin)
		ocrpt_part_set_left_margin(p, (char *)left_margin);

	if (!p->right_margin_expr && right_margin)
		ocrpt_part_set_right_margin(p, (char *)right_margin);

	if (!p->paper_type_expr && paper_type)
		ocrpt_part_set_paper_by_name(p, (char *)paper_type);

	if (iterations)
		ocrpt_report_set_iterations(r, (char *)iterations);

	if (!p->suppress_pageheader_firstpage_expr && suppress_pageheader_firstpage)
		ocrpt_part_set_suppress_pageheader_firstpage(p, (char *)suppress_pageheader_firstpage);

	if (suppress)
		ocrpt_report_set_suppress(r, (char *)suppress);

	if (query)
		ocrpt_report_set_main_query_from_expr(r, (char *)query);

	if (field_header_priority)
		ocrpt_report_set_fieldheader_priority(r, (char *)field_header_priority);

	if (!pd->border_width_expr && border_width)
		ocrpt_part_column_set_border_width(pd, (char *)border_width);

	if (!pd->border_color_expr && border_color)
		ocrpt_part_column_set_border_color(pd, (char *)border_color);

	if (!pd->detail_columns_expr && detail_columns)
		ocrpt_part_column_set_detail_columns(pd, (char *)detail_columns);

	if (!pd->column_pad_expr && column_pad)
		ocrpt_part_column_set_column_padding(pd, (char *)column_pad);

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
				if (p->pageheader.output_list) {
					ocrpt_ignore_child_nodes(o, reader, -1, "PageHeader");
					ocrpt_err_printf("Multiple <PageHeader> sections in the same <Part> section or in child <Report> sections\n");
				} else {
					ocrpt_layout_part_page_header_set_report(p, r);
					ocrpt_parse_output_parent_node(o, r, "PageHeader", &p->pageheader, reader);
				}
			} else if (!strcmp((char *)name, "PageFooter")) {
				if (p->pagefooter.output_list) {
					ocrpt_ignore_child_nodes(o, reader, -1, "PageFooter");
					ocrpt_err_printf("Multiple <PageFooter> sections in the same <Part> section or in child <Report> sections\n");
				} else {
					ocrpt_layout_part_page_footer_set_report(p, r);
					ocrpt_parse_output_parent_node(o, r, "PageFooter", &p->pagefooter, reader);
				}
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

bool ocrpt_parse_report_node_for_load(ocrpt_report *r) {
	const ocrpt_string *filename = ocrpt_expr_get_string(r->filename_expr);
	char *real_filename = ocrpt_find_file(r->o, filename->str);
	if (!real_filename) {
		ocrpt_err_printf("ocrpt_parse_report_node_for_load: can't find file %s\n", filename);
		return false;
	}

	ocrpt_expr *query = r->query_expr;
	r->query_expr = NULL;

	ocrpt_expr *iterations = r->iterations_expr;
	r->iterations_expr = NULL;

	xmlTextReaderPtr reader;
	ocrpt_report *r1 = NULL;

	reader = xmlReaderForFile(real_filename, NULL, XML_PARSE_RECOVER |
								XML_PARSE_NOENT | XML_PARSE_NOBLANKS |
								XML_PARSE_XINCLUDE | XML_PARSE_NOXINCNODE);
	ocrpt_mem_free(real_filename);
	if (!reader) {
		ocrpt_err_printf("ocrpt_parse_report_node_for_load: invalid XML file name or invalid contents\n");
		return false;
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
				r1 = ocrpt_parse_report_node(r->o, r->p, r->pr, r->pd, r, reader, r->called_from_ocrpt_node);
		}

		xmlFree(name);
	}

	xmlFreeTextReader(reader);

	if (r1) {
		if (query)
			r->query_expr = query;

		if (iterations)
			r->iterations_expr = iterations;

		return true;
	}

	return false;
}

static void ocrpt_parse_load(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, xmlTextReaderPtr reader_parent, bool called_from_ocrpt_node) {
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
		ocrpt_report *r = ocrpt_part_column_new_report(pd);

		r->p = p;
		r->pr = pr;
		r->pd = pd;
		r->called_from_ocrpt_node = called_from_ocrpt_node;

		r->filename_expr = ocrpt_expr_parse(o, (char *)filename, NULL);
		if (!r->filename_expr)
			r->filename_expr = ocrpt_newstring(o, NULL, (char *)filename);

		if (query)
			ocrpt_report_set_main_query_from_expr(r, (char *)query);

		if (iterations)
			ocrpt_report_set_iterations(r, (char *)iterations);
	}

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);
}

static void ocrpt_parse_pd_node(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, xmlTextReaderPtr reader, bool called_from_ocrpt_node) {
	ocrpt_part_column *pd = ocrpt_part_row_new_column(pr);

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

	if (width)
		ocrpt_part_column_set_width(pd, (char *)width);

	if (height)
		ocrpt_part_column_set_height(pd, (char *)height);

	if (border_width)
		ocrpt_part_column_set_border_width(pd, (char *)border_width);

	if (border_color)
		ocrpt_part_column_set_border_color(pd, (char *)border_color);

	if (detail_columns)
		ocrpt_part_column_set_detail_columns(pd, (char *)detail_columns);

	if (column_pad)
		ocrpt_part_column_set_column_padding(pd, (char *)column_pad);

	if (suppress)
		ocrpt_part_column_set_suppress(pd, (char *)suppress);

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
				ocrpt_parse_report_node(o, p, pr, pd, NULL, reader, called_from_ocrpt_node);
			else if (!strcmp((char *)name, "load"))
				ocrpt_parse_load(o, p, pr, pd, reader, called_from_ocrpt_node);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_part_row_node(opencreport *o, ocrpt_part *p, xmlTextReaderPtr reader, bool called_from_ocrpt_node) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	ocrpt_part_row *pr = ocrpt_part_new_row(p);

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

	if (layout)
		ocrpt_part_row_set_layout(pr, (char *)layout);

	if (newpage)
		ocrpt_part_row_set_newpage(pr, (char *)newpage);

	if (suppress)
		ocrpt_part_row_set_suppress(pr, (char *)suppress);

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
				ocrpt_parse_pd_node(o, p, pr, reader, called_from_ocrpt_node);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_part_node(opencreport *o, xmlTextReaderPtr reader, bool called_from_ocrpt_node) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	ocrpt_part *p = ocrpt_part_new(o);

	xmlChar *font_name, *font_size;
	xmlChar *size_unit, *noquery_show_nodata, *report_height_after_last, *follower_match_single;
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
		{ { "follower_match_single" }, &follower_match_single },
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

	if (!o->size_unit_expr) {
		if (!called_from_ocrpt_node)
			ocrpt_set_size_unit(o, size_unit ? (char *)size_unit : "'rlib'");
		else if (size_unit)
			ocrpt_set_size_unit(o, (char *)size_unit);
	}

	if (!o->noquery_show_nodata_expr) {
		if (!called_from_ocrpt_node)
			ocrpt_set_noquery_show_nodata(o, noquery_show_nodata ? (char *)noquery_show_nodata : "no");
		else if (noquery_show_nodata)
			ocrpt_set_noquery_show_nodata(o, (char *)noquery_show_nodata);
	}

	if (!o->report_height_after_last_expr) {
		if (!called_from_ocrpt_node)
			ocrpt_set_report_height_after_last(o, report_height_after_last ? (char *)report_height_after_last : "no");
		else if (report_height_after_last)
			ocrpt_set_report_height_after_last(o, (char *)report_height_after_last);
	}

	/* Simulate/approximate RLIB's buggy behaviour */
	if (!o->follower_match_single_expr) {
		if (!called_from_ocrpt_node)
			ocrpt_set_follower_match_single(o, follower_match_single ? (char *)follower_match_single : "yes");
		else if (follower_match_single)
			ocrpt_set_follower_match_single(o, (char *)follower_match_single);
	}

	if (!p->font_name_expr && font_name)
		ocrpt_part_set_font_name(p, (char *)font_name);

	if (!p->font_size_expr && font_size)
		ocrpt_part_set_font_size(p, (char *)font_size);

	if (orientation)
		ocrpt_part_set_orientation(p, (char *)orientation);

	if (top_margin)
		ocrpt_part_set_top_margin(p, (char *)top_margin);

	if (bottom_margin)
		ocrpt_part_set_bottom_margin(p, (char *)bottom_margin);

	if (left_margin)
		ocrpt_part_set_left_margin(p, (char *)left_margin);

	if (right_margin)
		ocrpt_part_set_right_margin(p, (char *)right_margin);

	if (!p->paper_type_expr && paper_type)
		ocrpt_part_set_paper_by_name(p, (char *)paper_type);

	if (iterations)
		ocrpt_part_set_iterations(p, (char *)iterations);

	if (suppress)
		ocrpt_part_set_suppress(p, (char *)suppress);

	if (suppress_pageheader_firstpage)
		ocrpt_part_set_suppress_pageheader_firstpage(p, (char *)suppress_pageheader_firstpage);

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
				ocrpt_parse_part_row_node(o, p, reader, called_from_ocrpt_node);
			else if (!strcmp((char *)name, "PageHeader")) {
				if (p->pageheader.output_list) {
					ocrpt_ignore_child_nodes(o, reader, -1, "PageHeader");
					ocrpt_err_printf("Multiple <PageHeader> sections in the same <Part> section or in child <Report> sections\n");
				} else
					ocrpt_parse_output_parent_node(o, NULL, "PageHeader", &p->pageheader, reader);
			} else if (!strcmp((char *)name, "PageFooter")) {
				if (p->pagefooter.output_list) {
					ocrpt_ignore_child_nodes(o, reader, -1, "PageFooter");
					ocrpt_err_printf("Multiple <PageFooter> sections in the same <Part> section or in child <Report> sections\n");
				} else
					ocrpt_parse_output_parent_node(o, NULL, "PageFooter", &p->pagefooter, reader);
			}
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_path_node(opencreport *o, xmlTextReaderPtr reader) {
	xmlChar *value = NULL;

	struct {
		char *attrs;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "value", &value },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs);

	if (value)
		ocrpt_add_search_path_from_expr(o, (char *)value);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

	ocrpt_ignore_child_nodes(o, reader, -1, "Path");
}

static void ocrpt_parse_paths_node(opencreport *o, xmlTextReaderPtr reader) {
	if (xmlTextReaderIsEmptyElement(reader))
		return;

	int depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);

		//processNode(reader);

		int nodetype = xmlTextReaderNodeType(reader);

		if (nodetype == XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader) && !strcmp((char *)name, "Paths")) {
			xmlFree(name);
			break;
		}

		if (nodetype == XML_READER_TYPE_ELEMENT) {
			if (!strcmp((char *)name, "Path"))
				ocrpt_parse_path_node(o, reader);
		}

		xmlFree(name);
	}
}

static void ocrpt_parse_opencreport_node(opencreport *o, xmlTextReaderPtr reader) {
	xmlChar *size_unit, *noquery_show_nodata, *report_height_after_last;
	xmlChar *follower_match_single, *precision_bits, *rounding_mode;
	xmlChar *locale, *xlate_domain, *xlate_dir;

	struct {
		char *attrs;
		xmlChar **attrp;
	} xmlattrs[] = {
		{ "size_unit", &size_unit },
		{ "noquery_show_nodata", &noquery_show_nodata },
		{ "report_height_after_last", &report_height_after_last },
		{ "follower_match_single", &follower_match_single },
		{ "precision_bits", &precision_bits },
		{ "rounding_mode", &rounding_mode },
		{ "locale", &locale },
		{ "translation_domain", &xlate_domain },
		{ "translation_directory", &xlate_dir },
		{ NULL, NULL },
	};
	int32_t i;

	for (i = 0; xmlattrs[i].attrp; i++)
		*xmlattrs[i].attrp = xmlTextReaderGetAttribute(reader, (const xmlChar *)xmlattrs[i].attrs);

	if (!o->size_unit_expr)
		ocrpt_set_size_unit(o, size_unit ? (char *)size_unit : "'rlib'");

	if (!o->noquery_show_nodata_expr)
		ocrpt_set_noquery_show_nodata(o, noquery_show_nodata ? (char *)noquery_show_nodata : "yes");

	if (!o->report_height_after_last_expr)
		ocrpt_set_report_height_after_last(o, report_height_after_last ? (char *)report_height_after_last : "no");

	if (!o->follower_match_single_expr)
		ocrpt_set_follower_match_single(o, follower_match_single ? (char *)follower_match_single : "no");

	if (!o->precision_expr && precision_bits)
		ocrpt_set_numeric_precision_bits(o, (char *)precision_bits);

	if (!o->rounding_mode_expr && rounding_mode)
		ocrpt_set_rounding_mode(o, (char *)rounding_mode);

	if (locale)
		ocrpt_set_locale_from_expr(o, (char *)locale);

	if (xlate_domain && xlate_dir)
		ocrpt_bindtextdomain_from_expr(o, (char *)xlate_domain, (char *)xlate_dir);

	for (i = 0; xmlattrs[i].attrp; i++)
		xmlFree(*xmlattrs[i].attrp);

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
			if (!strcmp((char *)name, "Paths"))
				ocrpt_parse_paths_node(o, reader);
			else if (!strcmp((char *)name, "Datasources"))
				ocrpt_parse_datasources_node(o, reader);
			else if (!strcmp((char *)name, "Queries"))
				ocrpt_parse_queries_node(o, reader);
			else if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, NULL, NULL, NULL, NULL, reader, true);
			else if (!strcmp((char *)name, "Part"))
				ocrpt_parse_part_node(o, reader, true);
		}

		xmlFree(name);
	}
}

static bool ocrpt_parse_xml_internal(opencreport *o, xmlTextReaderPtr reader) {
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
			else if (!strcmp((char *)name, "Paths"))
				ocrpt_parse_paths_node(o, reader);
			else if (!strcmp((char *)name, "Datasources"))
				ocrpt_parse_datasources_node(o, reader);
			else if (!strcmp((char *)name, "Datasource"))
				ocrpt_parse_datasource_node(o, reader);
			else if (!strcmp((char *)name, "Queries"))
				ocrpt_parse_queries_node(o, reader);
			else if (!strcmp((char *)name, "Query"))
				ocrpt_parse_query_node(o, reader);
			else if (!strcmp((char *)name, "Report"))
				ocrpt_parse_report_node(o, NULL, NULL, NULL, NULL, reader, false);
			else if (!strcmp((char *)name, "Part"))
				ocrpt_parse_part_node(o, reader, false);
		}

		xmlFree(name);
	}

	xmlFreeTextReader(reader);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_parse_xml(opencreport *o, const char *filename) {
	if (!o || !filename)
		return false;

	xmlTextReaderPtr reader = xmlReaderForFile(filename, NULL,
												XML_PARSE_RECOVER |
												XML_PARSE_NOENT |
												XML_PARSE_NOBLANKS |
												XML_PARSE_XINCLUDE |
												XML_PARSE_NOXINCNODE);

	return ocrpt_parse_xml_internal(o, reader);
}

DLL_EXPORT_SYM bool ocrpt_parse_xml_from_buffer(opencreport *o, const char *buffer, size_t size) {
	if (!o || !buffer)
		return false;

	xmlTextReaderPtr reader = xmlReaderForMemory(buffer, size, NULL, NULL,
												XML_PARSE_RECOVER |
												XML_PARSE_NOENT |
												XML_PARSE_NOBLANKS |
												XML_PARSE_XINCLUDE |
												XML_PARSE_NOXINCNODE);

	return ocrpt_parse_xml_internal(o, reader);
}
