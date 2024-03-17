/*
 * Datetime utilities
 * Copyright (C) 2023-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _DATETIME_H_
#define _DATETIME_H_

#include <stdbool.h>
#include <time.h>
#include "opencreport.h"

extern const int days_in_month[2][12];

static inline bool ocrpt_leap_year(int y) {
	return ((y % 100) == 0) ? ((y % 400) == 0) : ((y % 4) == 0);
}

bool ocrpt_parse_datetime(opencreport *o, const char *time_string, int ts_len, ocrpt_result *result) __attribute__((nonnull(1,2)));
bool ocrpt_parse_interval(opencreport *o, const char *time_string, int ts_len, ocrpt_result *result) __attribute__((nonnull(1,2)));
bool ocrpt_datetime_result_add_number(opencreport *o, ocrpt_result *dst, ocrpt_result *src_datetime, long number);
void ocrpt_datetime_add_number(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_number);
void ocrpt_datetime_sub_number(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_number);
void ocrpt_datetime_add_interval(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_interval);
void ocrpt_datetime_sub_datetime(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime1, ocrpt_result *src_datetime2);

#endif
