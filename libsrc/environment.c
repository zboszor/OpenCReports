/*
 * Expression tree utilities
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "datasource.h"

DLL_EXPORT_SYM ocrpt_env_query_func ocrpt_env_get = ocrpt_env_get_c;

DLL_EXPORT_SYM void ocrpt_env_set_query_func(ocrpt_env_query_func func) {
	if (!func)
		return;

	ocrpt_env_get = func;
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_env_get_c(opencreport *o, const char *env) {
	if (!env)
		return NULL;

	ocrpt_result *result = ocrpt_result_new(o);
	if (!result)
		return NULL;

	char *value = getenv(env);
	result->type = OCRPT_RESULT_STRING;
	if (value) {
		result->string = ocrpt_mem_string_new(value, true);
		result->string_owned = true;
	} else
		result->isnull = true;

	return result;
}
