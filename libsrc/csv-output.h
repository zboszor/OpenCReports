/*
 * OpenCReports CSV output driver
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_CSV_H_
#define _OCRPT_CSV_H_

#include <stdint.h>

#include "opencreport.h"
#include "common-output.h"

struct csv_private_data {
	common_private_data base;
	uint32_t column_index;
};
typedef struct csv_private_data csv_private_data;

void ocrpt_csv_init(opencreport *o);

#endif
