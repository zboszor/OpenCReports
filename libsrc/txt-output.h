/*
 * OpenCReports TXT output driver
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_TXT_H_
#define _OCRPT_TXT_H_

#include <stdint.h>

#include "opencreport.h"
#include "common-output.h"

struct txt_private_data {
	common_private_data base;
	ocrpt_string *bgimagepfx;
	ocrpt_string *spc_padding;
};
typedef struct txt_private_data txt_private_data;

void ocrpt_txt_init(opencreport *o);

#endif
