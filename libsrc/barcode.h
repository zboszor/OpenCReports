/*
 * OpenCReports barcode rendering module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _BARCODE_H_
#define _BARCODE_H_

#include <stdbool.h>
#include <opencreport.h>

#define INK_SPREADING_TOLERANCE (0.15)

bool ocrpt_barcode_encode(opencreport *o, ocrpt_barcode *barcode);

#endif
