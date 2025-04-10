/*
 * Memory utilities
 *
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <inttypes.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <yajl/yajl_common.h>

#include "opencreport.h"

#ifndef HAVE_REALLOCARRAY
/* GNU specific naive reallocarray() implementation. */
static void *reallocarray(void *ptr, size_t nmemb, size_t size) {
	size_t oldsz = malloc_usable_size(ptr);
	int padding = size % sizeof(void *);
	size_t reqsz = nmemb * (size + padding);

	if (oldsz >= reqsz)
		return ptr;

	void *newptr = malloc(reqsz);
	if (!newptr)
		return NULL;

	memcpy(newptr, ptr, oldsz);
	free(ptr);

	return newptr;
}
#endif

DLL_EXPORT_SYM ocrpt_mem_malloc_t ocrpt_mem_malloc0 = malloc;
DLL_EXPORT_SYM ocrpt_mem_realloc_t ocrpt_mem_realloc0 = realloc;
DLL_EXPORT_SYM ocrpt_mem_reallocarray_t ocrpt_mem_reallocarray0 = reallocarray;
DLL_EXPORT_SYM ocrpt_mem_free_t ocrpt_mem_free0 = (ocrpt_mem_free_t)free;
DLL_EXPORT_SYM ocrpt_mem_free_size_t ocrpt_mem_free_size0 = (ocrpt_mem_free_size_t)free;
DLL_EXPORT_SYM ocrpt_mem_strdup_t ocrpt_mem_strdup0 = strdup;
DLL_EXPORT_SYM ocrpt_mem_strndup_t ocrpt_mem_strndup0 = strndup;

static void *ocrpt_yajl_malloc_func(void *ctx OCRPT_UNUSED_PARAM, size_t sz) {
	return ocrpt_mem_malloc0(sz);
}

static void *ocrpt_yajl_realloc_func(void *ctx OCRPT_UNUSED_PARAM, void *ptr, size_t sz) {
	return ocrpt_mem_realloc0(ptr, sz);
}

static void ocrpt_yajl_free_func(void *ctx OCRPT_UNUSED_PARAM, void *ptr) {
	ocrpt_mem_free0(ptr);
}

yajl_alloc_funcs ocrpt_yajl_alloc_funcs = {
	.malloc = ocrpt_yajl_malloc_func,
	.realloc = ocrpt_yajl_realloc_func,
	.free = ocrpt_yajl_free_func,
	.ctx = NULL
};

static void *ocrpt_mem_fake_reallocarray(void *ptr, size_t nmemb, size_t sz) {
	return ocrpt_mem_realloc0(ptr, nmemb * sz);
}

DLL_EXPORT_SYM void ocrpt_mem_set_alloc_funcs(
						ocrpt_mem_malloc_t rmalloc,
						ocrpt_mem_realloc_t rrealloc,
						ocrpt_mem_reallocarray_t rreallocarray,
						ocrpt_mem_free_t rfree,
						ocrpt_mem_strdup_t rstrdup,
						ocrpt_mem_strndup_t rstrndup) {
	ocrpt_mem_malloc0 = rmalloc;
	ocrpt_mem_realloc0 = rrealloc;
	ocrpt_mem_reallocarray0 = (rreallocarray ? rreallocarray : ocrpt_mem_fake_reallocarray);
	ocrpt_mem_free0 = rfree;
	ocrpt_mem_free_size0 = (ocrpt_mem_free_size_t)rfree;
	ocrpt_mem_strdup0 = rstrdup;
	ocrpt_mem_strndup0 = rstrndup;

	/*
	 * Don't do this for libXML2. It crashes PHP.
	xmlMemSetup((ocrpt_mem_free_nconst_t)rfree, rmalloc, rrealloc, rstrdup);
	 */

	/* Use the same allocator in GMP/MPFR */
#if MPFR_VERSION_MAJOR >= 4
	mpfr_mp_memory_cleanup();
#endif
	mp_set_memory_functions(ocrpt_mem_malloc0, ocrpt_mem_reallocarray0, ocrpt_mem_free_size0);
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
	size_t strl;

	if (!string)
		return NULL;

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

DLL_EXPORT_SYM ocrpt_string *ocrpt_mem_string_new_vnprintf(size_t len, const char *format, va_list va) {
	ocrpt_string *string;

	if (len <= 0)
		return NULL;

	string = ocrpt_mem_string_new_with_len(NULL, len + 1);
	if (!string)
		return NULL;

	vsnprintf(string->str, len + 1, format, va);
	string->len = len;
	return string;
}

DLL_EXPORT_SYM ocrpt_string *ocrpt_mem_string_new_printf(const char *format, ...) {
	va_list va;
	ocrpt_string *string;
	size_t len;

	va_start(va, format);
	len = vsnprintf(NULL, 0, format, va);
	va_end(va);

	if (len <= 0)
		return NULL;

	va_start(va, format);
	string = ocrpt_mem_string_new_vnprintf(len, format, va);
	va_end(va);

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

DLL_EXPORT_SYM void ocrpt_mem_string_append_len(ocrpt_string *string, const char *str, const size_t len) {
	if (!string)
		return;
	if (!str)
		return;
	if (!len)
		return;

	if (string->allocated_len < string->len + len + 1) {
		char *strnew = ocrpt_mem_realloc(string->str, string->len + len + 1);

		if (!strnew)
			return;

		string->allocated_len = string->len + len + 1;
		string->str = strnew;
	}

	char *end = stpncpy(&string->str[string->len], str, len);
	*end = 0;
	string->len = end - string->str;
}

DLL_EXPORT_SYM void ocrpt_mem_string_append_len_binary(ocrpt_string *string, const char *str, const size_t len) {
	if (!string)
		return;
	if (!str)
		return;
	if (!len)
		return;

	if (string->allocated_len < string->len + len + 1) {
		char *strnew = ocrpt_mem_realloc(string->str, string->len + len + 1);

		if (!strnew)
			return;

		string->allocated_len = string->len + len + 1;
		string->str = strnew;
	}

	memcpy(&string->str[string->len], str, len);
	string->len += len;
	string->str[string->len] = 0;
}

DLL_EXPORT_SYM void ocrpt_mem_string_append(ocrpt_string *string, const char *str) {
	size_t len;

	if (!string)
		return;
	if (!str)
		return;

	len = strlen(str);
	ocrpt_mem_string_append_len(string, str, len);
}

DLL_EXPORT_SYM void ocrpt_mem_string_append_c(ocrpt_string *string, const char c) {
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

DLL_EXPORT_SYM void ocrpt_mem_string_append_printf(ocrpt_string *string, const char *format, ...) {
	va_list va;
	ocrpt_string *newstr;
	size_t len;

	va_start(va, format);
	len = vsnprintf(NULL, 0, format, va);
	va_end(va);

	if (len <= 0)
		return;

	va_start(va, format);
	newstr = ocrpt_mem_string_new_vnprintf(len, format, va);
	va_end(va);

	ocrpt_mem_string_append_len(string, newstr->str, newstr->len);

	ocrpt_mem_string_free(newstr, true);
}
