/*
 * OpenCReports HTML output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_HTML_H_
#define _OCRPT_HTML_H_

#include <cairo.h>
#include "opencreport.h"

struct html_private_data {
	ocrpt_list *pages;
	ocrpt_list *last_page;
	ocrpt_list *current_page;
	cairo_surface_t *nullpage_cs;
	cairo_t *cr;
	ocrpt_string *data;
	char *cwd;
	size_t cwdlen;
	double image_indent;
};
typedef struct html_private_data html_private_data;

void ocrpt_html_init(opencreport *o);

#endif
