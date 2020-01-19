/*
 * Expression tree utilities
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport-private.h"
#include "exprutil.h"
#include "datasource.h"

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_new(opencreport *o, const char *name,
											ocrpt_expr *newpage,
											ocrpt_expr *headernewpage,
											ocrpt_expr *suppressblank) {
	return NULL;
}

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_get(opencreport *o, const char *name) {
	return NULL;
}

DLL_EXPORT_SYM bool ocrpt_break_add_breakfield(opencreport *o, ocrpt_break *b, ocrpt_expr *bf) {
	return false;
}
