/*
 * OpenCReports color handling
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _OCRPT_COLOR_H_
#define _OCRPT_COLOR_H_

#include "opencreport.h"

struct ocrpt_named_color {
	const char *name;
	const char *html;
	ocrpt_color c;
};
typedef struct ocrpt_named_color ocrpt_named_color;

void ocrpt_init_color(void);

#endif
