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

#endif
