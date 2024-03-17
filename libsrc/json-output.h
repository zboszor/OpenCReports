/*
 * OpenCReports JSON output driver
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_JSON_H_
#define _OCRPT_JSON_H_

#include <yajl/yajl_gen.h>

#include "opencreport.h"
#include "common-output.h"

struct json_private_data {
	common_private_data base;
	yajl_gen yajl_gen;
	ocrpt_list *backlog;
	char *cwd;
	size_t cwdlen;
	double image_indent;
};
typedef struct json_private_data json_private_data;

typedef enum {
	part_row,
	part_column,
	part_report
} json_backlog_type;

struct json_backlog_data {
	json_backlog_type type;
	opencreport *o;
	ocrpt_part *p;
	ocrpt_part_row *pr;
	ocrpt_part_column *pd;
	ocrpt_report *r;
};
typedef struct json_backlog_data json_backlog_data;

void ocrpt_json_init(opencreport *o);

#endif
