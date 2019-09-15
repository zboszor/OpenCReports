/*
 * Memory utilities
 *
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _MEMUTIL_H_
#define _MEMUTIL_H_

#include <stddef.h> /* for size_t */
#include "opencreport.h"

struct ocrpt_string {
	char *str;
	size_t allocated_len;
	size_t len;
};
typedef struct ocrpt_string ocrpt_string;

ocrpt_string *ocrpt_mem_string_new(const char *str);

ocrpt_string *ocrpt_mem_string_new_with_len(const char *str, const size_t len);

char *ocrpt_mem_string_free(ocrpt_string *string, int free_str);

void ocrpt_mem_string_append_len(ocrpt_string *string, const char *str, const size_t len);

void ocrpt_mem_string_append(ocrpt_string *string, const char *str);

void ocrpt_mem_string_append_c(ocrpt_string *string, const char c);

#endif
