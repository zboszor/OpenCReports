/*
 * Expression tree utilities
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "opencreport-private.h"
#include "datasource.h"

DLL_EXPORT_SYM ocrpt_environment_query_func ocrpt_environment_get = ocrpt_environment_get_c;

DLL_EXPORT_SYM void ocrpt_environment_set_query_func(ocrpt_environment_query_func func) {
	ocrpt_environment_get = func;
}

DLL_EXPORT_SYM ocrpt_result *ocrpt_environment_get_c(const char *env) {
	ocrpt_result *result = ocrpt_mem_malloc(sizeof(ocrpt_result));
	char *value;

	if (!result)
		return NULL;

	value = getenv(env);
	memset(result, 0, sizeof(ocrpt_result));
	result->type = OCRPT_RESULT_STRING;
	if (value) {
		result->string = ocrpt_mem_string_new(value, true);
		result->string_owned = true;
	} else
		result->isnull = true;

	return result;
}
