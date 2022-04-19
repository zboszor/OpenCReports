/*
 * Datetime utilities
 * Copyright (C) 2022-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _DATETIME_H_
#define _DATETIME_H_

#include <stdbool.h>
#include <time.h>
#include "opencreport.h"

bool ocrpt_parse_datetime(opencreport *o, const char *time_string, int ts_len, ocrpt_result *result);
void ocrpt_datetime_add_number(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_number);
void ocrpt_datetime_sub_number(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_number);
void ocrpt_datetime_add_interval(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_interval);
void ocrpt_datetime_sub_interval(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_interval);

#endif
