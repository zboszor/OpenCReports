/*
 * OpenCReports TXT output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_TXT_H_
#define _OCRPT_TXT_H_

#include "opencreport.h"

struct txt_private_data {
	ocrpt_string *data;
	ocrpt_list *pages;
	ocrpt_list *last_page;
	ocrpt_list *current_page;
};
typedef struct txt_private_data txt_private_data;

void ocrpt_txt_init(opencreport *o);

#endif
