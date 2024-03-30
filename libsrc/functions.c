/*
 * OpenCReports expression function handling
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
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

static bool ocrpt_expr_init_result_internal(ocrpt_expr *e, enum ocrpt_result_type type, unsigned int which) {
	ocrpt_result *result = e->result[which];

	if (!result) {
		result = ocrpt_result_new(e->o);
		if (result) {
			e->result[which] = result;
			ocrpt_expr_set_result_owned(e, which, true);
		}
	}
	if (result) {
		switch (type) {
		case OCRPT_RESULT_STRING: {
			ocrpt_string *string = ocrpt_mem_string_resize(result->string, 16);
			if (string) {
				if (!result->string) {
					result->string = string;
					result->string_owned = true;
				}
				string->len = 0;
			}
			break;
		}
		case OCRPT_RESULT_NUMBER:
			if (!result->number_initialized) {
				mpfr_init2(result->number, e->o->prec);
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

DLL_EXPORT_SYM bool ocrpt_expr_init_result(ocrpt_expr *e, enum ocrpt_result_type type) {
	if (!e)
		return false;

	return ocrpt_expr_init_result_internal(e, type, e->o->residx);
}

DLL_EXPORT_SYM void ocrpt_expr_init_results(ocrpt_expr *e, enum ocrpt_result_type type) {
	if (!e)
		return;

	for (uint32_t i = 0; i < OCRPT_EXPR_RESULTS; i++)
		ocrpt_expr_init_result_internal(e, type, i);
}

OCRPT_STATIC_FUNCTION(ocrpt_abs) {
	if (e->n_ops == 1 && e->ops[0]->result[e->o->residx] && e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}

		mpfr_abs(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_uminus) {
	if (e->n_ops == 1 && e->ops[0]->result[e->o->residx] && e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_NUMBER) {
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}

		e->result[e->o->residx]->type = OCRPT_RESULT_NUMBER;
		mpfr_neg(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_add) {
	uint32_t nnum, nstr, ndt;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	nstr = 0;
	ndt = 0;
	for (uint32_t i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]) {
			if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_NUMBER)
				nnum++;
			if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_STRING)
				nstr++;
			if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_DATETIME)
				ndt++;
		}
	}

	if (nnum == e->n_ops) {
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		for (uint32_t i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[e->o->residx]->isnull) {
				e->result[e->o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
		for (uint32_t i = 1; i < e->n_ops; i++)
			mpfr_add(e->result[e->o->residx]->number, e->result[e->o->residx]->number, e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
	} else if (nstr == e->n_ops) {
		ocrpt_string *string;
		int32_t len;
		uint32_t i;

		ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[e->o->residx]->isnull) {
				e->result[e->o->residx]->isnull = true;
				return;
			}
		}

		for (len = 0, i = 0; i < e->n_ops; i++)
			len += e->ops[i]->result[e->o->residx]->string->len;

		string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, len);
		if (string) {
			if (!e->result[e->o->residx]->string) {
				e->result[e->o->residx]->string = string;
				e->result[e->o->residx]->string_owned = true;
			}
			string->len = 0;
			for (i = 0; i < e->n_ops; i++) {
				ocrpt_string *sstring = e->ops[i]->result[e->o->residx]->string;
				ocrpt_mem_string_append_len(string, sstring->str, sstring->len);
			}
		} else
			ocrpt_expr_make_error_result(e, "out of memory");
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
		ocrpt_expr_init_result(e, e->ops[0]->result[e->o->residx]->type);
		ocrpt_result_copy(e->result[e->o->residx], e->ops[0]->result[e->o->residx]);

		for (uint32_t i = 1; i < e->n_ops; i++) {
			switch (e->result[e->o->residx]->type) {
			case OCRPT_RESULT_NUMBER:
				switch (e->ops[i]->result[e->o->residx]->type) {
				case OCRPT_RESULT_NUMBER:
					mpfr_add(e->result[e->o->residx]->number, e->result[e->o->residx]->number, e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
					break;
				case OCRPT_RESULT_DATETIME:
					if (e->ops[i]->result[e->o->residx]->interval) {
						ocrpt_expr_make_error_result(e, "invalid operand(s)");
						return;
					}
					e->result[e->o->residx]->type = OCRPT_RESULT_DATETIME;
					ocrpt_datetime_add_number(e->o, e, e->ops[i]->result[e->o->residx], e->result[e->o->residx]);
					break;
				default:
					/* cannot happen */
					ocrpt_expr_make_error_result(e, "invalid operand(s)");
					return;
				}
				break;
			case OCRPT_RESULT_DATETIME:
				switch (e->ops[i]->result[e->o->residx]->type) {
				case OCRPT_RESULT_NUMBER:
					if (e->result[e->o->residx]->interval) {
						ocrpt_expr_make_error_result(e, "invalid operand(s)");
						return;
					}
					ocrpt_datetime_add_number(e->o, e, e->result[e->o->residx], e->ops[i]->result[e->o->residx]);
					break;
				case OCRPT_RESULT_DATETIME:
					if (!e->result[e->o->residx]->interval && !e->ops[i]->result[e->o->residx]->interval) {
						ocrpt_expr_make_error_result(e, "invalid operand(s)");
						return;
					}
					if (e->result[e->o->residx]->interval && !e->ops[i]->result[e->o->residx]->interval) {
						ocrpt_result interval = { .type = OCRPT_RESULT_DATETIME };
						ocrpt_result_copy(&interval, e->result[e->o->residx]);
						ocrpt_result_copy(e->result[e->o->residx], e->ops[i]->result[e->o->residx]);
						ocrpt_datetime_add_interval(e->o, e, e->result[e->o->residx], &interval);
					} else
						ocrpt_datetime_add_interval(e->o, e, e->result[e->o->residx], e->ops[i]->result[e->o->residx]);
					break;
				default:
					/* cannot happpen */
					ocrpt_expr_make_error_result(e, "invalid operand(s)");
					return;
				}
				break;
			default:
				/* cannot happen */
				ocrpt_expr_make_error_result(e, "invalid operand(s)");
				return;
			}
		}
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_sub) {
	uint32_t i, nnum = 0, ndt = 0;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	ndt = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]) {
			if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_NUMBER)
				nnum++;
			if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_DATETIME) {
				ndt++;
			}
		}
	}

	if (nnum == e->n_ops) {
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[e->o->residx]->isnull) {
				e->result[e->o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_sub(e->result[e->o->residx]->number, e->result[e->o->residx]->number, e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
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
		ocrpt_expr_init_result(e, e->ops[0]->result[e->o->residx]->type);
		ocrpt_result_copy(e->result[e->o->residx], e->ops[0]->result[e->o->residx]);

		for (uint32_t i = 1; i < e->n_ops; i++) {
			switch (e->result[e->o->residx]->type) {
			case OCRPT_RESULT_NUMBER:
				switch (e->ops[i]->result[e->o->residx]->type) {
				case OCRPT_RESULT_NUMBER:
					mpfr_sub(e->result[e->o->residx]->number, e->result[e->o->residx]->number, e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
					break;
				case OCRPT_RESULT_DATETIME:
					ocrpt_expr_make_error_result(e, "invalid operand(s)");
					return;
				default:
					/* cannot happen */
					ocrpt_expr_make_error_result(e, "invalid operand(s)");
					return;
				}
				break;
			case OCRPT_RESULT_DATETIME:
				switch (e->ops[i]->result[e->o->residx]->type) {
				case OCRPT_RESULT_NUMBER:
					if (e->result[e->o->residx]->interval) {
						ocrpt_expr_make_error_result(e, "invalid operand(s)");
						return;
					}
					ocrpt_datetime_sub_number(e->o, e, e->result[e->o->residx], e->ops[i]->result[e->o->residx]);
					break;
				case OCRPT_RESULT_DATETIME:
					if (e->result[e->o->residx]->interval && !e->ops[i]->result[e->o->residx]->interval) {
						ocrpt_expr_make_error_result(e, "invalid operand(s)");
						return;
					}
					ocrpt_datetime_sub_datetime(e->o, e, e->result[e->o->residx], e->ops[i]->result[e->o->residx]);
					break;
				default:
					/* cannot happpen */
					ocrpt_expr_make_error_result(e, "invalid operand(s)");
					return;
				}
				break;
			default:
				/* cannot happen */
				ocrpt_expr_make_error_result(e, "invalid operand(s)");
				return;
			}
		}
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_mul) {
	uint32_t i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx] && e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_NUMBER)
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[e->o->residx]->isnull) {
				e->result[e->o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_mul(e->result[e->o->residx]->number, e->result[e->o->residx]->number, e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_div) {
	uint32_t i, nnum;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	nnum = 0;
	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx] && e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_NUMBER)
			nnum++;
	}

	if (nnum == e->n_ops) {
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[e->o->residx]->isnull) {
				e->result[e->o->residx]->isnull = true;
				return;
			}
		}

		mpfr_set(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
		for (i = 1; i < e->n_ops; i++)
			mpfr_div(e->result[e->o->residx]->number, e->result[e->o->residx]->number, e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
	} else
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_eq) {
	unsigned long ret;

	if (e->n_ops != 2 || !e->ops[0]->result[e->o->residx] || !e->ops[1]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != e->ops[1]->result[e->o->residx]->type) {
		if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		else if (e->ops[1]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[1]->result[e->o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (uint32_t i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number) == 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[e->o->residx]->string->str, e->ops[1]->result[e->o->residx]->string->str) == 0);
		break;
	case OCRPT_RESULT_DATETIME:
		if ((e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid &&
				e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) ||
			(e->ops[0]->result[e->o->residx]->interval && e->ops[1]->result[e->o->residx]->interval)) {
			ret =
				(e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_mday == e->ops[1]->result[e->o->residx]->datetime.tm_mday) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_sec == e->ops[1]->result[e->o->residx]->datetime.tm_sec);
		} else if (e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid) {
			ret =
				(e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_mday == e->ops[1]->result[e->o->residx]->datetime.tm_mday);
		} else if (e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) {
			ret =
				(e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) &&
				(e->ops[0]->result[e->o->residx]->datetime.tm_sec == e->ops[1]->result[e->o->residx]->datetime.tm_sec);
		} else
			ret = false;
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_ne) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[e->o->residx] || !e->ops[1]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != e->ops[1]->result[e->o->residx]->type) {
		if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		else if (e->ops[1]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[1]->result[e->o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number) != 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[e->o->residx]->string->str, e->ops[1]->result[e->o->residx]->string->str) != 0);
		break;
	case OCRPT_RESULT_DATETIME:
		if ((e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid &&
				e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) ||
			(e->ops[0]->result[e->o->residx]->interval && e->ops[1]->result[e->o->residx]->interval)) {
			ret =
				(e->ops[0]->result[e->o->residx]->datetime.tm_year != e->ops[1]->result[e->o->residx]->datetime.tm_year) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_mon != e->ops[1]->result[e->o->residx]->datetime.tm_mon) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_mday != e->ops[1]->result[e->o->residx]->datetime.tm_mday) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_hour != e->ops[1]->result[e->o->residx]->datetime.tm_hour) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_min != e->ops[1]->result[e->o->residx]->datetime.tm_min) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_sec != e->ops[1]->result[e->o->residx]->datetime.tm_sec);
		} else if (e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid) {
			ret =
				(e->ops[0]->result[e->o->residx]->datetime.tm_year != e->ops[1]->result[e->o->residx]->datetime.tm_year) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_mon != e->ops[1]->result[e->o->residx]->datetime.tm_mon) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_mday != e->ops[1]->result[e->o->residx]->datetime.tm_mday);
		} else if (e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) {
			ret =
				(e->ops[0]->result[e->o->residx]->datetime.tm_hour != e->ops[1]->result[e->o->residx]->datetime.tm_hour) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_min != e->ops[1]->result[e->o->residx]->datetime.tm_min) ||
				(e->ops[0]->result[e->o->residx]->datetime.tm_sec != e->ops[1]->result[e->o->residx]->datetime.tm_sec);
		} else
			ret = false;
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_lt) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[e->o->residx] || !e->ops[1]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != e->ops[1]->result[e->o->residx]->type) {
		if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		else if (e->ops[1]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[1]->result[e->o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number) < 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[e->o->residx]->string->str, e->ops[1]->result[e->o->residx]->string->str) < 0);
		break;
	case OCRPT_RESULT_DATETIME:
		ret = false;
		if ((e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid &&
				e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) ||
			(e->ops[0]->result[e->o->residx]->interval && e->ops[1]->result[e->o->residx]->interval)) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_year < e->ops[1]->result[e->o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_mon < e->ops[1]->result[e->o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_mday < e->ops[1]->result[e->o->residx]->datetime.tm_mday)
						ret = true;
					else if (e->ops[0]->result[e->o->residx]->datetime.tm_mday == e->ops[1]->result[e->o->residx]->datetime.tm_mday) {
						if (e->ops[0]->result[e->o->residx]->datetime.tm_hour < e->ops[1]->result[e->o->residx]->datetime.tm_hour)
							ret = true;
						else if (e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) {
							if (e->ops[0]->result[e->o->residx]->datetime.tm_min < e->ops[1]->result[e->o->residx]->datetime.tm_min)
								ret = true;
							else if (e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) {
								if (e->ops[0]->result[e->o->residx]->datetime.tm_sec < e->ops[1]->result[e->o->residx]->datetime.tm_sec)
									ret = true;
							}
						}
					}
				}
			}
		} else if (e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_year < e->ops[1]->result[e->o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_mon < e->ops[1]->result[e->o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_mday < e->ops[1]->result[e->o->residx]->datetime.tm_mday)
						ret = true;
				}
			}
		} else if (e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_hour < e->ops[1]->result[e->o->residx]->datetime.tm_hour)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_min < e->ops[1]->result[e->o->residx]->datetime.tm_min)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_sec < e->ops[1]->result[e->o->residx]->datetime.tm_sec)
						ret = true;
				}
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_le) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[e->o->residx] || !e->ops[1]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != e->ops[1]->result[e->o->residx]->type) {
		if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		else if (e->ops[1]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[1]->result[e->o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number) <= 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[e->o->residx]->string->str, e->ops[1]->result[e->o->residx]->string->str) <= 0);
		break;
	case OCRPT_RESULT_DATETIME:
		ret = false;
		if ((e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid &&
				e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) ||
			(e->ops[0]->result[e->o->residx]->interval && e->ops[1]->result[e->o->residx]->interval)) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_year < e->ops[1]->result[e->o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_mon < e->ops[1]->result[e->o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_mday < e->ops[1]->result[e->o->residx]->datetime.tm_mday)
						ret = true;
					else if (e->ops[0]->result[e->o->residx]->datetime.tm_mday == e->ops[1]->result[e->o->residx]->datetime.tm_mday) {
						if (e->ops[0]->result[e->o->residx]->datetime.tm_hour < e->ops[1]->result[e->o->residx]->datetime.tm_hour)
							ret = true;
						else if (e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) {
							if (e->ops[0]->result[e->o->residx]->datetime.tm_min < e->ops[1]->result[e->o->residx]->datetime.tm_min)
								ret = true;
							else if (e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) {
								if (e->ops[0]->result[e->o->residx]->datetime.tm_sec <= e->ops[1]->result[e->o->residx]->datetime.tm_sec)
									ret = true;
							}
						}
					}
				}
			}
		} else if (e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_year < e->ops[1]->result[e->o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_mon < e->ops[1]->result[e->o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_mday <= e->ops[1]->result[e->o->residx]->datetime.tm_mday)
						ret = true;
				}
			}
		} else if (e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_hour < e->ops[1]->result[e->o->residx]->datetime.tm_hour)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_min < e->ops[1]->result[e->o->residx]->datetime.tm_min)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_sec <= e->ops[1]->result[e->o->residx]->datetime.tm_sec)
						ret = true;
				}
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_gt) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[e->o->residx] || !e->ops[1]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != e->ops[1]->result[e->o->residx]->type) {
		if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		else if (e->ops[1]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[1]->result[e->o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number) > 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[e->o->residx]->string->str, e->ops[1]->result[e->o->residx]->string->str) > 0);
		break;
	case OCRPT_RESULT_DATETIME:
		ret = false;
		if ((e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid &&
				e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) ||
			(e->ops[0]->result[e->o->residx]->interval && e->ops[1]->result[e->o->residx]->interval)) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_year > e->ops[1]->result[e->o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_mon > e->ops[1]->result[e->o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_mday > e->ops[1]->result[e->o->residx]->datetime.tm_mday)
						ret = true;
					else if (e->ops[0]->result[e->o->residx]->datetime.tm_mday == e->ops[1]->result[e->o->residx]->datetime.tm_mday) {
						if (e->ops[0]->result[e->o->residx]->datetime.tm_hour > e->ops[1]->result[e->o->residx]->datetime.tm_hour)
							ret = true;
						else if (e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) {
							if (e->ops[0]->result[e->o->residx]->datetime.tm_min > e->ops[1]->result[e->o->residx]->datetime.tm_min)
								ret = true;
							else if (e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) {
								if (e->ops[0]->result[e->o->residx]->datetime.tm_sec > e->ops[1]->result[e->o->residx]->datetime.tm_sec)
									ret = true;
							}
						}
					}
				}
			}
		} else if (e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_year > e->ops[1]->result[e->o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_mon > e->ops[1]->result[e->o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_mday > e->ops[1]->result[e->o->residx]->datetime.tm_mday)
						ret = true;
				}
			}
		} else if (e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_hour > e->ops[1]->result[e->o->residx]->datetime.tm_hour)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_min > e->ops[1]->result[e->o->residx]->datetime.tm_min)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_sec > e->ops[1]->result[e->o->residx]->datetime.tm_sec)
						ret = true;
				}
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_ge) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops != 2 || !e->ops[0]->result[e->o->residx] || !e->ops[1]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != e->ops[1]->result[e->o->residx]->type) {
		if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		else if (e->ops[1]->result[e->o->residx]->type == OCRPT_RESULT_ERROR)
			ocrpt_expr_make_error_result(e, e->ops[1]->result[e->o->residx]->string->str);
		else
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ret = (mpfr_cmp(e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number) >= 0);
		break;
	case OCRPT_RESULT_STRING:
		ret = (strcmp(e->ops[0]->result[e->o->residx]->string->str, e->ops[1]->result[e->o->residx]->string->str) >= 0);
		break;
	case OCRPT_RESULT_DATETIME:
		ret = false;
		if ((e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid &&
				e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) ||
			(e->ops[0]->result[e->o->residx]->interval && e->ops[1]->result[e->o->residx]->interval)) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_year > e->ops[1]->result[e->o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_mon > e->ops[1]->result[e->o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_mday > e->ops[1]->result[e->o->residx]->datetime.tm_mday)
						ret = true;
					else if (e->ops[0]->result[e->o->residx]->datetime.tm_mday == e->ops[1]->result[e->o->residx]->datetime.tm_mday) {
						if (e->ops[0]->result[e->o->residx]->datetime.tm_hour > e->ops[1]->result[e->o->residx]->datetime.tm_hour)
							ret = true;
						else if (e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) {
							if (e->ops[0]->result[e->o->residx]->datetime.tm_min > e->ops[1]->result[e->o->residx]->datetime.tm_min)
								ret = true;
							else if (e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) {
								if (e->ops[0]->result[e->o->residx]->datetime.tm_sec >= e->ops[1]->result[e->o->residx]->datetime.tm_sec)
									ret = true;
							}
						}
					}
				}
			}
		} else if (e->ops[0]->result[e->o->residx]->date_valid && e->ops[1]->result[e->o->residx]->date_valid) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_year > e->ops[1]->result[e->o->residx]->datetime.tm_year)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_year == e->ops[1]->result[e->o->residx]->datetime.tm_year) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_mon > e->ops[1]->result[e->o->residx]->datetime.tm_mon)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_mon == e->ops[1]->result[e->o->residx]->datetime.tm_mon) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_mday >= e->ops[1]->result[e->o->residx]->datetime.tm_mday)
						ret = true;
				}
			}
		} else if (e->ops[0]->result[e->o->residx]->time_valid && e->ops[1]->result[e->o->residx]->time_valid) {
			if (e->ops[0]->result[e->o->residx]->datetime.tm_hour > e->ops[1]->result[e->o->residx]->datetime.tm_hour)
				ret = true;
			else if (e->ops[0]->result[e->o->residx]->datetime.tm_hour == e->ops[1]->result[e->o->residx]->datetime.tm_hour) {
				if (e->ops[0]->result[e->o->residx]->datetime.tm_min > e->ops[1]->result[e->o->residx]->datetime.tm_min)
					ret = true;
				else if (e->ops[0]->result[e->o->residx]->datetime.tm_min == e->ops[1]->result[e->o->residx]->datetime.tm_min) {
					if (e->ops[0]->result[e->o->residx]->datetime.tm_sec >= e->ops[1]->result[e->o->residx]->datetime.tm_sec)
						ret = true;
				}
			}
		}
		break;
	default:
		ret = false;
		break;
	}

	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_val) {
	char *str;

	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx] || e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		mpfr_set(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
		break;
	case OCRPT_RESULT_STRING:
		str = e->ops[0]->result[e->o->residx]->string->str;
		if (!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "t"))
			str = "1";
		else if (!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "f"))
			str = "0";
		mpfr_set_str(e->result[e->o->residx]->number, str, 10, e->o->rndmode);
		break;
	default:
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_isnull) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx] && e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_set_ui(e->result[e->o->residx]->number, !e->ops[0]->result[e->o->residx] || e->ops[0]->result[e->o->residx]->isnull, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_null) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(e, e->ops[0]->result[e->o->residx]->type);
	e->result[e->o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulldt) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_DATETIME);
	memset(&e->result[e->o->residx]->datetime, 0, sizeof(e->result[e->o->residx]->datetime));
	e->result[e->o->residx]->date_valid = false;
	e->result[e->o->residx]->time_valid = false;
	e->result[e->o->residx]->interval = false;
	e->result[e->o->residx]->day_carry = 0;
	e->result[e->o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulln) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	e->result[e->o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_nulls) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);
	e->result[e->o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_iif) {
	int32_t opidx;
	long cond;
	ocrpt_string *string, *sstring;

	if (e->n_ops != 3) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx] || e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER || e->ops[0]->result[e->o->residx]->isnull) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	cond = mpfr_get_si(e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	opidx = (cond ? 1 : 2);

	if (!e->ops[opidx]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}
	if (e->ops[opidx]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[opidx]->result[e->o->residx]->string->str);
		return;
	}

	ocrpt_expr_init_result(e, e->ops[opidx]->result[e->o->residx]->type);

	if (e->ops[opidx]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	switch (e->ops[opidx]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		mpfr_set(e->result[e->o->residx]->number, e->ops[opidx]->result[e->o->residx]->number, e->o->rndmode);
		break;
	case OCRPT_RESULT_STRING:
		sstring = e->ops[opidx]->result[e->o->residx]->string;
		string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, sstring->len);
		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
		}
		e->result[e->o->residx]->string->len = 0;
		ocrpt_mem_string_append_len(e->result[e->o->residx]->string, sstring->str, sstring->len);
		break;
	case OCRPT_RESULT_DATETIME:
		e->result[e->o->residx]->datetime = e->ops[opidx]->result[e->o->residx]->datetime;
		e->result[e->o->residx]->date_valid = e->ops[opidx]->result[e->o->residx]->date_valid;
		e->result[e->o->residx]->time_valid = e->ops[opidx]->result[e->o->residx]->time_valid;
		e->result[e->o->residx]->interval = e->ops[opidx]->result[e->o->residx]->interval;
		e->result[e->o->residx]->day_carry = e->ops[opidx]->result[e->o->residx]->day_carry;
		break;
	default:
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_inc) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}

		mpfr_add_ui(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, 1, e->o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_dec) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

		if (e->ops[0]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}

		mpfr_sub_ui(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, 1, e->o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_error) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx] || e->ops[0]->result[e->o->residx]->isnull) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_STRING:
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		break;
	case OCRPT_RESULT_NUMBER:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_concat) {
	ocrpt_string *string;
	int32_t len;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx] || e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	for (len = 0, i = 0; i < e->n_ops; i++)
		len += e->ops[i]->result[e->o->residx]->string->len;

	string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, len);
	if (string) {
		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
		}

		string->len = 0;
		for (i = 0; i < e->n_ops; i++) {
			ocrpt_string *sstring = e->ops[i]->result[e->o->residx]->string;
			ocrpt_mem_string_append_len(string, sstring->str, sstring->len);
		}
	} else
		ocrpt_expr_make_error_result(e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_left) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t l, len;
	uint32_t i;

	if (e->n_ops != 2 || e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING || e->ops[1]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	l = mpfr_get_si(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[e->o->residx]->string;

	ocrpt_utf8forward(sstring->str, l, NULL, sstring->len, &len);

	string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, len);
	if (string) {
		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str, len);
	} else
		ocrpt_expr_make_error_result(e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_right) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t l, start;
	uint32_t i;

	if (e->n_ops != 2 || e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING || e->ops[1]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	l = mpfr_get_si(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[e->o->residx]->string;

	ocrpt_utf8backward(sstring->str, l, NULL, sstring->len, &start);

	string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, sstring->len - start);
	if (string) {
		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str + start, sstring->len - start);
	} else
		ocrpt_expr_make_error_result(e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_mid) {
	ocrpt_string *string;
	ocrpt_string *sstring;
	int32_t ofs, l, start, len;
	uint32_t i;

	if (e->n_ops != 3 || e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING ||
			e->ops[1]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER ||
			e->ops[2]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ofs = mpfr_get_si(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	l = mpfr_get_si(e->ops[2]->result[e->o->residx]->number, e->o->rndmode);
	if (l < 0)
		l = 0;

	sstring = e->ops[0]->result[e->o->residx]->string;

	if (ofs < 0)
		ocrpt_utf8backward(sstring->str, -ofs, NULL, sstring->len, &start);
	else if (ofs > 0)
		ocrpt_utf8forward(sstring->str, ofs - 1, NULL, sstring->len, &start);
	else
		start = 0;
	ocrpt_utf8forward(sstring->str + start, l, NULL, sstring->len - start, &len);

	string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, len);
	if (string) {
		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
		}

		string->len = 0;
		ocrpt_mem_string_append_len(string, sstring->str + start, len);
	} else
		ocrpt_expr_make_error_result(e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_random) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
	mpfr_urandomb(e->result[e->o->residx]->number, e->o->randstate);
}

