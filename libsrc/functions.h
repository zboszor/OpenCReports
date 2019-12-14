/*
 * OpenCReports main module
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORTS_FUNCTIONS_H_
#define _OPENCREPORTS_FUNCTIONS_H_

#include <stdbool.h>
#include <stdio.h>
#include <execinfo.h>
#include "exprutil.h"
#include "opencreport-private.h"

ocrpt_function *ocrpt_find_function(const char *fname);
bool ocrpt_init_func_result(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type);

#endif
