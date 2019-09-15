/*
 * OpenCReports main module
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORTS_FUNCTIONS_H_
#define _OPENCREPORTS_FUNCTIONS_H_

#include "opencreport.h"
#include "exprutil.h"

typedef ocrpt_expr *(*ocrpt_function_call)(opencreport *, int, ocrpt_expr *);

struct ocrpt_function {
	const char *fname;
	ocrpt_function_call func;
};

typedef struct ocrpt_function ocrpt_function;

extern ocrpt_function *ocrpt_find_function(const char *fname);

#endif