OCRPT_STATIC_FUNCTION(ocrpt_factorial) {
	intmax_t n;

	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	switch (e->ops[0]->result[e->o->residx]->type) {
	case OCRPT_RESULT_NUMBER:
		if (e->ops[0]->result[e->o->residx]->isnull) {
			ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
			e->result[e->o->residx]->isnull = true;
			return;
		}

		n = mpfr_get_sj(e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
		if (n < 0LL || n > LONG_MAX) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}

		ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);
		mpfr_fac_ui(e->result[e->o->residx]->number, (unsigned long)n, e->o->rndmode);
		break;
	case OCRPT_RESULT_ERROR:
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		break;
	case OCRPT_RESULT_STRING:
	case OCRPT_RESULT_DATETIME:
	default:
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		break;
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_land) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[e->o->residx]->number, 0) != 0);
	for (i = 1; ret && i < e->n_ops; i++) {
		unsigned long ret1 = (mpfr_cmp_ui(e->ops[i]->result[e->o->residx]->number, 0) != 0);
		ret = ret && ret1;
	}
	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_lor) {
	unsigned long ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[e->o->residx]->number, 0) != 0);
	for (i = 1; !ret && i < e->n_ops; i++) {
		unsigned long ret1 = (mpfr_cmp_ui(e->ops[i]->result[e->o->residx]->number, 0) != 0);
		ret = ret || ret1;
	}
	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_lnot) {
	intmax_t ret;

	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	ret = (mpfr_cmp_ui(e->ops[0]->result[e->o->residx]->number, 0) == 0);
	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_and) {
	uintmax_t ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	for (i = 1; ret && i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
		ret &= ret1;
	}
	mpfr_set_uj(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_or) {
	uintmax_t ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	for (i = 1; !ret && i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
		ret |= ret1;
	}
	mpfr_set_uj(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_xor) {
	uintmax_t ret;
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ret = mpfr_get_uj(e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	for (i = 1; i < e->n_ops; i++) {
		uintmax_t ret1 = mpfr_get_uj(e->ops[i]->result[e->o->residx]->number, e->o->rndmode);
		ret ^= ret1;
	}
	mpfr_set_uj(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_not) {
	intmax_t ret;

	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	ret = mpfr_get_uj(e->ops[0]->result[e->o->residx]->number, 0);
	mpfr_set_ui(e->result[e->o->residx]->number, ~ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_shl) {
	uintmax_t op, shift;
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	op = mpfr_get_uj(e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	shift = mpfr_get_uj(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	mpfr_set_uj(e->result[e->o->residx]->number, op << shift, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_shr) {
	uintmax_t op, shift;
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	op = mpfr_get_uj(e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	shift = mpfr_get_uj(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	mpfr_set_uj(e->result[e->o->residx]->number, op >> shift, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_fmod) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	mpfr_fmod(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_remainder) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	mpfr_fmod(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_rint) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_rint(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_ceil) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_ceil(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_floor) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_floor(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_round) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_round(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_trunc) {
	if (e->n_ops != 1 || !e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_trunc(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number);
}

OCRPT_STATIC_FUNCTION(ocrpt_pow) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	mpfr_pow(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_lower) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[e->o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		char cc[8];

		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
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
		ocrpt_expr_make_error_result(e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_upper) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[e->o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		char cc[8];

		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
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
		ocrpt_expr_make_error_result(e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_proper) {
	ocrpt_string *string;
	ocrpt_string *sstring;

	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	sstring = e->ops[0]->result[e->o->residx]->string;
	string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, sstring->len);
	if (string) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read, bytes_total, bytes_written;
		bool first = true;
		char cc[8];

		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
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
		ocrpt_expr_make_error_result(e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_rownum) {
	if (e->n_ops > 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->n_ops == 1) {
		if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
			return;
		}

		if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}

		if (!e->q && !e->ops[0]->result[e->o->residx]->isnull) {
			char *qname = e->ops[0]->result[e->o->residx]->string->str;
			ocrpt_list *ptr;

			for (ptr = e->o->queries; ptr; ptr = ptr->next) {
				ocrpt_query *tmp = (ocrpt_query *)ptr->data;
				if (strcmp(tmp->name, qname) == 0) {
					e->q = tmp;
					break;
				}
			}
		}
	} else {
		if (e->r && e->r->query)
			e->q = e->r->query;
		if (!e->q && e->o->queries)
			e->q = (ocrpt_query *)e->o->queries->data;
	}

	if (!e->q) {
		ocrpt_expr_make_error_result(e, "rownum(): no such query");
		return;
	}

	if (!e->result[e->o->residx]) {
		e->result[e->o->residx] = e->q->rownum->result[e->o->residx];
		ocrpt_expr_set_result_owned(e, e->o->residx, false);
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_brrownum) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->br && !e->ops[0]->result[e->o->residx]->isnull) {
		char *brname = e->ops[0]->result[e->o->residx]->string->str;
		ocrpt_list *ptr;

		for (ptr = e->r ? e->r->breaks : NULL; ptr; ptr = ptr->next) {
			ocrpt_break *tmp = (ocrpt_break *)ptr->data;
			if (strcmp(tmp->name, brname) == 0) {
				e->br = tmp;
				break;
			}
		}
	}

	if (!e->br) {
		ocrpt_expr_make_error_result(e, "brrownum(): no such break");
		return;
	}

	if (!e->result[e->o->residx]) {
		assert(!e->result[e->o->residx]);
		e->result[e->o->residx] = e->br->rownum->result[e->o->residx];
		ocrpt_expr_set_result_owned(e, e->o->residx, false);
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_stodt) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING && e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_DATETIME);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_DATETIME) {
		e->result[e->o->residx]->datetime = e->ops[0]->result[e->o->residx]->datetime;
		e->result[e->o->residx]->date_valid = e->ops[0]->result[e->o->residx]->date_valid;
		e->result[e->o->residx]->time_valid = e->ops[0]->result[e->o->residx]->time_valid;
		e->result[e->o->residx]->interval = e->ops[0]->result[e->o->residx]->interval;
		e->result[e->o->residx]->day_carry = e->ops[0]->result[e->o->residx]->day_carry;
	} else if (!ocrpt_parse_datetime(e->o, e->ops[0]->result[e->o->residx]->string->str, e->ops[0]->result[e->o->residx]->string->len, e->result[e->o->residx]))
		if (!ocrpt_parse_interval(e->o, e->ops[0]->result[e->o->residx]->string->str, e->ops[0]->result[e->o->residx]->string->len, e->result[e->o->residx]))
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_dtos) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->interval) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	if (e->ops[0]->result[e->o->residx]->isnull || !e->ops[0]->result[e->o->residx]->date_valid) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	ocrpt_string *string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, 64);

	if (string) {
		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
		}

		char *dfmt = nl_langinfo_l(D_FMT, e->o->locale);
		strftime(e->result[e->o->residx]->string->str, e->result[e->o->residx]->string->allocated_len, dfmt, &e->ops[0]->result[e->o->residx]->datetime);
	} else
		ocrpt_expr_make_error_result(e, "out of memory");
}

OCRPT_STATIC_FUNCTION(ocrpt_date) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->o->current_date->date_valid) {
		time_t t = time(NULL);
		localtime_r(&t, &e->o->current_date->datetime);
		e->o->current_date->date_valid = true;
		e->o->current_date->time_valid = false;
	}

	if (!e->result[e->o->residx]) {
		e->result[e->o->residx] = e->o->current_date;
		ocrpt_expr_set_result_owned(e, e->o->residx, false);
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_now) {
	if (e->n_ops != 0) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->o->current_date->date_valid) {
		time_t t = time(NULL);
		localtime_r(&t, &e->o->current_timestamp->datetime);
		e->o->current_timestamp->date_valid = true;
		e->o->current_timestamp->time_valid = true;
	}

	if (!e->result[e->o->residx]) {
		e->result[e->o->residx] = e->o->current_timestamp;
		ocrpt_expr_set_result_owned(e, e->o->residx, false);
	}
}

OCRPT_STATIC_FUNCTION(ocrpt_year) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull || (!e->ops[0]->result[e->o->residx]->interval && !e->ops[0]->result[e->o->residx]->date_valid)) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	int add = (e->ops[0]->result[e->o->residx]->interval ? 0 : 1900);
	mpfr_set_si(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->datetime.tm_year + add, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_month) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull || (!e->ops[0]->result[e->o->residx]->interval && !e->ops[0]->result[e->o->residx]->date_valid)) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	int add = (e->ops[0]->result[e->o->residx]->interval ? 0 : 1);
	mpfr_set_si(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->datetime.tm_mon + add, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_day) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull || (!e->ops[0]->result[e->o->residx]->interval && !e->ops[0]->result[e->o->residx]->date_valid)) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_set_si(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->datetime.tm_mday, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_dim) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[e->o->residx]->interval) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull || !e->ops[0]->result[e->o->residx]->date_valid) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	int mon = e->ops[0]->result[e->o->residx]->datetime.tm_mon;
	int year = e->ops[0]->result[e->o->residx]->datetime.tm_year;
	mpfr_set_si(e->result[e->o->residx]->number, days_in_month[ocrpt_leap_year(year)][mon], e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_wiy) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[e->o->residx]->interval) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull || !e->ops[0]->result[e->o->residx]->date_valid) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	char wiy[64] = "";
	strftime(wiy, sizeof(wiy), "%U", &e->ops[0]->result[e->o->residx]->datetime);
	mpfr_set_si(e->result[e->o->residx]->number, atoi(wiy), e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_wiy1) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[e->o->residx]->interval) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull || !e->ops[0]->result[e->o->residx]->date_valid) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	char wiy[64] = "";
	strftime(wiy, sizeof(wiy), "%W", &e->ops[0]->result[e->o->residx]->datetime);
	mpfr_set_si(e->result[e->o->residx]->number, atoi(wiy), e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_wiyo) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[e->o->residx]->interval) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[1]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	if (!e->ops[0]->result[e->o->residx]->date_valid) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	ocrpt_result res = { .type = OCRPT_RESULT_DATETIME, .interval = false };
	res.datetime = e->ops[0]->result[e->o->residx]->datetime;
	res.date_valid = e->ops[0]->result[e->o->residx]->date_valid;
	res.time_valid = false;

	char wiy[64] = "";
	int wyear, wyearofs;
	long ofs = mpfr_get_si(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	while (ofs < 0)
		ofs += 7;
	ofs = ofs % 7;

	strftime(wiy, sizeof(wiy), "%U", &res.datetime);
	wyear = atoi(wiy);

	ocrpt_datetime_result_add_number(e->o, &res, e->ops[0]->result[e->o->residx], -ofs);

	res.datetime.tm_isdst = -1;
	res.datetime.tm_wday = -1;
	time_t t = mktime(&res.datetime);
	localtime_r(&t, &res.datetime);

	strftime(wiy, sizeof(wiy), "%U", &res.datetime);
	wyearofs = atoi(wiy);

	if (wyearofs > wyear)
		wyearofs = 0;

	mpfr_set_si(e->result[e->o->residx]->number, wyearofs, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_stdwiy) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[e->o->residx]->interval) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull || !e->ops[0]->result[e->o->residx]->date_valid) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	char wiy[64] = "";
	strftime(wiy, sizeof(wiy), "%V", &e->ops[0]->result[e->o->residx]->datetime);
	mpfr_set_si(e->result[e->o->residx]->number, atoi(wiy), e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_dateof) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_DATETIME);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	ocrpt_result_copy(e->result[e->o->residx], e->ops[0]->result[e->o->residx]);

	e->result[e->o->residx]->datetime.tm_hour = 0;
	e->result[e->o->residx]->datetime.tm_min = 0;
	e->result[e->o->residx]->datetime.tm_sec = 0;
	e->result[e->o->residx]->date_valid = e->ops[0]->result[e->o->residx]->date_valid;
	e->result[e->o->residx]->time_valid = false;
	e->result[e->o->residx]->interval = e->ops[0]->result[e->o->residx]->interval;

	if (!e->result[e->o->residx]->interval && !e->result[e->o->residx]->date_valid)
		e->result[e->o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_timeof) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_DATETIME);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	ocrpt_result_copy(e->result[e->o->residx], e->ops[0]->result[e->o->residx]);

	e->result[e->o->residx]->datetime.tm_year = 0;
	e->result[e->o->residx]->datetime.tm_mon = 0;
	e->result[e->o->residx]->datetime.tm_mday = 0;
	e->result[e->o->residx]->date_valid = false;
	e->result[e->o->residx]->time_valid = e->ops[0]->result[e->o->residx]->time_valid;
	e->result[e->o->residx]->interval = e->ops[0]->result[e->o->residx]->interval;

	if (!e->result[e->o->residx]->interval && !e->result[e->o->residx]->time_valid)
		e->result[e->o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_chgdateof) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[i]->result[e->o->residx]->interval) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_DATETIME);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ocrpt_result_copy(e->result[e->o->residx], e->ops[0]->result[e->o->residx]);

	e->result[e->o->residx]->datetime.tm_year = e->ops[1]->result[e->o->residx]->datetime.tm_year;
	e->result[e->o->residx]->datetime.tm_mon = e->ops[1]->result[e->o->residx]->datetime.tm_mon;
	e->result[e->o->residx]->datetime.tm_mday = e->ops[1]->result[e->o->residx]->datetime.tm_mday;
	e->result[e->o->residx]->date_valid = e->ops[1]->result[e->o->residx]->date_valid;

	if (!e->result[e->o->residx]->date_valid && !e->result[e->o->residx]->time_valid)
		e->result[e->o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_chgtimeof) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[i]->result[e->o->residx]->interval) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_DATETIME);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ocrpt_result_copy(e->result[e->o->residx], e->ops[0]->result[e->o->residx]);

	e->result[e->o->residx]->datetime.tm_hour = e->ops[1]->result[e->o->residx]->datetime.tm_hour;
	e->result[e->o->residx]->datetime.tm_min = e->ops[1]->result[e->o->residx]->datetime.tm_min;
	e->result[e->o->residx]->datetime.tm_sec = e->ops[1]->result[e->o->residx]->datetime.tm_sec;
	e->result[e->o->residx]->time_valid = e->ops[1]->result[e->o->residx]->time_valid;

	if (!e->result[e->o->residx]->date_valid && !e->result[e->o->residx]->time_valid)
		e->result[e->o->residx]->isnull = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_gettimeinsecs) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[e->o->residx]->interval) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull || !e->ops[0]->result[e->o->residx]->time_valid) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	int ret = e->ops[0]->result[e->o->residx]->datetime.tm_hour * 3600 + e->ops[0]->result[e->o->residx]->datetime.tm_min * 60 + e->ops[0]->result[e->o->residx]->datetime.tm_sec;
	mpfr_set_ui(e->result[e->o->residx]->number, ret, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_settimeinsecs) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME || e->ops[0]->result[e->o->residx]->interval ||
		e->ops[1]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_DATETIME);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	ocrpt_result_copy(e->result[e->o->residx], e->ops[0]->result[e->o->residx]);

	long ret = mpfr_get_ui(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	if (ret < 0 || ret >= 86400) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}


	e->result[e->o->residx]->datetime.tm_sec = ret % 60;
	ret /= 60;
	e->result[e->o->residx]->datetime.tm_min = ret % 60;
	ret /= 60;
	e->result[e->o->residx]->datetime.tm_hour = ret;
	e->result[e->o->residx]->time_valid = true;
}

