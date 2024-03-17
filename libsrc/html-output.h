/*
 * OpenCReports HTML output driver
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_HTML_H_
#define _OCRPT_HTML_H_

#include <cairo.h>
#include "opencreport.h"
#include "common-output.h"

struct html_private_data {
	common_private_data base;
	char *cwd;
	size_t cwdlen;
	double image_indent;
	double image_height;
	double curr_pos_from_last_image;
	ocrpt_string *png;
	ocrpt_string *pngbase64;
};
typedef struct html_private_data html_private_data;

void ocrpt_html_init(opencreport *o);

#endif
