/*
 * Memory utilities
 *
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opencreport.h"

DLL_EXPORT_SYM ocrpt_mem_malloc_t ocrpt_mem_malloc0 = malloc;
DLL_EXPORT_SYM ocrpt_mem_realloc_t ocrpt_mem_realloc0 = realloc;
DLL_EXPORT_SYM ocrpt_mem_free_t ocrpt_mem_free0 = (ocrpt_mem_free_t)free;
DLL_EXPORT_SYM ocrpt_mem_strdup_t ocrpt_mem_strdup0 = strdup;
DLL_EXPORT_SYM ocrpt_mem_strndup_t ocrpt_mem_strndup0 = strndup;

DLL_EXPORT_SYM void ocrpt_mem_set_alloc_funcs(ocrpt_mem_malloc_t rmalloc, ocrpt_mem_realloc_t rrealloc, ocrpt_mem_free_t rfree, ocrpt_mem_strdup_t rstrdup, ocrpt_mem_strndup_t rstrndup) {
	ocrpt_mem_malloc0 = rmalloc;
	ocrpt_mem_realloc0 = rrealloc;
	ocrpt_mem_free0 = rfree;
	ocrpt_mem_strdup0 = rstrdup;
	ocrpt_mem_strndup0 = rstrndup;
}

DLL_EXPORT_SYM ocrpt_string *ocrpt_mem_string_new(const char *str, bool copy) {
	ocrpt_string *string = ocrpt_mem_malloc(sizeof(ocrpt_string));

	if (!string)
		return NULL;

	if (!str) {
		string->str = NULL;
		string->allocated_len = 0;
		string->len = 0;
	} else {
		string->str = (copy ? ocrpt_mem_strdup(str) : (char *)str);

		if (!string->str) {
			ocrpt_mem_free(string);
			return NULL;
		}

		string->len = strlen(string->str);
		string->allocated_len = string->len + 1;
	}

	return string;
}

/* Allocate len + 1 bytes and zero-terminate the string */
DLL_EXPORT_SYM ocrpt_string *ocrpt_mem_string_new_with_len(const char *str, size_t len) {
	ocrpt_string *string = ocrpt_mem_malloc(sizeof(ocrpt_string));
	uint32_t strl;

	if (!string)
		return NULL;

	if (!str)
		len = 0;

	string->str = ocrpt_mem_malloc(len + 1);
	if (!string->str) {
		ocrpt_mem_free(string);
		return NULL;
	}

	if (str) {
		char *end = stpncpy(string->str, str, len);
		strl = end - string->str;
	} else
		strl = 0;

	string->str[strl] = 0;
	string->len = strl;
	string->allocated_len = len + 1;
	return string;
}

DLL_EXPORT_SYM ocrpt_string *ocrpt_mem_string_resize(ocrpt_string *string, size_t len) {
	char *str;

	if (!string)
		return ocrpt_mem_string_new_with_len(NULL, len);

	if (string->allocated_len > len)
		return string;

	str = ocrpt_mem_realloc(string->str, len + 1);
	if (str) {
		string->str = str;
		string->allocated_len = len + 1;
		return string;
	}

	return NULL;
}

DLL_EXPORT_SYM char *ocrpt_mem_string_free(ocrpt_string *string, bool free_str) {
	char *str;

	if (!string)
		return NULL;

	if (free_str) {
		ocrpt_mem_free(string->str);
		str = NULL;
	} else
		str = string->str;

	ocrpt_mem_free(string);

	return str;
}

void ocrpt_mem_string_append_len(ocrpt_string *string, const char *str, const size_t len) {
	char *end;

	if (!string)
		return;
	if (!str)
		return;

	if (string->allocated_len < string->len + len + 1) {
		char *strnew = ocrpt_mem_realloc(string->str, string->len + len + 1);

		if (!strnew)
			return;

		string->allocated_len = string->len + len + 1;
		string->str = strnew;
	}

	end = stpncpy(&string->str[string->len], str, len);
	*end = 0;
	string->len = end - string->str;
}

void ocrpt_mem_string_append(ocrpt_string *string, const char *str) {
	size_t len;

	if (!string)
		return;
	if (!str)
		return;

	len = strlen(str);
	ocrpt_mem_string_append_len(string, str, len);
}

void ocrpt_mem_string_append_c(ocrpt_string *string, const char c) {
	if (!string)
		return;

	if (string->allocated_len < string->len + 2) {
		char *strnew = ocrpt_mem_realloc(string->str, string->len + 2);

		if (!strnew)
			return;

		string->allocated_len = string->len + 2;
		string->str = strnew;
	}

	string->str[string->len++] = c;
	string->str[string->len] = 0;
}
