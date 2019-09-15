/*
 * OpenCReports main module
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "opencreport.h"
#include "functions.h"

static ocrpt_function ocrpt_functions[] = {
	{ "abs",		NULL },
	{ "add",		NULL },
	{ "and",		NULL },
	{ "dec",		NULL },
	{ "div",		NULL },
	{ "eq",			NULL },
	{ "factorial",	NULL },
	{ "ge",			NULL },
	{ "gt",			NULL },
	{ "iif",		NULL },
	{ "inc",		NULL },
	{ "land",		NULL },
	{ "le",			NULL },
	{ "lnot",		NULL },
	{ "lor",		NULL },
	{ "lt",			NULL },
	{ "mod",		NULL },
	{ "mul",		NULL },
	{ "ne",			NULL },
	{ "not",		NULL },
	{ "or",			NULL },
	{ "shl",		NULL },
	{ "shr",		NULL },
	{ "sub",		NULL },
	{ "uminus",		NULL },
	{ "xor",		NULL },
};

static int n_ocrpt_functions = sizeof(ocrpt_functions) / sizeof(ocrpt_function);

static int funccmp(const void *key, const void *f) {
	return strcasecmp((const char *)key, ((const ocrpt_function *)f)->fname);
}

ocrpt_function *ocrpt_find_function(const char *fname) {
	return bsearch(fname, ocrpt_functions, n_ocrpt_functions, sizeof(ocrpt_function), funccmp);			
}
