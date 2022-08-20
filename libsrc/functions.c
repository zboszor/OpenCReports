/*
 * OpenCReports main module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <langinfo.h>
#include <utf8proc.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "datasource.h"
#include "exprutil.h"
#include "breaks.h"
#include "datetime.h"
#include "formatting.h"
#include "parts.h"
#include "functions.h"

static bool ocrpt_expr_init_result_internal(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type, unsigned int which) {
	ocrpt_result *result = e->result[which];

	if (!result) {
		result = ocrpt_mem_malloc(sizeof(ocrpt_result));
		if (result) {
			memset(result, 0, sizeof(ocrpt_result));
			e->result[which] = result;
			ocrpt_expr_set_result_owned(e, which, true);
		}
	}
	if (result) {
		switch (type) {
		case OCRPT_RESULT_STRING:
			ocrpt_string *string = ocrpt_mem_string_resize(result->string, 16);
			if (string) {
				if (!result->string) {
					result->string = string;
					result->string_owned = true;
				}
				string->len = 0;
			}
			break;
		case OCRPT_RESULT_NUMBER:
			if (!result->number_initialized) {
				mpfr_init2(result->number, o->prec);
				result->number_initialized = true;
			}
			break;
		case OCRPT_RESULT_DATETIME:
			memset(&result->datetime, 0, sizeof(result->datetime));
			result->date_valid = false;
			result->time_valid = false;
			result->interval = true;
			result->day_carry = 0;
			break;
		default:
			break;
		}

		result->type = type;
		result->isnull = false;
	}

	return !!result;
}

DLL_EXPORT_SYM bool ocrpt_expr_init_result(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type) {
	return ocrpt_expr_init_result_internal(o, e, type, o->residx);
}

DLL_EXPORT_SYM void ocrpt_expr_init_results(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type) {
	for (uint32_t i = 0; i < OCRPT_EXPR_RESULTS; i++)
		ocrpt_expr_init_result_internal(o, e, type, i);
}

OCRPT_STATIC_FUNCTION(ocrpt_abs) {
	if (e->n_ops == 1 && e->ops[0]->result[o->residx] && e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		mpfr_abs(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_uminus) {
	if (e->n_ops == 1 && e->ops[0]->result[o->residx] && e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		e->result[o->residx]->type = OCRPT_RESULT_NUMBER;
		mpfr_neg(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_add) {
	uint32_t nnum, nstr, ndt;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	nstr = 0;
	ndt = 0;
	for (uint32_t i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]) {
			if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
				nnum++;
			if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_STRING)
				nstr++;
			if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_DATETIME)
				ndt++;
		}
	}

	if (nnum == e->n_ops) {
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		for (uint32_t i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (uint32_t i = 1; i < e->n_ops; i++)
			mpfr_add(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else if (nstr == e->n_ops) {
		ocrpt_string *string;
		int32_t len;
		uint32_t i;

		ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

		for (len = 0, i = 0; i < e->n_ops; i++)
			len += e->ops[i]->result[o->residx]->string->len;

		string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
		if (string) {
			if (!e->result[o->residx]->string) {
				e->result[o->residx]->string = string;
				e->result[o->residx]->string_owned = true;
			}
			string->len = 0;
			for (i = 0; i < e->n_ops; i++) {
				ocrpt_string *sstring = e->ops[i]->result[o->residx]->string;
				ocrpt_mem_string_append_len(string, sstring->str, sstring->len);
			}
		} else
			ocrpt_expr_make_error_result(o, e, "out of memory");
	} else if (ndt > 0 && (nnum + ndt) == e->n_ops) {
		/*
		 * Rules for adding datetimes/intervals and numbers are a mix
		 * of RLIB and PostgreSQL rules.
		 *
		 * - strictly do from left to right, no rearrangements.
		 * - numbers are treated as integers, fractionals are truncated.
		 *
		 * - number + number -> number (duh)
		 *
		 * - number + datetime w/ valid time -> datetime + seconds
		 * - datetime w/ valid time + number -> datetime + seconds
		 *
		 * - number + interval -> interval + seconds
		 * - interval + number -> interval + seconds
		 *
		 * - number + datetime w/o valid time -> datetime + days
		 * - datetime w/o valid time + number -> datetime + days
		 *
		 * - datetime + interval -> datetime w/ valid time
		 * - interval + datetime -> datetime w/ valid time
		 *
		 * - interval + interval -> interval
		 *
		 * Invalid operand combinations:
		 * - datetime + datetime
		 */
		ocrpt_expr_init_result(o, e, e->ops[0]->result[o->residx]->type);
		ocrpt_result_copy(o, e->result[o->residx], e->ops[0]->result[o->residx]);

		for (uint32_t i = 1; i < e->n_ops; i++) {
			switch (e->result[o->residx]->type) {
			case OCRPT_RESULT_NUMBER:
				switch (e->ops[i]->result[o->residx]->type) {
				case OCRPT_RESULT_NUMBER:
					mpfr_add(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
					break;
				case OCRPT_RESULT_DATETIME:
					if (e->ops[i]->result[o->residx]->interval) {
						ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
						return;
					}
					e->result[o->residx]->type = OCRPT_RESULT_DATETIME;
					ocrpt_datetime_add_number(o, e, e->ops[i]->result[o->residx], e->result[o->residx]);
					break;
				default:
					/* cannot happen */
					ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
					return;
				}
				break;
			case OCRPT_RESULT_DATETIME:
				switch (e->ops[i]->result[o->residx]->type) {
				case OCRPT_RESULT_NUMBER:
					if (e->result[o->residx]->interval) {
						ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
						return;
					}
					ocrpt_datetime_add_number(o, e, e->result[o->residx], e->ops[i]->result[o->residx]);
					break;
				case OCRPT_RESULT_DATETIME:
					if (!e->result[o->residx]->interval && !e->ops[i]->result[o->residx]->interval) {
						ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
						return;
					}
					if (e->result[o->residx]->interval && !e->ops[i]->result[o->residx]->interval) {
						ocrpt_result interval = { .type = OCRPT_RESULT_DATETIME };
						ocrpt_result_copy(o, &interval, e->result[o->residx]);
						ocrpt_result_copy(o, e->result[o->residx], e->ops[i]->result[o->residx]);
						ocrpt_datetime_add_interval(o, e, e->result[o->residx], &interval);
					} else
						ocrpt_datetime_add_interval(o, e, e->result[o->residx], e->ops[i]->result[o->residx]);
					break;
				default:
					/* cannot happpen */
					ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
					return;
				}
				break;
			default:
				/* cannot happen */
				ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
				return;
			}
		}
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_sub) {
	uint32_t i, nnum = 0, ndt = 0;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	ndt = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]) {
			if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
				nnum++;
			if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_DATETIME) {
				ndt++;
			}
		}
	}

	if (nnum == e->n_ops) {
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_sub(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else if (ndt > 0 && (nnum + ndt) == e->n_ops) {
		/*
		 * Rules for subtracting datetimes/intervals and numbers are a mix
		 * of RLIB and PostgreSQL rules.
		 *
		 * - strictly do from left to right, no rearrangements
		 * - numbers are treated as integers, fractionals are truncated.
		 *
		 * - number - number -> number (duh)
		 *
		 * - datetime w/ valid time - number -> datetime - seconds
		 * - interval - number -> interval - seconds
		 *
		 * - datetime w/o valid time - number -> datetime - days
		 *
		 * - datetime - datetime -> interval
		 *
		 * - datetime - interval -> datetime w/ valid time
		 *
		 * - interval - interval -> interval
		 *
		 * Invalid operand combinations:
		 * - number - datetime
		 * - number - interval
		 * - interval - datetime
		 */
		ocrpt_expr_init_result(o, e, e->ops[0]->result[o->residx]->type);
		ocrpt_result_copy(o, e->result[o->residx], e->ops[0]->result[o->residx]);

		for (uint32_t i = 1; i < e->n_ops; i++) {
			switch (e->result[o->residx]->type) {
			case OCRPT_RESULT_NUMBER:
				switch (e->ops[i]->result[o->residx]->type) {
				case OCRPT_RESULT_NUMBER:
					mpfr_sub(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
					break;
				case OCRPT_RESULT_DATETIME:
					ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
					return;
				default:
					/* cannot happen */
					ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
					return;
				}
				break;
			case OCRPT_RESULT_DATETIME:
				switch (e->ops[i]->result[o->residx]->type) {
				case OCRPT_RESULT_NUMBER:
					if (e->result[o->residx]->interval) {
						ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
						return;
					}
					ocrpt_datetime_sub_number(o, e, e->result[o->residx], e->ops[i]->result[o->residx]);
					break;
				case OCRPT_RESULT_DATETIME:
					if (e->result[o->residx]->interval && !e->ops[i]->result[o->residx]->interval) {
						ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
						return;
					}
					ocrpt_datetime_sub_datetime(o, e, e->result[o->residx], e->ops[i]->result[o->residx]);
					break;
				default:
					/* cannot happpen */
					ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
					return;
				}
				break;
			default:
				/* cannot happen */
				ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
				return;
			}
		}
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_mul) {
	uint32_t i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx] && e->ops[i]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_mul(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_div) {
	uint32_t i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx] && e->ops[i]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->isnull) {
				e->result[o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_div(e->result[o->residx]->number, e->result[o->residx]->number, e->ops[i]->result[o->residx]->number, o->rndmode);
	} else
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_eq) {
	unsigned long ret;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (uint32_t i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) == 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) == 0);
		break;
	case OCRPT_RESULT_DATETIME:
		if ((e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid &&
				e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) ||
			(e->ops[0]->result[o->residx]->interval && e->ops[1]->result[o->residx]->interval)) {
			ret =
				(e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) &&
				(e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) &&
				(e->ops[0]->result[o->residx]->datetime.tm_mday == e->ops[1]->result[o->residx]->datetime.tm_mday) &&
				(e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) &&
				(e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) &&
				(e->ops[0]->result[o->residx]->datetime.tm_sec == e->ops[1]->result[o->residx]->datetime.tm_sec);
		} else if (e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid) {
			ret =
				(e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) &&
				(e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) &&
				(e->ops[0]->result[o->residx]->datetime.tm_mday == e->ops[1]->result[o->residx]->datetime.tm_mday);
		} else if (e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) {
			ret =
				(e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) &&
				(e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) &&
				(e->ops[0]->result[o->residx]->datetime.tm_sec == e->ops[1]->result[o->residx]->datetime.tm_sec);
		} else
			ret = false;
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_ne) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) != 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) != 0);
		break;
	case OCRPT_RESULT_DATETIME:
		if ((e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid &&
				e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) ||
			(e->ops[0]->result[o->residx]->interval && e->ops[1]->result[o->residx]->interval)) {
			ret =
				(e->ops[0]->result[o->residx]->datetime.tm_year != e->ops[1]->result[o->residx]->datetime.tm_year) ||
				(e->ops[0]->result[o->residx]->datetime.tm_mon != e->ops[1]->result[o->residx]->datetime.tm_mon) ||
				(e->ops[0]->result[o->residx]->datetime.tm_mday != e->ops[1]->result[o->residx]->datetime.tm_mday) ||
				(e->ops[0]->result[o->residx]->datetime.tm_hour != e->ops[1]->result[o->residx]->datetime.tm_hour) ||
				(e->ops[0]->result[o->residx]->datetime.tm_min != e->ops[1]->result[o->residx]->datetime.tm_min) ||
				(e->ops[0]->result[o->residx]->datetime.tm_sec != e->ops[1]->result[o->residx]->datetime.tm_sec);
		} else if (e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid) {
			ret =
				(e->ops[0]->result[o->residx]->datetime.tm_year != e->ops[1]->result[o->residx]->datetime.tm_year) ||
				(e->ops[0]->result[o->residx]->datetime.tm_mon != e->ops[1]->result[o->residx]->datetime.tm_mon) ||
				(e->ops[0]->result[o->residx]->datetime.tm_mday != e->ops[1]->result[o->residx]->datetime.tm_mday);
		} else if (e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) {
			ret =
				(e->ops[0]->result[o->residx]->datetime.tm_hour != e->ops[1]->result[o->residx]->datetime.tm_hour) ||
				(e->ops[0]->result[o->residx]->datetime.tm_min != e->ops[1]->result[o->residx]->datetime.tm_min) ||
				(e->ops[0]->result[o->residx]->datetime.tm_sec != e->ops[1]->result[o->residx]->datetime.tm_sec);
		} else
			ret = false;
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_lt) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) < 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) < 0);
		break;
	case OCRPT_RESULT_DATETIME:
		ret = false;
		if ((e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid &&
				e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) ||
			(e->ops[0]->result[o->residx]->interval && e->ops[1]->result[o->residx]->interval)) {
			if (e->ops[0]->result[o->residx]->datetime.tm_year < e->ops[1]->result[o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[o->residx]->datetime.tm_mon < e->ops[1]->result[o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[o->residx]->datetime.tm_mday < e->ops[1]->result[o->residx]->datetime.tm_mday)
						ret = true;
					else if (e->ops[0]->result[o->residx]->datetime.tm_mday == e->ops[1]->result[o->residx]->datetime.tm_mday) {
						if (e->ops[0]->result[o->residx]->datetime.tm_hour < e->ops[1]->result[o->residx]->datetime.tm_hour)
							ret = true;
						else if (e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) {
							if (e->ops[0]->result[o->residx]->datetime.tm_min < e->ops[1]->result[o->residx]->datetime.tm_min)
								ret = true;
							else if (e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) {
								if (e->ops[0]->result[o->residx]->datetime.tm_sec < e->ops[1]->result[o->residx]->datetime.tm_sec)
									ret = true;
							}
						}
					}
				}
			}
		} else if (e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid) {
			if (e->ops[0]->result[o->residx]->datetime.tm_year < e->ops[1]->result[o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[o->residx]->datetime.tm_mon < e->ops[1]->result[o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[o->residx]->datetime.tm_mday < e->ops[1]->result[o->residx]->datetime.tm_mday)
						ret = true;
				}
			}
		} else if (e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) {
			if (e->ops[0]->result[o->residx]->datetime.tm_hour < e->ops[1]->result[o->residx]->datetime.tm_hour)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) {
				if (e->ops[0]->result[o->residx]->datetime.tm_min < e->ops[1]->result[o->residx]->datetime.tm_min)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) {
					if (e->ops[0]->result[o->residx]->datetime.tm_sec < e->ops[1]->result[o->residx]->datetime.tm_sec)
						ret = true;
				}
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_le) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) <= 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) <= 0);
		break;
	case OCRPT_RESULT_DATETIME:
		ret = false;
		if ((e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid &&
				e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) ||
			(e->ops[0]->result[o->residx]->interval && e->ops[1]->result[o->residx]->interval)) {
			if (e->ops[0]->result[o->residx]->datetime.tm_year < e->ops[1]->result[o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[o->residx]->datetime.tm_mon < e->ops[1]->result[o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[o->residx]->datetime.tm_mday < e->ops[1]->result[o->residx]->datetime.tm_mday)
						ret = true;
					else if (e->ops[0]->result[o->residx]->datetime.tm_mday == e->ops[1]->result[o->residx]->datetime.tm_mday) {
						if (e->ops[0]->result[o->residx]->datetime.tm_hour < e->ops[1]->result[o->residx]->datetime.tm_hour)
							ret = true;
						else if (e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) {
							if (e->ops[0]->result[o->residx]->datetime.tm_min < e->ops[1]->result[o->residx]->datetime.tm_min)
								ret = true;
							else if (e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) {
								if (e->ops[0]->result[o->residx]->datetime.tm_sec <= e->ops[1]->result[o->residx]->datetime.tm_sec)
									ret = true;
							}
						}
					}
				}
			}
		} else if (e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid) {
			if (e->ops[0]->result[o->residx]->datetime.tm_year < e->ops[1]->result[o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[o->residx]->datetime.tm_mon < e->ops[1]->result[o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[o->residx]->datetime.tm_mday <= e->ops[1]->result[o->residx]->datetime.tm_mday)
						ret = true;
				}
			}
		} else if (e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) {
			if (e->ops[0]->result[o->residx]->datetime.tm_hour < e->ops[1]->result[o->residx]->datetime.tm_hour)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) {
				if (e->ops[0]->result[o->residx]->datetime.tm_min < e->ops[1]->result[o->residx]->datetime.tm_min)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) {
					if (e->ops[0]->result[o->residx]->datetime.tm_sec <= e->ops[1]->result[o->residx]->datetime.tm_sec)
						ret = true;
				}
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_gt) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) > 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) > 0);
		break;
	case OCRPT_RESULT_DATETIME:
		ret = false;
		if ((e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid &&
				e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) ||
			(e->ops[0]->result[o->residx]->interval && e->ops[1]->result[o->residx]->interval)) {
			if (e->ops[0]->result[o->residx]->datetime.tm_year > e->ops[1]->result[o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[o->residx]->datetime.tm_mon > e->ops[1]->result[o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[o->residx]->datetime.tm_mday > e->ops[1]->result[o->residx]->datetime.tm_mday)
						ret = true;
					else if (e->ops[0]->result[o->residx]->datetime.tm_mday == e->ops[1]->result[o->residx]->datetime.tm_mday) {
						if (e->ops[0]->result[o->residx]->datetime.tm_hour > e->ops[1]->result[o->residx]->datetime.tm_hour)
							ret = true;
						else if (e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) {
							if (e->ops[0]->result[o->residx]->datetime.tm_min > e->ops[1]->result[o->residx]->datetime.tm_min)
								ret = true;
							else if (e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) {
								if (e->ops[0]->result[o->residx]->datetime.tm_sec > e->ops[1]->result[o->residx]->datetime.tm_sec)
									ret = true;
							}
						}
					}
				}
			}
		} else if (e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid) {
			if (e->ops[0]->result[o->residx]->datetime.tm_year > e->ops[1]->result[o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[o->residx]->datetime.tm_mon > e->ops[1]->result[o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[o->residx]->datetime.tm_mday > e->ops[1]->result[o->residx]->datetime.tm_mday)
						ret = true;
				}
			}
		} else if (e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) {
			if (e->ops[0]->result[o->residx]->datetime.tm_hour > e->ops[1]->result[o->residx]->datetime.tm_hour)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) {
				if (e->ops[0]->result[o->residx]->datetime.tm_min > e->ops[1]->result[o->residx]->datetime.tm_min)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) {
					if (e->ops[0]->result[o->residx]->datetime.tm_sec > e->ops[1]->result[o->residx]->datetime.tm_sec)
						ret = true;
				}
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_ge) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[o->residx] || !e->ops[1]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != e->ops[1]->result[o->residx]->type) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		else if (e->ops[1]->result[o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(o, e, e->ops[1]->result[o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number) >= 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[o->residx]->string->str, e->ops[1]->result[o->residx]->string->str) >= 0);
		break;
	case OCRPT_RESULT_DATETIME:
		ret = false;
		if ((e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid &&
				e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) ||
			(e->ops[0]->result[o->residx]->interval && e->ops[1]->result[o->residx]->interval)) {
			if (e->ops[0]->result[o->residx]->datetime.tm_year > e->ops[1]->result[o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[o->residx]->datetime.tm_mon > e->ops[1]->result[o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[o->residx]->datetime.tm_mday > e->ops[1]->result[o->residx]->datetime.tm_mday)
						ret = true;
					else if (e->ops[0]->result[o->residx]->datetime.tm_mday == e->ops[1]->result[o->residx]->datetime.tm_mday) {
						if (e->ops[0]->result[o->residx]->datetime.tm_hour > e->ops[1]->result[o->residx]->datetime.tm_hour)
							ret = true;
						else if (e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) {
							if (e->ops[0]->result[o->residx]->datetime.tm_min > e->ops[1]->result[o->residx]->datetime.tm_min)
								ret = true;
							else if (e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) {
								if (e->ops[0]->result[o->residx]->datetime.tm_sec >= e->ops[1]->result[o->residx]->datetime.tm_sec)
									ret = true;
							}
						}
					}
				}
			}
		} else if (e->ops[0]->result[o->residx]->date_valid && e->ops[1]->result[o->residx]->date_valid) {
			if (e->ops[0]->result[o->residx]->datetime.tm_year > e->ops[1]->result[o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_year == e->ops[1]->result[o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[o->residx]->datetime.tm_mon > e->ops[1]->result[o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_mon == e->ops[1]->result[o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[o->residx]->datetime.tm_mday >= e->ops[1]->result[o->residx]->datetime.tm_mday)
						ret = true;
				}
			}
		} else if (e->ops[0]->result[o->residx]->time_valid && e->ops[1]->result[o->residx]->time_valid) {
			if (e->ops[0]->result[o->residx]->datetime.tm_hour > e->ops[1]->result[o->residx]->datetime.tm_hour)
				ret = true;
			else if (e->ops[0]->result[o->residx]->datetime.tm_hour == e->ops[1]->result[o->residx]->datetime.tm_hour) {
				if (e->ops[0]->result[o->residx]->datetime.tm_min > e->ops[1]->result[o->residx]->datetime.tm_min)
					ret = true;
				else if (e->ops[0]->result[o->residx]->datetime.tm_min == e->ops[1]->result[o->residx]->datetime.tm_min) {
					if (e->ops[0]->result[o->residx]->datetime.tm_sec >= e->ops[1]->result[o->residx]->datetime.tm_sec)
						ret = true;
				}
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_val) {
	char *str;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx] || e->ops[0]->result[o->residx]->type == OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
		break;
	case OCRPT_RESULT_STRING:
		str = e->ops[0]->result[o->residx]->string->str;
		if (!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "t"))
			str = "1";
		else if (!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "f"))
			str = "0";
		mpfr_set_str(e->result[o->residx]->number, str, 10, o->rndmode);
		break;
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_isnull) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
	mpfr_set_ui(e->result[o->residx]->number, e->ops[0]->result[o->residx]->isnull, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_null) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(o, e, e->ops[0]->result[o->residx]->type);
	e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulldt) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);
	memset(&e->result[o->residx]->datetime, 0, sizeof(e->result[o->residx]->datetime));
	e->result[o->residx]->date_valid = false;
	e->result[o->residx]->time_valid = false;
	e->result[o->residx]->interval = false;
	e->result[o->residx]->day_carry = 0;
	e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulln) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
	e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulls) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);
	e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_iif) {
	int32_t opidx;
	long cond;
	ocrpt_string *string, *sstring;

	if (e->n_ops != 3) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER || e->ops[0]->result[o->residx]->isnull) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	cond = mpfr_get_si(e->ops[0]->result[o->residx]->number, o->rndmode);
	opidx = (cond ? 1 : 2);

	if (!e->ops[opidx]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}
	if (e->ops[opidx]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[opidx]->result[o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(o, e, e->ops[opidx]->result[o->residx]->type);

	if (e->ops[opidx]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	switch (e->ops[opidx]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		mpfr_set(e->result[o->residx]->number, e->ops[opidx]->result[o->residx]->number, o->rndmode);
		break;
	case OCRPT_RESULT_STRING:
		sstring = e->ops[opidx]->result[o->residx]->string;
		string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len);
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}
		e->result[o->residx]->string->len = 0;
		ocrpt_mem_string_append_len(e->result[o->residx]->string, sstring->str, sstring->len);
		break;
	case OCRPT_RESULT_DATETIME:
		e->result[o->residx]->datetime = e->ops[opidx]->result[o->residx]->datetime;
		e->result[o->residx]->date_valid = e->ops[opidx]->result[o->residx]->date_valid;
		e->result[o->residx]->time_valid = e->ops[opidx]->result[o->residx]->time_valid;
		e->result[o->residx]->interval = e->ops[opidx]->result[o->residx]->interval;
		e->result[o->residx]->day_carry = e->ops[opidx]->result[o->residx]->day_carry;
		break;
	default:
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_inc) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		mpfr_add_ui(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, 1, o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_dec) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}

		mpfr_sub_ui(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, 1, o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_error) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx] || e->ops[0]->result[o->residx]->isnull) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_STRING:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_NUMBER:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_concat) {
	ocrpt_string *string;
	int32_t len;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx] || e->ops[i]->result[o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	for (len = 0, i = 0; i < e->n_ops; i++)
		len += e->ops[i]->result[o->residx]->string->len;

	string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		for (i = 0; i < e->n_ops; i++) {
			ocrpt_string *sstring = e->ops[i]->result[o->residx]->string;
			ocrpt_mem_string_append_len(string, sstring->str, sstring->len);
		}
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_left) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t l, len;
	uint32_t i;

	if (e->n_ops != 2 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING || e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	l = mpfr_get_si(e->ops[1]->result[o->residx]->number, o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[o->residx]->string;

	ocrpt_utf8forward(sstring->str, l, NULL, sstring->len, &len);

	string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str, len);
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_right) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t l, start;
	uint32_t i;

	if (e->n_ops != 2 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING || e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	l = mpfr_get_si(e->ops[1]->result[o->residx]->number, o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[o->residx]->string;

	ocrpt_utf8backward(sstring->str, l, NULL, sstring->len, &start);

	string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len - start);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str + start, sstring->len - start);
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_mid) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t ofs, l, start, len;
	uint32_t i;

	if (e->n_ops != 3 || e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING ||
			e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER ||
			e->ops[2]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ofs = mpfr_get_si(e->ops[1]->result[o->residx]->number, o->rndmode);
	l = mpfr_get_si(e->ops[2]->result[o->residx]->number, o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[o->residx]->string;

	if (ofs < 0)
		ocrpt_utf8backward(sstring->str, -ofs, NULL, sstring->len, &start);
	else if (ofs > 0)
		ocrpt_utf8forward(sstring->str, ofs - 1, NULL, sstring->len, &start);
	else
		start = 0;
	ocrpt_utf8forward(sstring->str + start, l, NULL, sstring->len - start, &len);

	string = ocrpt_mem_string_resize(e->result[o->residx]->string, len);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str + start, len);
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_random) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
	mpfr_urandomb(e->result[o->residx]->number, o->randstate);
}

