/*
 * OpenCReports CSV output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_CSV_H_
#define _OCRPT_CSV_H_

#include <stdint.h>

#include "opencreport.h"

struct csv_private_data {
	ocrpt_string *data;
	ocrpt_list *pages;
	ocrpt_list *last_page;
	ocrpt_list *current_page;
	uint32_t column_index;
};
typedef struct csv_private_data csv_private_data;

void ocrpt_csv_init(opencreport *o);

#endif