OCRPT_STATIC_FUNCTION(ocrpt_interval) {
	uint32_t i;

	if (e->n_ops != 1 && e->n_ops != 6) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	if (e->n_ops == 6) {
		for (i = 0; i < e->n_ops; i++) {
			if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
				ocrpt_expr_make_error_result(e, "invalid operand(s)");
				return;
			}
		}
	} else {
		if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_DATETIME);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	if (e->n_ops == 6) {
		int year = mpfr_get_ui(e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
		int mon  = mpfr_get_ui(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
		int mday = mpfr_get_ui(e->ops[2]->result[e->o->residx]->number, e->o->rndmode);
		int hour = mpfr_get_ui(e->ops[3]->result[e->o->residx]->number, e->o->rndmode);
		int min  = mpfr_get_ui(e->ops[4]->result[e->o->residx]->number, e->o->rndmode);
		int sec  = mpfr_get_ui(e->ops[5]->result[e->o->residx]->number, e->o->rndmode);

		memset(&e->result[e->o->residx]->datetime, 0, sizeof(e->result[e->o->residx]->datetime));
		e->result[e->o->residx]->datetime.tm_year = year;
		e->result[e->o->residx]->datetime.tm_mon = mon;
		e->result[e->o->residx]->datetime.tm_mday = mday;
		e->result[e->o->residx]->datetime.tm_hour = hour;
		e->result[e->o->residx]->datetime.tm_min = min;
		e->result[e->o->residx]->datetime.tm_sec = sec;
		e->result[e->o->residx]->date_valid = false;
		e->result[e->o->residx]->time_valid = false;
		e->result[e->o->residx]->interval = true;
	} else if (!ocrpt_parse_interval(e->o, e->ops[0]->result[e->o->residx]->string->str, e->ops[0]->result[e->o->residx]->string->len, e->result[e->o->residx]))
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
}

OCRPT_STATIC_FUNCTION(ocrpt_cos) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_cos(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_sin) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_sin(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_tan) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_tan(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_acos) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_acos(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_asin) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_asin(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_atan) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_atan(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_sec) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_sec(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_csc) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_csc(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_cot) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_cot(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_log) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_log(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_log2) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_log2(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_log10) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_log10(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_exp) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_exp(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_exp2) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_exp2(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_exp10) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_exp10(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_sqr) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_sqr(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_sqrt) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	mpfr_sqrt(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_fxpval) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	if ((e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING && e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) || e->ops[1]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_NUMBER)
		mpfr_set(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->number, e->o->rndmode);
	else
		mpfr_set_str(e->result[e->o->residx]->number, e->ops[0]->result[e->o->residx]->string->str, 10, e->o->rndmode);

	mpfr_t tmp;

	mpfr_init2(tmp, e->o->prec);
	mpfr_exp10(tmp, e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	mpfr_div(e->result[e->o->residx]->number, e->result[e->o->residx]->number, tmp, e->o->rndmode);
	mpfr_clear(tmp);
}

OCRPT_STATIC_FUNCTION(ocrpt_str) {
	uint32_t i;

	if (e->n_ops != 3) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->isnull) {
			e->result[e->o->residx]->isnull = true;
			return;
		}
	}

	int len = mpfr_get_ui(e->ops[1]->result[e->o->residx]->number, e->o->rndmode);
	int decimal = mpfr_get_ui(e->ops[2]->result[e->o->residx]->number, e->o->rndmode);
	char fmt[16];

	sprintf(fmt, "%%%d.%dRf", len, decimal);

	len = mpfr_snprintf(NULL, 0, fmt, e->ops[0]->result[e->o->residx]->number);

	ocrpt_string *string = ocrpt_mem_string_resize(e->result[e->o->residx]->string, len + 1);
	if (string) {
		if (!e->result[e->o->residx]->string) {
			e->result[e->o->residx]->string = string;
			e->result[e->o->residx]->string_owned = true;
		}
		string->len = 0;
	} else {
		ocrpt_expr_make_error_result(e, "out of memory");
		return;
	}

	len = mpfr_snprintf(e->result[e->o->residx]->string->str, len + 1, fmt, e->ops[0]->result[e->o->residx]->number);
	e->result[e->o->residx]->string->len = len;
}

