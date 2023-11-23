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
	ocrpt_list *drawing_page;
	cairo_surface_t *nullpage_cs;
	cairo_t *cr;
	ocrpt_string *data;
};
typedef struct common_private_data common_private_data;

static inline unsigned char ocrpt_common_color_value(double component) {
	if (component < 0.0)
		component = 0.0;
	if (component >= 1.0)
		component = 1.0;

	return (unsigned char)(0xff * component);
}

void ocrpt_common_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width);
void ocrpt_common_get_text_sizes(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, bool last, double total_width);
void ocrpt_common_init(opencreport *o, size_t privsz, size_t datasz, size_t outbufsz);
void ocrpt_common_finalize(opencreport *o);

#endif
