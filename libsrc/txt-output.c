/*
 * OpenCReports main module
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>

#include "ocrpt-private.h"
#include "txt-output.h"

void ocrpt_txt_init(opencreport *o) {
	memset(&o->output_functions, 0, sizeof(ocrpt_output_functions));
}
