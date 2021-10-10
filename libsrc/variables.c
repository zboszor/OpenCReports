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
#include "variables.h"

static inline void ocrpt_variable_initialize_results(opencreport *o, ocrpt_expr *e) {
	ocrpt_expr_set_iterative_start_value(e, false);
	ocrpt_expr_init_both_results(o, e, OCRPT_RESULT_NUMBER);
	if (ocrpt_expr_get_result_owned(o, e, false))
		mpfr_set_ui(e->result[0]->number, 0, o->rndmode);
	if (ocrpt_expr_get_result_owned(o, e, true))
		mpfr_set_ui(e->result[1]->number, 0, o->rndmode);
}

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
DLL_EXPORT_SYM ocrpt_var *ocrpt_variable_new(opencreport *o, ocrpt_report *r, ocrpt_var_type type, const char *name, ocrpt_expr *e, const char *reset_on_break_name) {
	ocrpt_list *ptr;
	ocrpt_var *var;
	ocrpt_string *exprstr;
	uint32_t vartypes;

	if (!o) {
		fprintf(stderr, "invalid variable definitition: valid opencreport pointer expected\n");
		ocrpt_expr_free(o, r, e);
		return NULL;
	}
	if (!r) {
		fprintf(stderr, "invalid variable definitition: valid ocrpt_report pointer expected\n");
		ocrpt_expr_free(o, r, e);
		return NULL;
	}
	if (!name) {
		fprintf(stderr, "invalid variable definitition: valid name expected\n");
		ocrpt_expr_free(o, r, e);
		return NULL;
	}

	if (!ocrpt_report_validate(o, r)) {
		ocrpt_expr_free(o, r, e);
		return NULL;
	}

	for (ptr = r->variables; ptr; ptr = ptr->next) {
		var = (ocrpt_var *)ptr->data;
		if (strcmp(var->name, name) == 0) {
			fprintf(stderr, "variable '%s': duplicate variable name\n", name);
			ocrpt_expr_free(o, r, e);
			return NULL;
		}
	}

	switch (type) {
	case OCRPT_VARIABLE_EXPRESSION:
	case OCRPT_VARIABLE_COUNT:
	case OCRPT_VARIABLE_COUNTALL:
	case OCRPT_VARIABLE_SUM:
	case OCRPT_VARIABLE_AVERAGE:
	case OCRPT_VARIABLE_AVERAGEALL:
	case OCRPT_VARIABLE_LOWEST:
	case OCRPT_VARIABLE_HIGHEST:
		if (type != OCRPT_VARIABLE_COUNT && type != OCRPT_VARIABLE_COUNTALL && !e) {
			fprintf(stderr, "variable '%s': expression is NULL\n", name);
			ocrpt_expr_free(o, r, e);
			return NULL;
		}
		if (e && ocrpt_expr_references(o, r, e, OCRPT_VARREF_VVAR, &vartypes)) {
			if ((vartypes & OCRPT_VARIABLE_UNKNOWN_BIT)) {
				fprintf(stderr, "variable '%s': references an unknown variable name\n", name);
				ocrpt_expr_free(o, r, e);
				return NULL;
			}
			if ((vartypes & ~OCRPT_VARIABLE_EXPRESSION_BIT) != 0) {
				fprintf(stderr, "variable '%s': may only reference expression variables\n", name);
				ocrpt_expr_free(o, r, e);
				return NULL;
			}
		}
		break;
	default:
		fprintf(stderr, "invalid type for variable '%s': %d\n", name, type);
		ocrpt_expr_free(o, r, e);
		return NULL;
	}

	var = ocrpt_mem_malloc(sizeof(ocrpt_var));
	if (!var) {
		fprintf(stderr, "variable '%s': out of memory\n", name);
		ocrpt_expr_free(o, r, e);
		return NULL;
	}

	memset(var, 0, sizeof(ocrpt_var));
	var->name = ocrpt_mem_strdup(name);
	var->type = type;

	if (reset_on_break_name)
		var->br_name = ocrpt_mem_strdup(reset_on_break_name);

	r->dont_add_exprs = true;

	switch (type) {
	case OCRPT_VARIABLE_EXPRESSION:
		var->resultexpr = e;
		break;

	case OCRPT_VARIABLE_COUNT:
		if (!e)
			e = ocrpt_expr_parse(o, r, "1", NULL);

		var->baseexpr = e;

		var->resultexpr = ocrpt_expr_parse(o, r, "r.self + (isnull(r.baseexpr) ? 0 : 1)", NULL);
		ocrpt_variable_initialize_results(o, var->resultexpr);
		break;

	case OCRPT_VARIABLE_COUNTALL:
		if (e)
			ocrpt_expr_free(o, r, e);

		var->resultexpr = ocrpt_expr_parse(o, r, "r.self + 1", NULL);
		ocrpt_variable_initialize_results(o, var->resultexpr);
		break;

	case OCRPT_VARIABLE_SUM:
		var->baseexpr = e;

		exprstr = ocrpt_mem_string_new_printf("(%s%s%s == 1 ? 0 : r.self) + (isnull(r.baseexpr) ? 0 : r.baseexpr)",
						reset_on_break_name ? "brrownum('" : "rownum(",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : ")");
		var->resultexpr = ocrpt_expr_parse(o, r, exprstr->str, NULL);
		ocrpt_mem_string_free(exprstr, true);
		ocrpt_variable_initialize_results(o, var->resultexpr);
		break;

	case OCRPT_VARIABLE_AVERAGE:
		var->baseexpr = e;

		exprstr = ocrpt_mem_string_new_printf("(%s%s%s == 1 ? 0 : r.self) + (isnull(r.baseexpr) ? 0 : r.baseexpr)",
						reset_on_break_name ? "brrownum('" : "rownum(",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : ")");
		var->intermedexpr = ocrpt_expr_parse(o, r, exprstr->str, NULL);
		ocrpt_mem_string_free(exprstr, true);
		ocrpt_variable_initialize_results(o, var->intermedexpr);

		var->intermed2expr = ocrpt_expr_parse(o, r, "r.self + (isnull(r.baseexpr) ? 0 : 1)", NULL);
		ocrpt_variable_initialize_results(o, var->intermed2expr);

		var->resultexpr = ocrpt_expr_parse(o, r, "r.intermedexpr / r.intermed2expr", NULL);
		ocrpt_variable_initialize_results(o, var->resultexpr);
		break;

	case OCRPT_VARIABLE_AVERAGEALL:
		var->baseexpr = e;

		exprstr = ocrpt_mem_string_new_printf("(%s%s%s == 1 ? 0 : r.self) + (isnull(r.baseexpr) ? 0 : r.baseexpr)",
						reset_on_break_name ? "brrownum('" : "rownum(",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : ")");
		var->intermedexpr = ocrpt_expr_parse(o, r, exprstr->str, NULL);
		ocrpt_mem_string_free(exprstr, true);
		ocrpt_variable_initialize_results(o, var->intermedexpr);

		exprstr = ocrpt_mem_string_new_printf("r.intermedexpr / %s%s%s",
						reset_on_break_name ? "brrownum('" : "rownum(",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : ")");
		var->resultexpr = ocrpt_expr_parse(o, r, exprstr->str, NULL);
		ocrpt_mem_string_free(exprstr, true);
		ocrpt_variable_initialize_results(o, var->resultexpr);
		break;

	case OCRPT_VARIABLE_LOWEST:
		var->baseexpr = e;

		exprstr = ocrpt_mem_string_new_printf(
						"%s%s%s == 1 ? "
							"(r.baseexpr) : "
							"(isnull(r.self) ? "
								"(r.baseexpr) : "
								"(isnull(r.baseexpr) ? "
									"r.self : "
									"(r.self < (r.baseexpr) ? r.self : (r.baseexpr))"
								")"
							")",
						reset_on_break_name ? "brrownum('" : "rownum(",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : ")");
		var->resultexpr = ocrpt_expr_parse(o, r, exprstr->str, NULL);
		ocrpt_mem_string_free(exprstr, true);
		ocrpt_variable_initialize_results(o, var->resultexpr);
		break;
	case OCRPT_VARIABLE_HIGHEST:
		var->baseexpr = e;

		exprstr = ocrpt_mem_string_new_printf(
						"%s%s%s == 1 ? "
							"(r.baseexpr) : "
							"(isnull(r.self) ? "
								"(r.baseexpr) : "
								"(isnull(r.baseexpr) ? "
									"r.self : "
									"(r.self > (r.baseexpr) ? r.self : (r.baseexpr))"
								")"
							")",
						reset_on_break_name ? "brrownum('" : "rownum(",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : ")");
		var->resultexpr = ocrpt_expr_parse(o, r, exprstr->str, NULL);
		ocrpt_mem_string_free(exprstr, true);
		ocrpt_variable_initialize_results(o, var->resultexpr);
		break;
	default:
		/* cannot happen */
		break;
	}

	r->dont_add_exprs = false;

	r->variables = ocrpt_list_append(r->variables, var);

	return var;
}

