/*
 * OpenCReports JSON output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_JSON_H_
#define _OCRPT_JSON_H_

#include "opencreport.h"

struct json_private_data {
	ocrpt_string *data;
	ocrpt_list *pages;
	ocrpt_list *last_page;
	ocrpt_list *current_page;
};
typedef struct json_private_data json_private_data;

void ocrpt_json_init(opencreport *o);

#endif