OCRPT_STATIC_FUNCTION(ocrpt_strlen) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (!e->ops[0]->result[e->o->residx]) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
		ocrpt_expr_make_error_result(e, e->ops[0]->result[e->o->residx]->string->str);
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_NUMBER);

	if (e->ops[0]->result[e->o->residx]->isnull) {
		e->result[e->o->residx]->isnull = true;
		return;
	}

	int len = 0;

	ocrpt_utf8forward(e->ops[0]->result[e->o->residx]->string->str, -1, &len, -1, NULL);
	mpfr_set_ui(e->result[e->o->residx]->number, len, e->o->rndmode);
}

OCRPT_STATIC_FUNCTION(ocrpt_format) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	if (e->ops[1]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	ocrpt_string formatstring;

	if (!e->ops[1]->result[e->o->residx]->isnull && e->ops[1]->result[e->o->residx]->string->len) {
		formatstring.str = e->ops[1]->result[e->o->residx]->string->str;
		formatstring.len = e->ops[1]->result[e->o->residx]->string->len;
	} else {
		switch (e->ops[0]->result[e->o->residx]->type) {
		case OCRPT_RESULT_NUMBER:
			formatstring.str = "%d";
			formatstring.len = 2;
			break;
		case OCRPT_RESULT_STRING:
			formatstring.str = "%s";
			formatstring.len = 2;
			break;
		case OCRPT_RESULT_DATETIME:
			/*
			 * Result would be garbage for intervals.
			 * Let's return an empty string.
			 */
			if (e->ops[0]->result[e->o->residx]->interval) {
				e->result[e->o->residx]->string->str[0] = 0;
				e->result[e->o->residx]->string->len = 0;
				return;
			}

			formatstring.str = nl_langinfo_l(D_FMT, e->o->locale);
			formatstring.len = strlen(formatstring.str);
			break;
		case OCRPT_RESULT_ERROR:
			/* This case is caught earlier */
			return;
		}
	}

	ocrpt_format_string(e->o, e, e->result[e->o->residx]->string, &formatstring, e->ops, 1);
}

