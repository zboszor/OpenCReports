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
#include <mpfr.h>

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
	char *timefmts[] = { "%Tp", "%T", "%Rp" ,"%R", tfmt, NULL };
	char *fmt;
	int i;
	bool parsed_date = false;
	bool parsed_time = false;
	bool parsed_zone = false;
	bool allnums = true;
	bool time_pm = false;

	final_time_string = malloc(ts_len + 10);
	memset(final_time_string, 0, ts_len + 10);
	fts_last = final_time_string;

	for (i = 0; i < ts_len; i++) {
		if (time_string[i] < '0' || time_string[i] > '9') {
			if (i == ts_len - 1 && time_string[i] == 'p')
				time_pm = true;
			else
				allnums = false;
			break;
		}
	}

	if (allnums) {
		switch (ts_len) {
		case 14:
			ret = strptime(time_string, "%Y%m%d%H%M%S", &tm);
			if (ret && (ret - time_string) == 14) {
				fts_last = stpncpy(final_time_string, time_string, 14);
				*fts_last = 0;
				strcpy(final_fmt, "%Y%m%d%H%M%S");
				parsed_date = true;
				parsed_time = true;
				goto end;
			}
			goto end_error;
		case 7:
			if (!time_pm)
				goto end_error;
		case 6:
			ret = strptime(time_string, "%H%M%S", &tm);
			if (ret && (ret - time_string) == 6) {
				if (!time_pm || (time_pm && tm.tm_hour < 12)) {
					fts_last = stpncpy(final_time_string, time_string, 6);
					*fts_last = 0;
					strcpy(final_fmt, "%H%M%S");
					parsed_time = true;
					goto end;
				}
			}
			goto end_error;
		case 5:
			if (!time_pm)
				goto end_error;
		case 4:
			ret = strptime(time_string, "%H%M", &tm);
			if (ret && (ret - time_string) == 4) {
				if (!time_pm || (time_pm && tm.tm_hour < 12)) {
					fts_last = stpncpy(final_time_string, time_string, 4);
					*fts_last = 0;
					strcpy(final_fmt, "%H%M");
					parsed_time = true;
					goto end;
				}
			}
			goto end_error;
		default:
			goto end_error;
		}
	}

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
		if (ret) {
			if (i == 0 || i == 2) {
				if (tm.tm_hour < 12) {
					time_pm = true;
					break;
				} else
					goto end_error;
			}
			break;
		}
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
		} else {
			if (time_pm)
				tm.tm_hour += 12;
			result->datetime = tm;
		}

		result->date_valid = parsed_date;
		result->time_valid = parsed_time;
		result->interval = false;
		result->day_carry = 0;
		result->isnull = false;
	}

	end_error:
	free(final_time_string);

	return parsed_date || parsed_time;
}

static inline void fix_time_wrap(struct tm *tm) {
	while (tm->tm_sec >= 60) {
		tm->tm_sec -= 60;
		tm->tm_min++;
		if (tm->tm_min >= 60) {
			tm->tm_min -= 60;
			tm->tm_hour++;
			if (tm->tm_hour >= 24) {
				tm->tm_hour -= 24;
				tm->tm_mday++;
			}
		}
	}
	while (tm->tm_sec < 0) {
		tm->tm_sec += 60;
		tm->tm_min--;
		if (tm->tm_min < 0) {
			tm->tm_min += 60;
			tm->tm_hour--;
			if (tm->tm_hour < 0) {
				tm->tm_hour += 24;
				tm->tm_mday--;
			}
		}
	}
}

const int days_in_month[2][12] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static inline void fix_day_wrap(struct tm *tm, bool interval) {
	if (!interval) {
		bool ly;
		int dim;

		for (ly = ocrpt_leap_year(tm->tm_year + 1900), dim = days_in_month[ly][tm->tm_mon]; tm->tm_mday > dim; ly = ocrpt_leap_year(tm->tm_year + 1900), dim = days_in_month[ly][tm->tm_mon]) {
			tm->tm_mday -= dim;
			tm->tm_mon++;
			if (tm->tm_mon >= 12) {
				tm->tm_mon -= 12;
				tm->tm_year++;
			}
		}

		while (tm->tm_mday < 1) {
			tm->tm_mon--;
			if (tm->tm_mon < 0) {
				tm->tm_mon += 12;
				tm->tm_year--;
			}
			tm->tm_mday += days_in_month[ocrpt_leap_year(tm->tm_year + 1900)][tm->tm_mon];
		}
	}

	while (tm->tm_mon >= 12) {
		tm->tm_mon -= 12;
		tm->tm_year++;
	}
	while (tm->tm_mon < 0) {
		tm->tm_mon += 12;
		tm->tm_year--;
	}
}

bool ocrpt_datetime_result_add_number(opencreport *o, ocrpt_result *dst, ocrpt_result *src_datetime, long number) {
	dst->datetime = src_datetime->datetime;
	dst->date_valid = src_datetime->date_valid;
	dst->time_valid = src_datetime->time_valid;
	dst->interval = src_datetime->interval;
	dst->day_carry = src_datetime->day_carry;

	if (src_datetime->interval) {
		dst->datetime.tm_sec += number;
		fix_time_wrap(&dst->datetime);
		fix_day_wrap(&dst->datetime, dst->interval);
		return true;
	} else if (src_datetime->date_valid && src_datetime->time_valid) {
		time_t t = mktime(&src_datetime->datetime) + number;
		localtime_r(&t, &dst->datetime);
		return true;
	} else if (src_datetime->date_valid) {
		dst->datetime.tm_mday += number;
		fix_day_wrap(&dst->datetime, dst->interval);
		return true;
	} else if (src_datetime->time_valid) {
		dst->datetime.tm_sec += number;
		fix_time_wrap(&dst->datetime);
		dst->datetime.tm_year = 0;
		dst->datetime.tm_mon = 0;
		dst->datetime.tm_mday = 0;
		return true;
	}

	return false;
}

void ocrpt_datetime_add_number(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_number) {
	if (!ocrpt_datetime_result_add_number(o, dst->result[o->residx], src_datetime, mpfr_get_si(src_number->number, o->rndmode)))
		ocrpt_expr_make_error_result(o, dst, "invalid operand(s)");
}

void ocrpt_datetime_sub_number(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_number) {
	if (!ocrpt_datetime_result_add_number(o, dst->result[o->residx], src_datetime, -mpfr_get_si(src_number->number, o->rndmode)))
		ocrpt_expr_make_error_result(o, dst, "invalid operand(s)");
}

void ocrpt_datetime_add_interval(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_interval) {
}

void ocrpt_datetime_sub_interval(opencreport *o, ocrpt_expr *dst, ocrpt_result *src_datetime, ocrpt_result *src_interval) {
}
