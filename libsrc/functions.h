/*
 * OpenCReports main module
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORTS_FUNCTIONS_H_
#define _OPENCREPORTS_FUNCTIONS_H_

#include <stdbool.h>

#include "opencreport.h"
#include "exprutil.h"

typedef void (*ocrpt_function_call)(opencreport *, ocrpt_expr *);

struct ocrpt_function {
	const char *fname;
	const int n_ops;
	const bool commutative;
	const bool associative;
	ocrpt_function_call func;
};

typedef struct ocrpt_function ocrpt_function;

extern ocrpt_function *ocrpt_find_function(const char *fname);

#endif
