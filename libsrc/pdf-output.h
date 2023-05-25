/*
 * OpenCReports PDF output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_PDF_H_
#define _OCRPT_PDF_H_

#include <cairo.h>
#include "opencreport.h"

struct pdf_private_data {
	ocrpt_list *pages;
	ocrpt_list *last_page;
	ocrpt_list *current_page;
	ocrpt_list *drawing_page;
	cairo_surface_t *nullpage_cs;
	cairo_t *cr;
};
typedef struct pdf_private_data pdf_private_data;

void ocrpt_pdf_init(opencreport *o);

#endif