DLL_EXPORT_SYM void ocrpt_variable_free(opencreport *o, ocrpt_report *r, ocrpt_var *var) {
	ocrpt_expr_free(o, r, var->baseexpr);
	ocrpt_expr_free(o, r, var->intermedexpr);
	ocrpt_expr_free(o, r, var->intermed2expr);
	ocrpt_expr_free(o, r, var->resultexpr);
	ocrpt_mem_free(var->name);
	ocrpt_mem_free(var->br_name);
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

DLL_EXPORT_SYM void ocrpt_variable_set_precalculate(ocrpt_var *var, bool value) {
	var->precalculate = value;
}

DLL_EXPORT_SYM void ocrpt_variable_resolve(opencreport *o, ocrpt_report *r, ocrpt_var *v) {
	if (v->baseexpr) {
		ocrpt_expr_resolve_worker(o, r, v->baseexpr, v->baseexpr, v, 0);
		ocrpt_expr_optimize(o, r, v->baseexpr);
	}
	if (v->intermedexpr) {
		ocrpt_expr_resolve_worker(o, r, v->intermedexpr, v->intermedexpr, v, 0);
		ocrpt_expr_optimize(o, r, v->intermedexpr);
	}
	if (v->intermed2expr) {
		ocrpt_expr_resolve_worker(o, r, v->intermed2expr, v->intermed2expr, v, 0);
		ocrpt_expr_optimize(o, r, v->intermed2expr);
	}

	ocrpt_expr_resolve_worker(o, r, v->resultexpr, v->resultexpr, v, 0);
	ocrpt_expr_optimize(o, r, v->resultexpr);

	if (v->br_name && !v->br) {
		ocrpt_break *br = ocrpt_break_get(o, r, v->br_name);

		if (br)
			v->br = br;
		else {
			fprintf(stderr, "%s: break '%s' not found, disabling resetonbreak for v.'%s'\n", __func__, v->br_name, v->name);
			ocrpt_mem_free(v->br_name);
			v->br_name = NULL;
		}
    }
}

DLL_EXPORT_SYM void ocrpt_variable_evaluate(opencreport *o, ocrpt_report *r, ocrpt_var *v) {
	if (!o || !r || !v)
		return;

	if (v->baseexpr)
		ocrpt_expr_eval_worker(o, r, v->baseexpr, v->baseexpr, v);
	if (v->intermedexpr)
		ocrpt_expr_eval_worker(o, r, v->intermedexpr, v->intermedexpr, v);
	if (v->intermed2expr)
		ocrpt_expr_eval_worker(o, r, v->intermed2expr, v->intermed2expr, v);
	ocrpt_expr_eval_worker(o, r, v->resultexpr, v->resultexpr, v);
}

void ocrpt_variable_reset(opencreport *o, ocrpt_var *v) {
	if (!v)
		return;

	/* Don't initialize ocrpt_result pointers on baseexpr */
	if (v->intermedexpr) {
		ocrpt_variable_initialize_results(o, v->intermedexpr);
	}
	if (v->intermed2expr) {
		ocrpt_variable_initialize_results(o, v->intermed2expr);
	}
	ocrpt_variable_initialize_results(o, v->resultexpr);
}
