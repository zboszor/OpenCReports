/*
 * OpenCReports JSON output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_JSON_H_
#define _OCRPT_JSON_H_

#include "opencreport.h"
#include "common-output.h"

struct json_private_data {
	common_private_data base;
};
typedef struct json_private_data json_private_data;

void ocrpt_json_init(opencreport *o);

#endif
