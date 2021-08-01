/*
 * Break utilities
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport.h"
#include "exprutil.h"
#include "datasource.h"

#define OCRPT_BREAK_EXPR(o, brname, expr) \
	do { \
		if (expr) { \
			ocrpt_expr_optimize(o, expr); \
			if (ocrpt_expr_is_const(expr)) { \
				if (ocrpt_expr_is_dconst(expr)) { \
					long tmp = mpfr_get_si(expr->ops[0]->result[o->residx]->number, o->rndmode); \
					brname->expr = !!tmp; \
					ocrpt_expr_free(expr); \
					expr = NULL; \
				} else { \
					error = true; \
					fprintf(stderr, "%s: invalid expression for " #expr "\n", __func__); \
				} \
			} else \
				brname->e_##expr = expr; \
		} \
	} while (0)

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_new(opencreport *o, ocrpt_part *part,
											const char *name,
											ocrpt_expr *newpage,
											ocrpt_expr *headernewpage,
											ocrpt_expr *suppressblank) {
	ocrpt_break *br;
	bool error = false;

	if (!o || !part) {
		ocrpt_expr_free(newpage);
		ocrpt_expr_free(headernewpage);
		ocrpt_expr_free(suppressblank);
		return NULL;
	}

	br = ocrpt_mem_malloc(sizeof(ocrpt_break));

	if (!br) {
		ocrpt_expr_free(newpage);
		ocrpt_expr_free(headernewpage);
		ocrpt_expr_free(suppressblank);
		return NULL;
	}

	memset(br, 0, sizeof(ocrpt_break));

	OCRPT_BREAK_EXPR(o, br, newpage);
	OCRPT_BREAK_EXPR(o, br, headernewpage);
	OCRPT_BREAK_EXPR(o, br, suppressblank);

	if (error) {
		ocrpt_expr_free(newpage);
		ocrpt_expr_free(headernewpage);
		ocrpt_expr_free(suppressblank);
		ocrpt_mem_free(br);
		return NULL;
	}

	//TODO: Append the new break to the list of breaks
	//o->breaks = ocrpt_list_append()

	return br;
}

DLL_EXPORT_SYM void ocrpt_break_free(opencreport *o, ocrpt_break *br) {
	ocrpt_expr_free(br->e_newpage);
	ocrpt_expr_free(br->e_headernewpage);
	ocrpt_expr_free(br->e_suppressblank);
	ocrpt_list_free_deep(br->breakfields, (ocrpt_mem_free_t)ocrpt_expr_free);
	ocrpt_mem_free(br);
}

DLL_EXPORT_SYM ocrpt_break *ocrpt_break_get(opencreport *o, ocrpt_part *part, const char *name) {
	return NULL;
}

DLL_EXPORT_SYM bool ocrpt_break_add_breakfield(opencreport *o, ocrpt_part *part, ocrpt_break *b, ocrpt_expr *bf) {
	return false;
}

DLL_EXPORT_SYM bool ocrpt_break_check_fields(opencreport *o, ocrpt_break *br) {
	return false;
}
