/*
 * Variable utilities
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

/*
 * Create a named report variable
 *
 * Possible types are: count, expression, sum, average, lowest, highest
 * All types (except count) need a base expression
 */
DLL_EXPORT_SYM ocrpt_var *ocrpt_variable_new(opencreport *o, ocrpt_report *r, ocrpt_var_type type, const char *name, ocrpt_expr *e) {
	ocrpt_var *var;

	if (!o || !name)
		return NULL;

	if (type > OCRPT_VARIABLE_HIGHEST)
		return NULL;

	if (type == OCRPT_VARIABLE_COUNT) {
		if (e)
			return NULL;

		e = ocrpt_expr_parse(o, "1", NULL);
		if (!e)
			return NULL;

		type = OCRPT_VARIABLE_SUM;
	} else {
		if (e) {
			uint32_t vartypes;
			if (ocrpt_expr_references(o, r, e, OCRPT_VARREF_VVAR, &vartypes)) {
				switch (type) {
				case OCRPT_VARIABLE_EXPRESSION:
					return NULL;
				case OCRPT_VARIABLE_COUNT:
				case OCRPT_VARIABLE_SUM:
				case OCRPT_VARIABLE_AVERAGE:
				case OCRPT_VARIABLE_LOWEST:
				case OCRPT_VARIABLE_HIGHEST:
					if ((vartypes & ~OCRPT_VARIABLE_EXPRESSION_BIT) != 0)
						return NULL;
					break;
				}
				return NULL;
			}
		} else
			return NULL;
	}

	var = ocrpt_mem_malloc(sizeof(ocrpt_var));
	if (!var)
		return NULL;

	memset(var, 0, sizeof(ocrpt_var));
	var->name = ocrpt_mem_strdup(name);

	switch (type) {
	case OCRPT_VARIABLE_EXPRESSION:
		var->expr = e;
		break;
	case OCRPT_VARIABLE_SUM:
		break;
	case OCRPT_VARIABLE_AVERAGE:
		break;
	case OCRPT_VARIABLE_LOWEST:
		break;
	case OCRPT_VARIABLE_HIGHEST:
		break;
	default:
		/* Can't happen */
		break;
	}

	r->variables = ocrpt_list_append(r->variables, var);

	return var;
}

DLL_EXPORT_SYM void ocrpt_variable_free(opencreport *o, ocrpt_report *r, ocrpt_var *var) {
	if (var->reset_on_br) {
		ocrpt_break *br;

		if (var->br_resolved)
			br = var->br;
		else
			br = ocrpt_break_get(o, r, var->br_name);

		if (br)
			ocrpt_list_remove(br->reset_vars, var);
	}

	ocrpt_expr_free(var->expr);
	ocrpt_mem_free(var->name);
	ocrpt_mem_free(var);
}
