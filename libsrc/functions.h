/*
 * OpenCReports main module
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORTS_FUNCTIONS_H_
#define _OPENCREPORTS_FUNCTIONS_H_

#include <stdbool.h>

#include "exprutil.h"
#include "opencreport-private.h"

extern ocrpt_function *ocrpt_find_function(const char *fname);

static inline bool ocrpt_init_func_result(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type) {
	ocrpt_result *result = e->result[o->residx];

	if (!result) {
		result = ocrpt_mem_malloc(sizeof(ocrpt_result));
		if (result) {
			memset(result, 0, sizeof(ocrpt_result));
			e->result[o->residx] = result;
			e->result_owned[o->residx] = true;
		}
	}
	if (result) {
		if (type == OCRPT_RESULT_NUMBER && !result->number_initialized) {
			mpfr_init2(result->number, o->prec);
			result->number_initialized = true;
		}
		result->type = type;
	}

	return !!result;
}

#endif