OCRPT_STATIC_FUNCTION(ocrpt_dtosf) {
	uint32_t i;

	if (e->n_ops != 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_DATETIME) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[1]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	/*
	 * Result would be garbage for intervals.
	 * Let's return an empty string.
	 */
	if (e->ops[0]->result[e->o->residx]->interval) {
		e->result[e->o->residx]->string->str[0] = 0;
		e->result[e->o->residx]->string->len = 0;
		return;
	}

	ocrpt_string formatstring;

	if (!e->ops[1]->result[e->o->residx]->isnull && e->ops[1]->result[e->o->residx]->string->len) {
		formatstring.str = e->ops[1]->result[e->o->residx]->string->str;
		formatstring.len = e->ops[1]->result[e->o->residx]->string->len;
	} else {
		formatstring.str = nl_langinfo_l(D_FMT, e->o->locale);
		formatstring.len = strlen(formatstring.str);
	}

	ocrpt_format_string(e->o, e, e->result[e->o->residx]->string, &formatstring, e->ops, 1);
}

OCRPT_STATIC_FUNCTION(ocrpt_printf) {
	uint32_t i;

	if (e->n_ops < 2) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx]) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	ocrpt_format_string(e->o, e, e->result[e->o->residx]->string, e->ops[0]->result[e->o->residx]->string, &e->ops[1], e->n_ops - 1);
}

