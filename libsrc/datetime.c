/*
 * OpenCReports main module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <langinfo.h>

#include "opencreport.h"
#include "datetime.h"

bool ocrpt_parse_datetime(opencreport *o, const char *time_string, int ts_len, ocrpt_result *result) {
	char *final_time_string, *fts_last;
	char final_fmt[64] = "", *ffmt_last = final_fmt;
	char *rest = NULL, *ret = NULL;
	char *tfmt = nl_langinfo_l(T_FMT, o->locale);
	char *dfmt = nl_langinfo_l(D_FMT, o->locale);
	struct tm tm;
	char *datefmts[] = { "%d/%m/%y", "%d/%m/%Y", "%F", dfmt, NULL };
	char *timefmts[] = { "%T", "%R", tfmt, NULL };
	char *fmt;
	int i;
	bool parsed_date = false;
	bool parsed_time = false;
	bool parsed_zone = false;

	final_time_string = malloc(ts_len + 10);
	memset(final_time_string, 0, ts_len + 10);
	fts_last = final_time_string;

	/* Parse date part */
	for (i = 0, fmt = datefmts[i]; fmt; i++, fmt = datefmts[i]) {
		memset(&tm, 0, sizeof(struct tm));
		tm.tm_isdst = -1;
		ret = strptime(time_string, fmt, &tm);
		if (ret) {
			/* Short read on %y, try again with %Y */
			if (i == 0 && *ret >= '0' && *ret <= '9')
				continue;
			break;
		}
	}

	if (ret && fmt) {
		fts_last = stpncpy(final_time_string, time_string, ret - time_string);
		*fts_last = 0;
		ffmt_last = stpcpy(final_fmt, fmt);
		*ffmt_last = 0;
		parsed_date = true;
	}

	if (!ret)
		ret = (char *)time_string;

	if (*ret == 'T') {
		*fts_last++ = 'T';
		*ffmt_last++ = 'T';
		ret++;
	}
	while (*ret == ' ') {
		*fts_last++ = ' ';
		*ffmt_last++ = ' ';
		ret++;
	}
	*fts_last = 0;
	*ffmt_last = 0;
	if (*ret == 0)
		goto end;

	rest = ret;

	/* Parse time part */
	for (i = 0, fmt = timefmts[i]; fmt; i++, fmt = timefmts[i]) {
		memset(&tm, 0, sizeof(struct tm));
		tm.tm_isdst = -1;
		ret = strptime(rest, fmt, &tm);
		if (ret)
			break;
	}

	if (ret && fmt) {
		fts_last = stpncpy(fts_last, rest, ret - rest);
		*fts_last = 0;
		ffmt_last = stpcpy(ffmt_last, fmt);
		*ffmt_last = 0;

		parsed_time = true;

		/* Ignore fractional seconds */
		if (*ret == '.') {
			ret++;
			while (*ret >= '0' && *ret <= '9')
				ret++;
		}

		while (*ret == ' ') {
			*fts_last++ = ' ';
			*ffmt_last++ = ' ';
			ret++;
		}
		*fts_last = 0;
		*ffmt_last = 0;
		if (*ret == 0)
			goto end;

		rest = ret;

		/* Parse timezone specification */
		if (*rest == '+' || *rest == '-') {
			fmt = "%z";

			memset(&tm, 0, sizeof(struct tm));
			tm.tm_isdst = -1;
			tm.tm_gmtoff = timezone;
			ret = strptime(rest, fmt, &tm);

			if (!ret)
				goto end;

			fts_last = stpncpy(fts_last, rest, ret - rest);
			*fts_last = 0;
			ffmt_last = stpcpy(ffmt_last, fmt);
			*ffmt_last = 0;

			parsed_zone = true;
		}
	}

	end:

	if (parsed_date || parsed_time) {
		memset(&tm, 0, sizeof(struct tm));
		tm.tm_isdst = -1;
		tm.tm_gmtoff = timezone;
		ret = strptime(final_time_string, final_fmt, &tm);

		if (parsed_zone) {
			time_t gmtoff = tm.tm_gmtoff;
			tm.tm_isdst = -1;
			time_t ts = mktime(&tm);
			ts -= gmtoff - tm.tm_gmtoff;
			localtime_r(&ts, &result->datetime);
		} else
			result->datetime = tm;

		result->date_valid = parsed_date;
		result->time_valid = parsed_time;
		result->interval = false;
		result->day_carry = 0;
		result->isnull = false;
	}

	free(final_time_string);

	return parsed_date || parsed_time;
}
