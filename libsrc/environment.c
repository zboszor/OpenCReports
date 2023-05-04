/*
 * Expression tree utilities
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
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

DLL_EXPORT_SYM void ocrpt_add_mvariable(opencreport *o, const char *name, const char *value) {
	if (!o)
		return;

	ocrpt_mvarentry *e = NULL;

	for (ocrpt_list *l = o->mvarlist; l; l = l->next) {
		ocrpt_mvarentry *m = (ocrpt_mvarentry *)l->data;
		if (strcmp(name, m->name) == 0) {
			e = m;
			break;
		}
	}

	if (e) {
		if (value) {
			ocrpt_mem_free(e->value);
			e->value = ocrpt_mem_strdup(value);
		} else
			o->mvarlist = ocrpt_list_end_remove(o->mvarlist, &o->mvarlist_end, e);
	} else {
		e = ocrpt_mem_malloc(sizeof(ocrpt_mvarentry));
		e->name = ocrpt_mem_strdup(name);
		e->value = ocrpt_mem_strdup(value);
		o->mvarlist = ocrpt_list_end_append(o->mvarlist, &o->mvarlist_end, e);
	}
}

ocrpt_result *ocrpt_find_mvariable(opencreport *o, const char *name) {
	if (!o)
		return NULL;

	ocrpt_mvarentry *e = NULL;

	for (ocrpt_list *l = o->mvarlist; l; l = l->next) {
		ocrpt_mvarentry *m = (ocrpt_mvarentry *)l->data;
		if (strcmp(name, m->name) == 0) {
			e = m;
			break;
		}
	}

	if (!e)
		return NULL;

	ocrpt_result *result = ocrpt_result_new(o);
	if (!result)
		return NULL;

	result->type = OCRPT_RESULT_STRING;
	result->string = ocrpt_mem_string_new(e->value, true);
	result->string_owned = true;

	return result;
}