OCRPT_STATIC_FUNCTION(ocrpt_xlate) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	if (e->ops[0]->result[e->o->residx]->isnull) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	e->result[e->o->residx]->string->len = 0;

	if (e->o->textdomain) {
		locale_t locale = uselocale(e->o->locale);
		ocrpt_mem_string_append(e->result[e->o->residx]->string, dgettext(e->o->textdomain, e->ops[0]->result[e->o->residx]->string->str));
		uselocale(locale);
	} else
		ocrpt_mem_string_append_len(e->result[e->o->residx]->string, e->ops[0]->result[e->o->residx]->string->str, e->ops[0]->result[e->o->residx]->string->len);
}

OCRPT_STATIC_FUNCTION(ocrpt_xlate2) {
	int32_t i;

	if (e->n_ops != 3) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	for (i = 0; i < e->n_ops; i++) {
		if (!e->ops[i]->result[e->o->residx] || e->ops[i]->result[e->o->residx]->isnull) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	for (i = 0; i < e->n_ops; i++) {
		if (e->ops[i]->result[e->o->residx]->type == OCRPT_RESULT_ERROR) {
			ocrpt_expr_make_error_result(e, e->ops[i]->result[e->o->residx]->string->str);
			return;
		}
	}

	for (i = 0; i < 2; i++) {
		if (e->ops[i]->result[e->o->residx]->type != OCRPT_RESULT_STRING) {
			ocrpt_expr_make_error_result(e, "invalid operand(s)");
			return;
		}
	}

	if (e->ops[2]->result[e->o->residx]->type != OCRPT_RESULT_NUMBER) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	ocrpt_expr_init_result(e, OCRPT_RESULT_STRING);

	e->result[e->o->residx]->string->len = 0;

	if (e->o->textdomain) {
		locale_t locale = uselocale(e->o->locale);
		ocrpt_mem_string_append(e->result[e->o->residx]->string,
								dngettext(e->o->textdomain,
										e->ops[0]->result[e->o->residx]->string->str,
										e->ops[1]->result[e->o->residx]->string->str,
										mpfr_get_si(e->ops[2]->result[e->o->residx]->number, e->o->rndmode)));
		uselocale(locale);
	} else
		ocrpt_mem_string_append_len(e->result[e->o->residx]->string, e->ops[0]->result[e->o->residx]->string->str, e->ops[0]->result[e->o->residx]->string->len);
}

OCRPT_STATIC_FUNCTION(ocrpt_prevval) {
	if (e->n_ops != 1) {
		ocrpt_expr_make_error_result(e, "invalid operand(s)");
		return;
	}

	int previdx = ocrpt_expr_prev_residx(e->o->residx);

	if (e->ops[0]->result[previdx]) {
		ocrpt_expr_init_result(e, e->ops[0]->result[previdx]->type);
		ocrpt_result_copy(e->result[e->o->residx], e->ops[0]->result[previdx]);
	} else
		ocrpt_expr_make_error_result(e, "Subexpression has no previous result");
}

/*
 * Keep this sorted by function name because it is
 * used via bsearch()
 */
static const ocrpt_function ocrpt_functions[] = {
	{ "abs",		ocrpt_abs,	NULL,	1,	false,	false,	false,	false },
	{ "acos",		ocrpt_acos,	NULL,	1,	false,	false,	false,	false },
	{ "add",		ocrpt_add,	NULL,	-1,	true,	true,	false,	false },
	{ "and",		ocrpt_and,	NULL,	-1,	true,	true,	false,	false },
	{ "asin",		ocrpt_asin,	NULL,	1,	false,	false,	false,	false },
	{ "atan",		ocrpt_atan,	NULL,	1,	false,	false,	false,	false },
	{ "brrownum",	ocrpt_brrownum,	NULL,	1,	false,	false,	false,	true },
	{ "ceil",		ocrpt_ceil,	NULL,	1,	false,	false,	false,	false },
	{ "chgdateof",	ocrpt_chgdateof,	NULL,	2,	false,	false,	false,	false },
	{ "chgtimeof",	ocrpt_chgtimeof,	NULL,	2,	false,	false,	false,	false },
	{ "concat",		ocrpt_concat,	NULL,	-1,	false,	false,	false,	false },
	{ "cos",		ocrpt_cos,	NULL,	1,	false,	false,	false,	false },
	{ "cot",		ocrpt_cot,	NULL,	1,	false,	false,	false,	false },
	{ "csc",		ocrpt_csc,	NULL,	1,	false,	false,	false,	false },
	{ "date",		ocrpt_date,	NULL,	0,	false,	false,	false,	false },
	{ "dateof",		ocrpt_dateof,	NULL,	1,	false,	false,	false,	false },
	{ "day",		ocrpt_day,	NULL,	1,	false,	false,	false,	false },
	{ "dec",		ocrpt_dec,	NULL,	1,	false,	false,	false,	false },
	{ "dim",		ocrpt_dim,	NULL,	1,	false,	false,	false,	false },
	{ "div",		ocrpt_div,	NULL,	-1,	false,	false,	true,	false },
	{ "dtos",		ocrpt_dtos,	NULL,	1,	false,	false,	false,	false },
	{ "dtosf",		ocrpt_dtosf,	NULL,	2,	false,	false,	false,	false },
	{ "eq",			ocrpt_eq,	NULL,	2,	true,	false,	false,	false },
	{ "error",		ocrpt_error,	NULL,	1,	false,	false,	false,	false },
	{ "exp",		ocrpt_exp,	NULL,	1,	false,	false,	false,	false },
	{ "exp10",		ocrpt_exp10,	NULL,	1,	false,	false,	false,	false },
	{ "exp2",		ocrpt_exp2,	NULL,	1,	false,	false,	false,	false },
	{ "factorial",	ocrpt_factorial,	NULL,	1,	false,	false,	false,	false },
	{ "floor",		ocrpt_floor,	NULL,	1,	false,	false,	false,	false },
	{ "fmod",		ocrpt_fmod,	NULL,	2,	false,	false,	false,	false },
	{ "format" ,	ocrpt_format,	NULL,	2,	false,	false,	false,	false },
	{ "fxpval",		ocrpt_fxpval,	NULL,	2,	false,	false,	false,	false },
	{ "ge",			ocrpt_ge,	NULL,	2,	false,	false,	false,	false },
	{ "gettimeinsecs",	ocrpt_gettimeinsecs,	NULL,	1,	false,	false,	false,	false },
	{ "gt",			ocrpt_gt,	NULL,	2,	false,	false,	false,	false },
	{ "iif",		ocrpt_iif,	NULL,	3,	false,	false,	false,	false },
	{ "inc",		ocrpt_inc,	NULL,	1,	false,	false,	false,	false },
	{ "interval",	ocrpt_interval,	NULL,	-1,	false,	false,	false,	false },
	{ "isnull",		ocrpt_isnull,	NULL,	1,	false,	false,	false,	false },
	{ "land",		ocrpt_land,	NULL,	-1,	true,	true,	false,	false },
	{ "le",			ocrpt_le,	NULL,	2,	false,	false,	false,	false },
	{ "left",		ocrpt_left,	NULL,	2,	false,	false,	false,	false },
	{ "ln",			ocrpt_log,	NULL,	1,	false,	false,	false,	false },
	{ "lnot",		ocrpt_lnot,	NULL,	1,	false,	false,	false,	false },
	{ "log",		ocrpt_log,	NULL,	1,	false,	false,	false,	false },
	{ "log10",		ocrpt_log10,	NULL,	1,	false,	false,	false,	false },
	{ "log2",		ocrpt_log2,	NULL,	1,	false,	false,	false,	false },
	{ "lor",		ocrpt_lor,	NULL,	-1,	true,	true,	false,	false },
	{ "lower",		ocrpt_lower,	NULL,	1,	false,	false,	false,	false },
	{ "lt",			ocrpt_lt,	NULL,	2,	false,	false,	false,	false },
	{ "mid",		ocrpt_mid,	NULL,	3,	false,	false,	false,	false },
	{ "mod",		ocrpt_remainder,	NULL,	2,	false,	false,	false,	false },
	{ "month",		ocrpt_month,	NULL,	1,	false,	false,	false,	false },
	{ "mul",		ocrpt_mul,	NULL,	-1,	true,	true,	false,	false },
	{ "ne",			ocrpt_ne,	NULL,	2,	true,	false,	false,	false },
	{ "not",		ocrpt_not,	NULL,	1,	false,	false,	false,	false },
	{ "now",		ocrpt_now,	NULL,	0,	false,	false,	false,	false },
	{ "null",		ocrpt_null,	NULL,	1,	false,	false,	false,	false },
	{ "nulldt",		ocrpt_nulldt,	NULL,	0,	false,	false,	false,	false },
	{ "nulln",		ocrpt_nulln,	NULL,	0,	false,	false,	false,	false },
	{ "nulls",		ocrpt_nulls,	NULL,	0,	false,	false,	false,	false },
	{ "or",			ocrpt_or,	NULL,	-1,	true,	true,	false,	false },
	{ "pow",		ocrpt_pow,	NULL,	2,	false,	false,	false,	false },
	{ "prevval",	ocrpt_prevval,	NULL,	1,	false, false, false, false },
	{ "printf",		ocrpt_printf,	NULL,	-1,	false,	false,	false,	false },
	{ "proper",		ocrpt_proper,	NULL,	1,	false,	false,	false,	false },
	{ "random",		ocrpt_random,	NULL,	0,	false,	false,	false,	true },
	{ "remainder",	ocrpt_remainder,	NULL,	2,	false,	false,	false,	false },
	{ "right",		ocrpt_right,	NULL,	2,	false,	false,	false,	false },
	{ "rint",		ocrpt_rint,	NULL,	1,	false,	false,	false,	false },
	{ "round",		ocrpt_round,	NULL,	1,	false,	false,	false,	false },
	{ "rownum",		ocrpt_rownum,	NULL,	-1,	false,	false,	false,	true },
	{ "sec",		ocrpt_sec,	NULL,	1,	false,	false,	false,	false },
	{ "settimeinsecs",	ocrpt_settimeinsecs,	NULL,	2,	false,	false,	false,	false },
	{ "shl",		ocrpt_shl,	NULL,	2,	false,	false,	false,	false },
	{ "shr",		ocrpt_shr,	NULL,	2,	false,	false,	false,	false },
	{ "sin",		ocrpt_sin,	NULL,	1,	false,	false,	false,	false },
	{ "sqr",		ocrpt_sqr,	NULL,	1,	false,	false,	false,	false },
	{ "sqrt",		ocrpt_sqrt,	NULL,	1,	false,	false,	false,	false },
	{ "stdwiy",		ocrpt_stdwiy,	NULL,	1,	false,	false,	false,	false },
	{ "stod",		ocrpt_stodt,	NULL,	1,	false,	false,	false,	false },
	{ "stodt",		ocrpt_stodt,	NULL,	1,	false,	false,	false,	false },
	{ "stodtsql",	ocrpt_stodt,	NULL,	1,	false,	false,	false,	false },
	{ "str",		ocrpt_str,	NULL,		3,	false,	false,	false,	false },
	{ "strlen",		ocrpt_strlen,	NULL,	1,	false,	false,	false,	false },
	{ "sub",		ocrpt_sub,	NULL,	-1,	false,	false,	false,	false },
	{ "tan",		ocrpt_tan,	NULL,	1,	false,	false,	false,	false },
	{ "timeof",		ocrpt_timeof,	NULL,	1,	false,	false,	false,	false },
	{ "translate",	ocrpt_xlate,	NULL,	1,	false,	false,	false,	false },
	{ "translate2",	ocrpt_xlate2,	NULL,	1,	false,	false,	false,	false },
	{ "trunc",		ocrpt_trunc,	NULL,	1,	false,	false,	false,	false },
	{ "tstod",		ocrpt_stodt,	NULL,	1,	false,	false,	false,	false },
	{ "uminus",		ocrpt_uminus,	NULL,	1,	false,	false,	false,	false },
	{ "upper",		ocrpt_upper,	NULL,	1,	false,	false,	false,	false },
	{ "val",		ocrpt_val,	NULL,	1,	false,	false,	false,	false },
	{ "wiy",		ocrpt_wiy,	NULL,	1,	false,	false,	false,	false },
	{ "wiy1",		ocrpt_wiy1,	NULL,	1,	false,	false,	false,	false },
	{ "wiyo",		ocrpt_wiyo,	NULL,	2,	false,	false,	false,	false },
	{ "xor",		ocrpt_xor,	NULL,	-1,	true,	true,	true,	false },
	{ "year",		ocrpt_year,	NULL,	1,	false,	false,	false,	false },
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

DLL_EXPORT_SYM bool ocrpt_function_add(opencreport *o, const char *fname,
										ocrpt_function_call func, void *user_data,
										int32_t n_ops, bool commutative, bool associative,
										bool left_associative, bool dont_optimize) {
	if (!o || !fname || !*fname || !func)
		return false;

	ocrpt_function *new_func;
	ocrpt_function **f_array;

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
	new_func->user_data = user_data;
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
	if (!o || !fname)
		return NULL;

	if (o->functions) {
		ocrpt_function **ret = bsearch(fname, o->functions, o->n_functions, sizeof(ocrpt_function *), funccmpind);
		if (ret)
			return *ret;
	}

	return bsearch(fname, ocrpt_functions, n_ocrpt_functions, sizeof(ocrpt_function), funccmp);
}