OCRPT_STATIC_FUNCTION(ocrpt_factorial) {
	intmax_t n;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		if (e->ops[0]->result[o->residx]->isnull) {
			ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
			e->result[o->residx]->isnull = true;
			return;
		}

		n = mpfr_get_sj(e->ops[0]->result[o->residx]->number, o->rndmode);
		if (n < 0LL || n > LONG_MAX) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}

		ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
		mpfr_fac_ui(e->result[o->residx]->number, (unsigned long)n, o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_land) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[o->residx]->number, 0) != 0);
	for (i = 1; ret && i < e->n_ops; i++) {
		unsigned long ret1 = (mpfr_cmp_ui(e->ops[i]->result[o->residx]->number, 0) != 0);
		ret = ret && ret1;
	}
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_lor) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[o->residx]->number, 0) != 0);
	for (i = 1; !ret && i < e->n_ops; i++) {
		unsigned long ret1 = (mpfr_cmp_ui(e->ops[i]->result[o->residx]->number, 0) != 0);
		ret = ret || ret1;
	}
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_lnot) {
	intmax_t ret;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[o->residx]->number, 0) == 0);
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_and) {
	uintmax_t ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	for (i = 1; ret && i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[o->residx]->number, o->rndmode);
		ret &= ret1;
	}
	mpfr_set_uj(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_or) {
	uintmax_t ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	for (i = 1; !ret && i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[o->residx]->number, o->rndmode);
		ret |= ret1;
	}
	mpfr_set_uj(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_xor) {
	uintmax_t ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	for (i = 1; i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[o->residx]->number, o->rndmode);
		ret ^= ret1;
	}
	mpfr_set_uj(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_not) {
	intmax_t ret;

	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ret = mpfr_get_uj(e->ops[0]->result[o->residx]->number, 0);
	mpfr_set_ui(e->result[o->residx]->number, ~ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_shl) {
	uintmax_t op, shift;
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	op = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	shift = mpfr_get_uj(e->ops[1]->result[o->residx]->number, o->rndmode);
	mpfr_set_uj(e->result[o->residx]->number, op << shift, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_shr) {
	uintmax_t op, shift;
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	op = mpfr_get_uj(e->ops[0]->result[o->residx]->number, o->rndmode);
	shift = mpfr_get_uj(e->ops[1]->result[o->residx]->number, o->rndmode);
	mpfr_set_uj(e->result[o->residx]->number, op >> shift, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_fmod) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	mpfr_fmod(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_remainder) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	mpfr_fmod(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_rint) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_rint(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_ceil) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_ceil(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_floor) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_floor(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_round) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_round(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_trunc) {
	if (e->n_ops != 1 || !e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_trunc(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_pow) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	mpfr_pow(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, e->ops[1]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_lower) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		char cc[8];

		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		bytes_total = 0;

		while (bytes_total < (utf8proc_ssize_t)sstring->len) {
			bytes_read = utf8proc_iterate((utf8proc_uint8_t *)(sstring->str + bytes_total), sstring->len - bytes_total, &c);
			c = utf8proc_tolower(c);
			bytes_written = utf8proc_encode_char(c, (utf8proc_uint8_t *)cc);
			ocrpt_mem_string_append_len(string, cc, bytes_written);
			bytes_total += bytes_read;
		}
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_upper) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		char cc[8];

		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		bytes_total = 0;

		while (bytes_total < (utf8proc_ssize_t)sstring->len) {
			bytes_read = utf8proc_iterate((utf8proc_uint8_t *)(sstring->str + bytes_total), sstring->len - bytes_total, &c);
			c = utf8proc_toupper(c);
			bytes_written = utf8proc_encode_char(c, (utf8proc_uint8_t *)cc);
			ocrpt_mem_string_append_len(string, cc, bytes_written);
			bytes_total += bytes_read;
		}
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_proper) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		bool first = true;
		char cc[8];

		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		string->len = 0;
		bytes_total = 0;

		while (bytes_total < (utf8proc_ssize_t)sstring->len) {
			bytes_read = utf8proc_iterate((utf8proc_uint8_t *)(sstring->str + bytes_total), sstring->len - bytes_total, &c);
			c = (first ? utf8proc_toupper(c) : utf8proc_tolower(c));
			bytes_written = utf8proc_encode_char(c, (utf8proc_uint8_t *)cc);
			ocrpt_mem_string_append_len(string, cc, bytes_written);
			bytes_total += bytes_read;
			first = false;
		}
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_rownum) {
	if (e->n_ops > 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->n_ops == 1) {
		if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
			return;
		}

		if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}

		if (!e->q && !e->ops[0]->result[o->residx]->isnull) {
			char *qname = e->ops[0]->result[o->residx]->string->str;
			ocrpt_list *ptr;

			for (ptr = o->queries; ptr; ptr = ptr->next) {
				ocrpt_query *tmp = (ocrpt_query *)ptr->data;
				if (strcmp(tmp->name, qname) == 0) {
					e->q = tmp;
					break;
				}
			}
		}
	} else {
		if (r && r->query)
			e->q = r->query;
		if (!e->q && o->queries)
			e->q = (ocrpt_query *)o->queries->data;
	}

	if (!e->q) {
		ocrpt_expr_make_error_result(o, e, "rownum(): no such query");
		return;
	}

	if (!e->result[o->residx]) {
		e->result[o->residx] = e->q->rownum->result[o->residx];
		ocrpt_expr_set_result_owned(e, o->residx, false);
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_brrownum) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->br && !e->ops[0]->result[o->residx]->isnull) {
		char *brname = e->ops[0]->result[o->residx]->string->str;
		ocrpt_list *ptr;

		for (ptr = r->breaks; ptr; ptr = ptr->next) {
			ocrpt_break *tmp = (ocrpt_break *)ptr->data;
			if (strcmp(tmp->name, brname) == 0) {
				e->br = tmp;
				break;
			}
		}
	}

	if (!e->br) {
		ocrpt_expr_make_error_result(o, e, "brrownum(): no such break");
		return;
	}

	if (!e->result[o->residx]) {
		assert(!e->result[o->residx]);
		e->result[o->residx] = e->br->rownum->result[o->residx];
		ocrpt_expr_set_result_owned(e, o->residx, false);
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_stodt) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING && e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_DATETIME) {
		e->result[o->residx]->datetime = e->ops[0]->result[o->residx]->datetime;
		e->result[o->residx]->date_valid = e->ops[0]->result[o->residx]->date_valid;
		e->result[o->residx]->time_valid = e->ops[0]->result[o->residx]->time_valid;
		e->result[o->residx]->interval = e->ops[0]->result[o->residx]->interval;
		e->result[o->residx]->day_carry = e->ops[0]->result[o->residx]->day_carry;
	} else if (!ocrpt_parse_datetime(o, e->ops[0]->result[o->residx]->string->str, e->ops[0]->result[o->residx]->string->len, e->result[o->residx]))
		if (!ocrpt_parse_interval(o, e->ops[0]->result[o->residx]->string->str, e->ops[0]->result[o->residx]->string->len, e->result[o->residx]))
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_dtos) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->interval) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[o->residx]->isnull || !e->ops[0]->result[o->residx]->date_valid) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ocrpt_string *string = ocrpt_mem_string_resize(e->result[o->residx]->string, 64);

	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}

		char *dfmt = nl_langinfo_l(D_FMT, o->locale);
		strftime(e->result[o->residx]->string->str, e->result[o->residx]->string->allocated_len, dfmt, &e->ops[0]->result[o->residx]->datetime);
	} else
		ocrpt_expr_make_error_result(o, e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_date) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!o->current_date->date_valid) {
		time_t t = time(NULL);
		localtime_r(&t, &o->current_date->datetime);
		o->current_date->date_valid = true;
		o->current_date->time_valid = false;
	}

	if (!e->result[o->residx]) {
		e->result[o->residx] = o->current_date;
		ocrpt_expr_set_result_owned(e, o->residx, false);
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_now) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!o->current_date->date_valid) {
		time_t t = time(NULL);
		localtime_r(&t, &o->current_timestamp->datetime);
		o->current_timestamp->date_valid = true;
		o->current_timestamp->time_valid = true;
	}

	if (!e->result[o->residx]) {
		e->result[o->residx] = o->current_timestamp;
		ocrpt_expr_set_result_owned(e, o->residx, false);
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_year) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull || (!e->ops[0]->result[o->residx]->interval && !e->ops[0]->result[o->residx]->date_valid)) {
		e->result[o->residx]->isnull = true;
		return;
	}

	int add = (e->ops[0]->result[o->residx]->interval ? 0 : 1900);
	mpfr_set_si(e->result[o->residx]->number, e->ops[0]->result[o->residx]->datetime.tm_year + add, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_month) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull || (!e->ops[0]->result[o->residx]->interval && !e->ops[0]->result[o->residx]->date_valid)) {
		e->result[o->residx]->isnull = true;
		return;
	}

	int add = (e->ops[0]->result[o->residx]->interval ? 0 : 1);
	mpfr_set_si(e->result[o->residx]->number, e->ops[0]->result[o->residx]->datetime.tm_mon + add, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_day) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull || (!e->ops[0]->result[o->residx]->interval && !e->ops[0]->result[o->residx]->date_valid)) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_set_si(e->result[o->residx]->number, e->ops[0]->result[o->residx]->datetime.tm_mday, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_dim) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[o->residx]->interval) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull || !e->ops[0]->result[o->residx]->date_valid) {
		e->result[o->residx]->isnull = true;
		return;
	}

	int mon = e->ops[0]->result[o->residx]->datetime.tm_mon;
	int year = e->ops[0]->result[o->residx]->datetime.tm_year;
	mpfr_set_si(e->result[o->residx]->number, days_in_month[ocrpt_leap_year(year)][mon], o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_wiy) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[o->residx]->interval) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull || !e->ops[0]->result[o->residx]->date_valid) {
		e->result[o->residx]->isnull = true;
		return;
	}

	char wiy[64] = "";
	strftime(wiy, sizeof(wiy), "%U", &e->ops[0]->result[o->residx]->datetime);
	mpfr_set_si(e->result[o->residx]->number, atoi(wiy), o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_wiy1) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[o->residx]->interval) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull || !e->ops[0]->result[o->residx]->date_valid) {
		e->result[o->residx]->isnull = true;
		return;
	}

	char wiy[64] = "";
	strftime(wiy, sizeof(wiy), "%W", &e->ops[0]->result[o->residx]->datetime);
	mpfr_set_si(e->result[o->residx]->number, atoi(wiy), o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_wiyo) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[o->residx]->interval) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	if (!e->ops[0]->result[o->residx]->date_valid) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ocrpt_result res = { .type = OCRPT_RESULT_DATETIME, .interval = false };
	res.datetime = e->ops[0]->result[o->residx]->datetime;
	res.date_valid = e->ops[0]->result[o->residx]->date_valid;
	res.time_valid = false;

	char wiy[64] = "";
	int wyear, wyearofs;
	long ofs = mpfr_get_si(e->ops[1]->result[o->residx]->number, o->rndmode);
	while (ofs < 0)
		ofs += 7;
	ofs = ofs % 7;

	strftime(wiy, sizeof(wiy), "%U", &res.datetime);
	wyear = atoi(wiy);

	ocrpt_datetime_result_add_number(o, &res, e->ops[0]->result[o->residx], -ofs);

	res.datetime.tm_isdst = -1;
	res.datetime.tm_wday = -1;
	time_t t = mktime(&res.datetime);
	localtime_r(&t, &res.datetime);

	strftime(wiy, sizeof(wiy), "%U", &res.datetime);
	wyearofs = atoi(wiy);

	if (wyearofs > wyear)
		wyearofs = 0;

	mpfr_set_si(e->result[o->residx]->number, wyearofs, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_stdwiy) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[o->residx]->interval) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull || !e->ops[0]->result[o->residx]->date_valid) {
		e->result[o->residx]->isnull = true;
		return;
	}

	char wiy[64] = "";
	strftime(wiy, sizeof(wiy), "%V", &e->ops[0]->result[o->residx]->datetime);
	mpfr_set_si(e->result[o->residx]->number, atoi(wiy), o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_dateof) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ocrpt_result_copy(o, e->result[o->residx], e->ops[0]->result[o->residx]);

	e->result[o->residx]->datetime.tm_hour = 0;
	e->result[o->residx]->datetime.tm_min = 0;
	e->result[o->residx]->datetime.tm_sec = 0;
	e->result[o->residx]->date_valid = e->ops[0]->result[o->residx]->date_valid;
	e->result[o->residx]->time_valid = false;
	e->result[o->residx]->interval = e->ops[0]->result[o->residx]->interval;

	if (!e->result[o->residx]->interval && !e->result[o->residx]->date_valid)
		e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_timeof) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	ocrpt_result_copy(o, e->result[o->residx], e->ops[0]->result[o->residx]);

	e->result[o->residx]->datetime.tm_year = 0;
	e->result[o->residx]->datetime.tm_mon = 0;
	e->result[o->residx]->datetime.tm_mday = 0;
	e->result[o->residx]->date_valid = false;
	e->result[o->residx]->time_valid = e->ops[0]->result[o->residx]->time_valid;
	e->result[o->residx]->interval = e->ops[0]->result[o->residx]->interval;

	if (!e->result[o->residx]->interval && !e->result[o->residx]->time_valid)
		e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_chgdateof) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[i]->result[o->residx]->interval) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ocrpt_result_copy(o, e->result[o->residx], e->ops[0]->result[o->residx]);

	e->result[o->residx]->datetime.tm_year = e->ops[1]->result[o->residx]->datetime.tm_year;
	e->result[o->residx]->datetime.tm_mon = e->ops[1]->result[o->residx]->datetime.tm_mon;
	e->result[o->residx]->datetime.tm_mday = e->ops[1]->result[o->residx]->datetime.tm_mday;
	e->result[o->residx]->date_valid = e->ops[1]->result[o->residx]->date_valid;

	if (!e->result[o->residx]->date_valid && !e->result[o->residx]->time_valid)
		e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_chgtimeof) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[i]->result[o->residx]->interval) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ocrpt_result_copy(o, e->result[o->residx], e->ops[0]->result[o->residx]);

	e->result[o->residx]->datetime.tm_hour = e->ops[1]->result[o->residx]->datetime.tm_hour;
	e->result[o->residx]->datetime.tm_min = e->ops[1]->result[o->residx]->datetime.tm_min;
	e->result[o->residx]->datetime.tm_sec = e->ops[1]->result[o->residx]->datetime.tm_sec;
	e->result[o->residx]->time_valid = e->ops[1]->result[o->residx]->time_valid;

	if (!e->result[o->residx]->date_valid && !e->result[o->residx]->time_valid)
		e->result[o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_gettimeinsecs) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[o->residx]->interval) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull || !e->ops[0]->result[o->residx]->time_valid) {
		e->result[o->residx]->isnull = true;
		return;
	}

	int ret = e->ops[0]->result[o->residx]->datetime.tm_hour * 3600 + e->ops[0]->result[o->residx]->datetime.tm_min * 60 + e->ops[0]->result[o->residx]->datetime.tm_sec;
	mpfr_set_ui(e->result[o->residx]->number, ret, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_settimeinsecs) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[o->residx]->interval ||
		e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	ocrpt_result_copy(o, e->result[o->residx], e->ops[0]->result[o->residx]);

	long ret = mpfr_get_ui(e->ops[1]->result[o->residx]->number, o->rndmode);
	if (ret < 0 || ret >= 86400) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}


	e->result[o->residx]->datetime.tm_sec = ret % 60;
	ret /= 60;
	e->result[o->residx]->datetime.tm_min = ret % 60;
	ret /= 60;
	e->result[o->residx]->datetime.tm_hour = ret;
	e->result[o->residx]->time_valid = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_interval) {
	uint32_t i;

	if (e->n_ops != 1 && e->n_ops != 6) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	if (e->n_ops == 6) {
		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
				ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
				return;
			}
		}
	} else {
		if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_DATETIME);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	if (e->n_ops == 6) {
		int year = mpfr_get_ui(e->ops[0]->result[o->residx]->number, o->rndmode);
		int mon  = mpfr_get_ui(e->ops[1]->result[o->residx]->number, o->rndmode);
		int mday = mpfr_get_ui(e->ops[2]->result[o->residx]->number, o->rndmode);
		int hour = mpfr_get_ui(e->ops[3]->result[o->residx]->number, o->rndmode);
		int min  = mpfr_get_ui(e->ops[4]->result[o->residx]->number, o->rndmode);
		int sec  = mpfr_get_ui(e->ops[5]->result[o->residx]->number, o->rndmode);

		memset(&e->result[o->residx]->datetime, 0, sizeof(e->result[o->residx]->datetime));
		e->result[o->residx]->datetime.tm_year = year;
		e->result[o->residx]->datetime.tm_mon = mon;
		e->result[o->residx]->datetime.tm_mday = mday;
		e->result[o->residx]->datetime.tm_hour = hour;
		e->result[o->residx]->datetime.tm_min = min;
		e->result[o->residx]->datetime.tm_sec = sec;
		e->result[o->residx]->date_valid = false;
		e->result[o->residx]->time_valid = false;
		e->result[o->residx]->interval = true;
	} else if (!ocrpt_parse_interval(o, e->ops[0]->result[o->residx]->string->str, e->ops[0]->result[o->residx]->string->len, e->result[o->residx]))
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_cos) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_cos(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_sin) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_sin(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_tan) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_tan(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_acos) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_acos(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_asin) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_asin(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_atan) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_atan(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_sec) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_sec(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_csc) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_csc(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_cot) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_cot(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_log) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_log(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_log2) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_log2(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_log10) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_log10(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_exp) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_exp(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_exp2) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_exp2(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_exp10) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_exp10(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_sqr) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_sqr(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_sqrt) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[o->residx]) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(o, e, e->ops[0]->result[o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[o->residx]->isnull) {
		e->result[o->residx]->isnull = true;
		return;
	}

	mpfr_sqrt(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_fxpval) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	if ((e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING && e->ops[0]->result[o->residx]->type != OCRPT_RESULT_NUMBER) || e->ops[1]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	if (e->ops[0]->result[o->residx]->type == OCRPT_RESULT_NUMBER)
		mpfr_set(e->result[o->residx]->number, e->ops[0]->result[o->residx]->number, o->rndmode);
	else
		mpfr_set_str(e->result[o->residx]->number, e->ops[0]->result[o->residx]->string->str, 10, o->rndmode);

	mpfr_t tmp;

	mpfr_init2(tmp, o->prec);
	mpfr_exp10(tmp, e->ops[1]->result[o->residx]->number, o->rndmode);
	mpfr_div(e->result[o->residx]->number, e->result[o->residx]->number, tmp, o->rndmode);
	mpfr_clear(tmp);
}

OCRPT_STATIC_FUNCTION(ocrpt_str) {
	uint32_t i;

	if (e->n_ops != 3) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->isnull) {
			e->result[o->residx]->isnull = true;
			return;
		}
	}

	int len = mpfr_get_ui(e->ops[1]->result[o->residx]->number, o->rndmode);
	int decimal = mpfr_get_ui(e->ops[2]->result[o->residx]->number, o->rndmode);
	char fmt[16];

	sprintf(fmt, "%%%d.%dRf", len, decimal);

	len = mpfr_snprintf(NULL, 0, fmt, e->ops[0]->result[o->residx]->number);

	ocrpt_string *string = ocrpt_mem_string_resize(e->result[o->residx]->string, len + 1);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}
		string->len = 0;
	} else {
		ocrpt_expr_make_error_result(o, e, "out of memory");
		return;
	}

	len = mpfr_snprintf(e->result[o->residx]->string->str, len + 1, fmt, e->ops[0]->result[o->residx]->number);
	e->result[o->residx]->string->len = len;
}

