/*
 * OpenCReports output driver common code
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_COMMON_OUTPUT_H_
#define _OCRPT_COMMON_OUTPUT_H_

#include <libxml/tree.h>
#include "opencreport.h"

struct common_private_data {
	ocrpt_list *pages;
	ocrpt_list *last_page;
	ocrpt_list *current_page;
	cairo_surface_t *nullpage_cs;
	cairo_t *cr;
	ocrpt_string *data;
};
typedef struct common_private_data common_private_data;

void ocrpt_common_init(opencreport *o, size_t privsz, size_t datasz, size_t outbufsz);
void ocrpt_common_finalize(opencreport *o);

#endif
