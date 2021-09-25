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
 * Takes ownership of the expression pointer.
 *
 * Possible types are: expression, count, countall, sum, average, lowest, highest
 * All types (except count and countall) need a base expression.
 *
 * Returns ocrpt_var pointer or NULL in case of an error
 * When returning NULL, the expression is freed so the caller
 * doesn't have to distinguish between different types of error.
 */
DLL_EXPORT_SYM ocrpt_var *ocrpt_variable_new(opencreport *o, ocrpt_report *r, ocrpt_var_type type, const char *name, ocrpt_expr *e) {
	ocrpt_list *ptr;
	ocrpt_var *var;
	uint32_t vartypes;

	if (!o) {
		fprintf(stderr, "invalid variable definitition: valid opencreport pointer expected\n");
		return NULL;
	}
	if (!r) {
		fprintf(stderr, "invalid variable definitition: valid ocrpt_report pointer expected\n");
		return NULL;
	}
	if (!name) {
		fprintf(stderr, "invalid variable definitition: valid name expected\n");
		return NULL;
	}

	for (ptr = r->variables; ptr; ptr = ptr->next) {
		var = (ocrpt_var *)ptr->data;
		if (strcmp(var->name, name) == 0) {
			fprintf(stderr, "variable '%s': duplicate variable name\n", name);
			ocrpt_expr_free(e);
			return NULL;
		}
	}

	switch (type) {
	case OCRPT_VARIABLE_EXPRESSION:
	case OCRPT_VARIABLE_COUNT:
	case OCRPT_VARIABLE_COUNTALL:
	case OCRPT_VARIABLE_SUM:
	case OCRPT_VARIABLE_AVERAGE:
	case OCRPT_VARIABLE_LOWEST:
	case OCRPT_VARIABLE_HIGHEST:
		if (type == OCRPT_VARIABLE_COUNT || type == OCRPT_VARIABLE_COUNTALL) {
			/*
			 * If no expression is passed, the constant expression 1 will be used.
			 * For "count", if an expression results in isnull()==true then the
			 * line is not counted, similarly to SQL's "count(expr)".
			 * For "countall", all lines (even NULL) is counted, similarly
			 * to SQL's "count(*)"
			 */
			if (!e)
				e = ocrpt_expr_parse(o, "1", NULL);
			/* Out of memory */
			if (!e) {
				fprintf(stderr, "variable '%s': cannot parse constant expression 1\n", name);
				return NULL;
			}
		} else if (!e) {
			fprintf(stderr, "variable '%s': expression is NULL\n", name);
			return NULL;
		}
		if (ocrpt_expr_references(o, r, e, OCRPT_VARREF_VVAR, &vartypes)) {
			if ((vartypes & OCRPT_VARIABLE_UNKNOWN_BIT)) {
				fprintf(stderr, "variable '%s': references an unknown variable name\n", name);
				ocrpt_expr_free(e);
				return NULL;
			}
			if ((vartypes & ~OCRPT_VARIABLE_EXPRESSION_BIT) != 0) {
				fprintf(stderr, "variable '%s': may only reference expression variables\n", name);
				ocrpt_expr_free(e);
				return NULL;
			}
		}
		break;
	default:
		fprintf(stderr, "invalid type for variable '%s': %d\n", name, type);
		ocrpt_expr_free(e);
		return NULL;
	}

	var = ocrpt_mem_malloc(sizeof(ocrpt_var));
	if (!var) {
		fprintf(stderr, "variable '%s': out of memory\n", name);
		ocrpt_expr_free(e);
		return NULL;
	}

	memset(var, 0, sizeof(ocrpt_var));
	var->name = ocrpt_mem_strdup(name);
	var->type = type;
	var->expr = e;

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

DLL_EXPORT_SYM void ocrpt_variables_free(opencreport *o, ocrpt_report *r) {
	ocrpt_list *ptr;
	bool good = ocrpt_report_validate(o, r);

	for (ptr = good ? r->variables : NULL; ptr; ptr = ptr->next)
		ocrpt_variable_free(o, r, (ocrpt_var *)ptr->data);

	if (good) {
		ocrpt_list_free(r->variables);
		r->variables = NULL;
	}
}

DLL_EXPORT_SYM void ocrpt_variable_reset_on_break(ocrpt_var *var, const char *brname) {
	var->br_resolved = false;
	var->reset_on_br = true;
	ocrpt_mem_free(var->br_name);
	var->br_name = brname ? ocrpt_mem_strdup(brname) : NULL;
}

DLL_EXPORT_SYM void ocrpt_variable_set_precalculate(ocrpt_var *var, bool value) {
	var->precalculate = value;
}