OCRPT_STATIC_FUNCTION(ocrpt_format) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	if (e->ops[1]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	char *formatstring = NULL;
	int32_t formatlen = 0;

	switch (e->ops[0]->result[o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		if (e->ops[1]->result[o->residx]->isnull || e->ops[1]->result[o->residx]->string->len == 0)
			formatstring = "%d";
		break;
	case OCRPT_RESULT_STRING:
		if (e->ops[1]->result[o->residx]->isnull || e->ops[1]->result[o->residx]->string->len == 0)
			formatstring = "%s";
		break;
	case OCRPT_RESULT_DATETIME:
		/*
		 * Result would be garbage for intervals.
		 * Let's return an empty string.
		 */
		if (e->ops[0]->result[o->residx]->interval) {
			e->result[o->residx]->string->str[0] = 0;
			e->result[o->residx]->string->len = 0;
			return;
		}
		if (e->ops[1]->result[o->residx]->isnull || e->ops[1]->result[o->residx]->string->len == 0)
			formatstring = nl_langinfo_l(D_FMT, o->locale);
		break;
	case OCRPT_RESULT_ERROR:
		/* This case is caught earlier */
		return;
	}

	if (!formatstring) {
		formatstring = e->ops[1]->result[o->residx]->string->str;
		formatlen = e->ops[1]->result[o->residx]->string->len;
	} else
		formatlen = strlen(formatstring);

	ocrpt_format_string(o, e, e->result[o->residx]->string, formatstring, formatlen, e->ops, 1);
}

OCRPT_STATIC_FUNCTION(ocrpt_dtosf) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	if (e->ops[1]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	/*
	 * Result would be garbage for intervals.
	 * Let's return an empty string.
	 */
	if (e->ops[0]->result[o->residx]->interval) {
		e->result[o->residx]->string->str[0] = 0;
		e->result[o->residx]->string->len = 0;
		return;
	}

	char *formatstring = NULL;
	int32_t formatlen = 0;

	if (e->ops[1]->result[o->residx]->isnull || e->ops[1]->result[o->residx]->string->len == 0)
		formatstring = nl_langinfo_l(D_FMT, o->locale);

	if (!formatstring) {
		formatstring = e->ops[1]->result[o->residx]->string->str;
		formatlen = e->ops[1]->result[o->residx]->string->len;
	} else
		formatlen = strlen(formatstring);

	ocrpt_format_string(o, e, e->result[o->residx]->string, formatstring, formatlen, e->ops, 1);
}

OCRPT_STATIC_FUNCTION(ocrpt_printf) {
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[o->residx]) {
			ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(o, e, e->ops[i]->result[o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(o, e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);

	ocrpt_format_string(o, e, e->result[o->residx]->string, e->ops[0]->result[o->residx]->string->str, e->ops[0]->result[o->residx]->string->len, &e->ops[1], e->n_ops - 1);
}

/*
 * Keep this sorted by function name because it is
 * used via bsearch()
 */
static const ocrpt_function ocrpt_functions[] = {
	{ "abs",		ocrpt_abs,	1,	false,	false,	false,	false },
	{ "acos",		ocrpt_acos,	1,	false,	false,	false,	false },
	{ "add",		ocrpt_add,	-1,	true,	true,	false,	false },
	{ "and",		ocrpt_and,	-1,	true,	true,	false,	false },
	{ "asin",		ocrpt_asin,	1,	false,	false,	false,	false },
	{ "atan",		ocrpt_atan,	1,	false,	false,	false,	false },
	{ "brrownum",	ocrpt_brrownum,	1,	false,	false,	false,	true },
	{ "ceil",		ocrpt_ceil,	1,	false,	false,	false,	false },
	{ "chgdateof",	ocrpt_chgdateof,	2,	false,	false,	false,	false },
	{ "chgtimeof",	ocrpt_chgtimeof,	2,	false,	false,	false,	false },
	{ "concat",		ocrpt_concat,	-1,	false,	false,	false,	false },
	{ "cos",		ocrpt_cos,	1,	false,	false,	false,	false },
	{ "cot",		ocrpt_cot,	1,	false,	false,	false,	false },
	{ "csc",		ocrpt_csc,	1,	false,	false,	false,	false },
	{ "date",		ocrpt_date,	0,	false,	false,	false,	false },
	{ "dateof",		ocrpt_dateof,	1,	false,	false,	false,	false },
	{ "day",		ocrpt_day,	1,	false,	false,	false,	false },
	{ "dec",		ocrpt_dec,	1,	false,	false,	false,	false },
	{ "dim",		ocrpt_dim,	1,	false,	false,	false,	false },
	{ "div",		ocrpt_div,	-1,	false,	false,	true,	false },
	{ "dtos",		ocrpt_dtos,	1,	false,	false,	false,	false },
	{ "dtosf",		ocrpt_dtosf,	2,	false,	false,	false,	false },
	{ "eq",			ocrpt_eq,	2,	true,	false,	false,	false },
	{ "error",		ocrpt_error,	1,	false,	false,	false,	false },
	{ "exp",		ocrpt_exp,	1,	false,	false,	false,	false },
	{ "exp10",		ocrpt_exp10,	1,	false,	false,	false,	false },
	{ "exp2",		ocrpt_exp2,	1,	false,	false,	false,	false },
	{ "factorial",	ocrpt_factorial,	1,	false,	false,	false,	false },
	{ "floor",		ocrpt_floor,	1,	false,	false,	false,	false },
	{ "fmod",		ocrpt_fmod,	2,	false,	false,	false,	false },
	{ "format" ,	ocrpt_format,	2,	false,	false,	false,	false },
	{ "fxpval",		ocrpt_fxpval,	2,	false,	false,	false,	false },
	{ "ge",			ocrpt_ge,	2,	false,	false,	false,	false },
	{ "gettimeinsecs",	ocrpt_gettimeinsecs,	1,	false,	false,	false,	false },
	{ "gt",			ocrpt_gt,	2,	false,	false,	false,	false },
	{ "iif",		ocrpt_iif,	3,	false,	false,	false,	false },
	{ "inc",		ocrpt_inc,	1,	false,	false,	false,	false },
	{ "interval",	ocrpt_interval,	-1,	false,	false,	false,	false },
	{ "isnull",		ocrpt_isnull,	1,	false,	false,	false,	false },
	{ "land",		ocrpt_land,	-1,	true,	true,	false,	false },
	{ "le",			ocrpt_le,	2,	false,	false,	false,	false },
	{ "left",		ocrpt_left,	2,	false,	false,	false,	false },
	{ "ln",			ocrpt_log,	1,	false,	false,	false,	false },
	{ "lnot",		ocrpt_lnot,	1,	false,	false,	false,	false },
	{ "log",		ocrpt_log,	1,	false,	false,	false,	false },
	{ "log10",		ocrpt_log10,	1,	false,	false,	false,	false },
	{ "log2",		ocrpt_log2,	1,	false,	false,	false,	false },
	{ "lor",		ocrpt_lor,	-1,	true,	true,	false,	false },
	{ "lower",		ocrpt_lower,	1,	false,	false,	false,	false },
	{ "lt",			ocrpt_lt,	2,	false,	false,	false,	false },
	{ "mid",		ocrpt_mid,	3,	false,	false,	false,	false },
	{ "mod",		ocrpt_remainder,	2,	false,	false,	false,	false },
	{ "month",		ocrpt_month,	1,	false,	false,	false,	false },
	{ "mul",		ocrpt_mul,	-1,	true,	true,	false,	false },
	{ "ne",			ocrpt_ne,	2,	true,	false,	false,	false },
	{ "not",		ocrpt_not,	1,	false,	false,	false,	false },
	{ "now",		ocrpt_now,	0,	false,	false,	false,	false },
	{ "null",		ocrpt_null,	1,	false,	false,	false,	false },
	{ "nulldt",		ocrpt_nulldt,	0,	false,	false,	false,	false },
	{ "nulln",		ocrpt_nulln,	0,	false,	false,	false,	false },
	{ "nulls",		ocrpt_nulls,	0,	false,	false,	false,	false },
	{ "or",			ocrpt_or,	-1,	true,	true,	false,	false },
	{ "pow",		ocrpt_pow,	2,	false,	false,	false,	false },
	{ "printf",		ocrpt_printf,	-1,	false,	false,	false,	false },
	{ "proper",		ocrpt_proper,	1,	false,	false,	false,	false },
	{ "random",		ocrpt_random,	0,	false,	false,	false,	true },
	{ "remainder",	ocrpt_remainder,	2,	false,	false,	false,	false },
	{ "right",		ocrpt_right,	2,	false,	false,	false,	false },
	{ "rint",		ocrpt_rint,	1,	false,	false,	false,	false },
	{ "round",		ocrpt_round,	1,	false,	false,	false,	false },
	{ "rownum",		ocrpt_rownum,	-1,	false,	false,	false,	true },
	{ "sec",		ocrpt_sec,	1,	false,	false,	false,	false },
	{ "settimeinsecs",	ocrpt_settimeinsecs,	2,	false,	false,	false,	false },
	{ "shl",		ocrpt_shl,	2,	false,	false,	false,	false },
	{ "shr",		ocrpt_shr,	2,	false,	false,	false,	false },
	{ "sin",		ocrpt_sin,	1,	false,	false,	false,	false },
	{ "sqr",		ocrpt_sqr,	1,	false,	false,	false,	false },
	{ "sqrt",		ocrpt_sqrt,	1,	false,	false,	false,	false },
	{ "stdwiy",		ocrpt_stdwiy,	1,	false,	false,	false,	false },
	{ "stod",		ocrpt_stodt,	1,	false,	false,	false,	false },
	{ "stodt",		ocrpt_stodt,	1,	false,	false,	false,	false },
	{ "stodtsql",	ocrpt_stodt,	1,	false,	false,	false,	false },
	{ "str",		ocrpt_str,		3,	false,	false,	false,	false },
	{ "sub",		ocrpt_sub,	-1,	false,	false,	false,	false },
	{ "tan",		ocrpt_tan,	1,	false,	false,	false,	false },
	{ "timeof",		ocrpt_timeof,	1,	false,	false,	false,	false },
	{ "trunc",		ocrpt_trunc,	1,	false,	false,	false,	false },
	{ "tstod",		ocrpt_stodt,	1,	false,	false,	false,	false },
	{ "uminus",		ocrpt_uminus,	1,	false,	false,	false,	false },
	{ "upper",		ocrpt_upper,	1,	false,	false,	false,	false },
	{ "val",		ocrpt_val,	1,	false,	false,	false,	false },
	{ "wiy",		ocrpt_wiy,	1,	false,	false,	false,	false },
	{ "wiy1",		ocrpt_wiy1,	1,	false,	false,	false,	false },
	{ "wiyo",		ocrpt_wiyo,	2,	false,	false,	false,	false },
	{ "xor",		ocrpt_xor,	-1,	true,	true,	true,	false },
	{ "year",		ocrpt_year,	1,	false,	false,	false,	false },
};

static int n_ocrpt_functions = sizeof(ocrpt_functions) / sizeof(ocrpt_function);

static int funccmp(const void *key, const void *f) {
	return strcasecmp((const char *)key, ((const ocrpt_function *)f)->fname);
}

static int funccmpind(const void *key, const void *f) {
	return strcasecmp((const char *)key, (*(const ocrpt_function **)f)->fname);
}

static int funcsortind(const void *key1, const void *key2) {
	return strcasecmp((*(const ocrpt_function **)key1)->fname, (*(const ocrpt_function **)key2)->fname);
}

DLL_EXPORT_SYM bool ocrpt_function_add(opencreport *o, const char *fname, ocrpt_function_call func,
										int32_t n_ops, bool commutative, bool associative,
										bool left_associative, bool dont_optimize) {
	ocrpt_function *new_func;
	ocrpt_function **f_array;

	if (!fname || !*fname || !func)
		return false;

	new_func = ocrpt_mem_malloc(sizeof(ocrpt_function));
	if (!new_func)
		return false;

	f_array = ocrpt_mem_realloc(o->functions, (o->n_functions + 1) * sizeof(ocrpt_function *));
	if (!f_array) {
		ocrpt_mem_free(new_func);
		return false;
	}

	new_func->fname = ocrpt_mem_strdup(fname);
	if (!new_func->fname) {
		ocrpt_mem_free(new_func);
		return false;
	}

	new_func->func = func;
	new_func->n_ops = n_ops;
	new_func->commutative = commutative;
	new_func->associative = associative;
	new_func->left_associative = left_associative;
	new_func->dont_optimize = dont_optimize;

	o->functions = f_array;
	o->functions[o->n_functions++] = new_func;

	qsort(o->functions, o->n_functions, sizeof(ocrpt_function *), funcsortind);

	return true;
}

DLL_EXPORT_SYM const ocrpt_function *ocrpt_function_get(opencreport *o, const char *fname) {
	ocrpt_function **ret;

	if (o->functions) {
		ret = bsearch(fname, o->functions, o->n_functions, sizeof(ocrpt_function *), funccmpind);
		if (ret)
			return *ret;
	}

	return bsearch(fname, ocrpt_functions, n_ocrpt_functions, sizeof(ocrpt_function), funccmp);
}
