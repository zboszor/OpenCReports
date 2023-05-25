/*
 * OpenCReports XML output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_XML_H_
#define _OCRPT_XML_H_

#include "opencreport.h"

struct xml_private_data {
	ocrpt_string *data;
	ocrpt_list *pages;
	ocrpt_list *last_page;
	ocrpt_list *current_page;
};
typedef struct xml_private_data xml_private_data;

void ocrpt_xml_init(opencreport *o);

#endif
