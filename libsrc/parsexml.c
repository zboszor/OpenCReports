/*
 * OpenCReports main header
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

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

static void ocrpt_parse_query_node(opencreport *o, xmlTextReaderPtr reader) {
	xmlChar *name = xmlTextReaderGetAttribute(reader, (const xmlChar *)"name");
	xmlChar *datasource = xmlTextReaderGetAttribute(reader, (const xmlChar *)"datasource");
	xmlChar *value = xmlTextReaderReadString(reader);
	ocrpt_datasource *ds;
	int ret, depth, nodetype;

	/*
	 * A query's value (i.e. an SQL query string) is preferred
	 * to be in the text element but it's accepted as a value="..."
	 * attribute, too.
	 */

	if (!value)
		value = xmlTextReaderGetAttribute(reader, (const xmlChar *)"value");

	ds = ocrpt_datasource_find(o, (char *)datasource);
	if (ds->input == &ocrpt_array_input) {
		/* Array datasource */
		xmlChar *cols = xmlTextReaderGetAttribute(reader, (const xmlChar *)"cols");
		xmlChar *rows = xmlTextReaderGetAttribute(reader, (const xmlChar *)"rows");
		xmlChar *coltypes = xmlTextReaderGetAttribute(reader, (const xmlChar *)"coltypes");
		void *arrayptr, *coltypesptr;
		int32_t cols1, rows1;

		cols1 = atoi((char *)cols);
		rows1 = atoi((char *)rows);

		ocrpt_query_discover_array((char *)value, &arrayptr, (char *)coltypes, &coltypesptr);
		if (arrayptr)
			ocrpt_query_add_array(o, ds, (char *)name, (const char **)arrayptr, rows1, cols1, coltypesptr);
		else
			fprintf(stderr, "Cannot determine array pointer for array query\n");

		xmlFree(rows);
		xmlFree(cols);
		xmlFree(coltypes);
	}

	/*
	 * TODO: implement adding XML, JSON, CVS, PostgreSQL,
	 * MariaDB and ODBC queries
	 */

	xmlFree(name);
	xmlFree(datasource);
	xmlFree(value);

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
	int ret, depth, nodetype;

	/*
	 * TODO: implement:
	 * - adding XML, JSON and CVS datasources, and
	 * - connecting to PostgreSQL, MariaDB and ODBC datasources
	 */
	if (!strcmp((char *)type, "array"))
		ocrpt_datasource_add_array(o, (char *)name);

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
