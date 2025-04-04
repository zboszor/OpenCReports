/*
 * Variable utilities
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mpfr.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "exprutil.h"
#include "datasource.h"
#include "variables.h"
#include "breaks.h"
#include "parts.h"

#define VAR_SUBEXPRS	(5)

/*
 * Create a custom named report variable
 *
 * Returns ocrpt_var pointer or NULL in case of an error.
 */
DLL_EXPORT_SYM ocrpt_var *ocrpt_variable_new_full(ocrpt_report *r, enum ocrpt_result_type type, const char *name, const char *baseexpr, const char *ignoreexpr, const char *intermedexpr, const char *intermed2expr, const char *resultexpr, const char *reset_on_break_name) {
	if (!r) {
		ocrpt_err_printf("invalid variable definitition: valid ocrpt_report pointer expected\n");
		return NULL;
	}
	if (!name) {
		ocrpt_err_printf("invalid variable definitition: valid name expected\n");
		return NULL;
	}

	if (!r->o || r->o->executing || r->executing) {
		ocrpt_err_printf("adding a variable definitition during report execution is invalid\n");
		return NULL;
	}

	ocrpt_var *var = NULL;

	for (ocrpt_list *ptr = r->variables; ptr; ptr = ptr->next) {
		var = (ocrpt_var *)ptr->data;
		if (strcmp(var->name, name) == 0) {
			ocrpt_err_printf("variable '%s': duplicate variable name\n", name);
			return NULL;
		}
	}

	var = ocrpt_mem_malloc(sizeof(ocrpt_var));
	if (!var) {
		ocrpt_err_printf("variable '%s': out of memory\n", name);
		return NULL;
	}

	memset(var, 0, sizeof(ocrpt_var));
	var->r = r;
	var->name = ocrpt_mem_strdup(name);
	var->break_index = SHRT_MAX;
	var->type = OCRPT_VARIABLE_CUSTOM;
	var->basetype = type;

	if (reset_on_break_name && *reset_on_break_name)
		var->br_name = ocrpt_mem_strdup(reset_on_break_name);

	r->dont_add_exprs = true;

	if (!ignoreexpr)
		ignoreexpr = "false";

	struct {
		const char *expr;
		char *error;
	} exprs[VAR_SUBEXPRS] = {
		{ baseexpr, "base expression not specified" },
		{ ignoreexpr, "ignore expression not specified" },
		{ intermedexpr, "intermediate expression not specified" },
		{ intermed2expr, "second intermediate expression not specified" },
		{ resultexpr, "result expression not specified" }
	};

	for (int32_t i = 0; i < VAR_SUBEXPRS; i++) {
		ocrpt_expr *e = NULL;
		char *err = NULL;
		bool free_err = false;

		if (exprs[i].expr) {
			e = ocrpt_report_expr_parse(r, exprs[i].expr, &err);
			free_err = true;
		} else
			err = exprs[i].error;
		if (e) {
			uint32_t vartypes;

			if (e && ocrpt_expr_references(e, OCRPT_VARREF_VVAR, &vartypes)) {
				if ((vartypes & OCRPT_VARIABLE_UNKNOWN_BIT)) {
					ocrpt_err_printf("variable '%s': references an unknown variable name\n", name);
					ocrpt_expr_free(e);
					ocrpt_variable_free(var);
					return NULL;
				}
			}
		} else
			e = ocrpt_report_expr_parse(r, "1", NULL);
		if (i)
			ocrpt_expr_init_iterative_results(e, type);
		if (err) {
			ocrpt_expr_make_error_result(e, err);
			if (free_err)
				ocrpt_strfree(err);
		}
		switch (i) {
		case 0: var->baseexpr = e; break;
		case 1: var->ignoreexpr = e; break;
		case 2: var->intermedexpr = e; break;
		case 3: var->intermed2expr = e; break;
		case 4: var->resultexpr = e; break;
		}
	}

	r->dont_add_exprs = false;

	r->variables = ocrpt_list_append(r->variables, var);

	return var;
}

/*
 * Create a named report variable
 *
 * Possible types are: expression, count, countall, sum,
 * average, averageall, lowest, highest
 *
 * All types (except count and countall) need a base expression.
 *
 * Returns ocrpt_var pointer or NULL in case of an error
 * When returning NULL, the expression is freed so the caller
 * doesn't have to distinguish between different types of error.
 */
DLL_EXPORT_SYM ocrpt_var *ocrpt_variable_new(ocrpt_report *r, ocrpt_var_type type, const char *name, const char *expr, const char *ignoreexpr, const char *reset_on_break_name) {
	ocrpt_expr *e = NULL;
	ocrpt_string *exprstr, *expr1str;
	ocrpt_var *var = NULL;

	switch (type) {
	case OCRPT_VARIABLE_EXPRESSION:
	case OCRPT_VARIABLE_COUNT:
	case OCRPT_VARIABLE_COUNTALL:
	case OCRPT_VARIABLE_SUM:
	case OCRPT_VARIABLE_AVERAGE:
	case OCRPT_VARIABLE_AVERAGEALL:
	case OCRPT_VARIABLE_LOWEST:
	case OCRPT_VARIABLE_HIGHEST:
		if (type != OCRPT_VARIABLE_COUNT && type != OCRPT_VARIABLE_COUNTALL && !expr) {
			ocrpt_err_printf("variable '%s': expression is NULL\n", name);
			return NULL;
		}
		break;
	default:
		ocrpt_err_printf("invalid type for variable '%s': %d\n", name, type);
		ocrpt_expr_free(e);
		return NULL;
	}

	switch (type) {
	case OCRPT_VARIABLE_EXPRESSION:
		var = ocrpt_variable_new_full(r, OCRPT_RESULT_NUMBER, name, expr, ignoreexpr, NULL, NULL, "r.ignoreexpr ? nulln() : r.baseexpr", reset_on_break_name);
		break;
	case OCRPT_VARIABLE_COUNT:
		var = ocrpt_variable_new_full(r, OCRPT_RESULT_NUMBER, name, expr ? expr : "1", ignoreexpr, NULL, NULL, "r.self + ((r.ignoreexpr || isnull(r.baseexpr)) ? 0 : 1)", reset_on_break_name);
		break;
	case OCRPT_VARIABLE_COUNTALL:
		var = ocrpt_variable_new_full(r, OCRPT_RESULT_NUMBER, name, NULL, ignoreexpr, NULL, NULL, "r.self + (r.ignoreexpr ? 0 : 1)", reset_on_break_name);
		break;
	case OCRPT_VARIABLE_SUM:
		exprstr = ocrpt_mem_string_new_printf("(%s%s%s == 1 ? 0 : r.self) + (r.ignoreexpr ? 0 : (isnull(r.baseexpr) ? 0 : r.baseexpr))",
						reset_on_break_name ? "brrownum('" : "r.lineno",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : "");

		var = ocrpt_variable_new_full(r, OCRPT_RESULT_NUMBER, name, expr, ignoreexpr, NULL, NULL, exprstr->str, reset_on_break_name);
		ocrpt_mem_string_free(exprstr, true);
		break;
	case OCRPT_VARIABLE_AVERAGE:
		exprstr = ocrpt_mem_string_new_printf("(%s%s%s == 1 ? 0 : r.self) +  (r.ignoreexpr ? 0 : (isnull(r.baseexpr) ? 0 : r.baseexpr))",
						reset_on_break_name ? "brrownum('" : "r.lineno",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : "");
		var = ocrpt_variable_new_full(r, OCRPT_RESULT_NUMBER, name, expr, ignoreexpr, exprstr->str, "r.self + ((r.ignoreexpr || isnull(r.baseexpr)) ? 0 : 1)", "r.intermedexpr / r.intermed2expr", reset_on_break_name);
		ocrpt_mem_string_free(exprstr, true);
		break;
	case OCRPT_VARIABLE_AVERAGEALL:
		exprstr = ocrpt_mem_string_new_printf("(%s%s%s == 1 ? 0 : r.self) + (r.ignoreexpr ? 0 : (isnull(r.baseexpr) ? 0 : r.baseexpr))",
						reset_on_break_name ? "brrownum('" : "r.lineno",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : "");
		expr1str = ocrpt_mem_string_new_printf("r.intermedexpr / %s%s%s",
						reset_on_break_name ? "brrownum('" : "r.lineno",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : "");
		var = ocrpt_variable_new_full(r, OCRPT_RESULT_NUMBER, name, expr, ignoreexpr, exprstr->str, NULL, expr1str->str, reset_on_break_name);
		ocrpt_mem_string_free(exprstr, true);
		ocrpt_mem_string_free(expr1str, true);
		break;
	case OCRPT_VARIABLE_LOWEST:
		exprstr = ocrpt_mem_string_new_printf(
						"%s%s%s == 1 ? "
							"(r.baseexpr) : "
							"(isnull(r.self) ? "
								"(r.baseexpr) : "
								"((r.ignoreexpr || isnull(r.baseexpr)) ? "
									"r.self : "
									"(r.self < (r.baseexpr) ? r.self : (r.baseexpr))"
								")"
							")",
						reset_on_break_name ? "brrownum('" : "r.lineno",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : "");
		var = ocrpt_variable_new_full(r, OCRPT_RESULT_NUMBER, name, expr, ignoreexpr, NULL, NULL, exprstr->str, reset_on_break_name);
		ocrpt_mem_string_free(exprstr, true);
		break;
	case OCRPT_VARIABLE_HIGHEST:
		exprstr = ocrpt_mem_string_new_printf(
						"%s%s%s == 1 ? "
							"(r.baseexpr) : "
							"(isnull(r.self) ? "
								"(r.baseexpr) : "
								"((r.ignoreexpr || isnull(r.baseexpr)) ? "
									"r.self : "
									"(r.self > (r.baseexpr) ? r.self : (r.baseexpr))"
								")"
							")",
						reset_on_break_name ? "brrownum('" : "r.lineno",
						reset_on_break_name ? reset_on_break_name : "",
						reset_on_break_name ? "')" : "");
		var = ocrpt_variable_new_full(r, OCRPT_RESULT_NUMBER, name, expr, ignoreexpr, NULL, NULL, exprstr->str, reset_on_break_name);
		ocrpt_mem_string_free(exprstr, true);
		break;
	default:
		/* cannot happen */
		break;
	}

	if (var)
		var->type = type;
	return var;
}

DLL_EXPORT_SYM ocrpt_var_type ocrpt_variable_get_type(ocrpt_var *v) {
	return v ? v->type : OCRPT_VARIABLE_INVALID;
}

void ocrpt_variable_free(ocrpt_var *var) {
	ocrpt_list_free_deep(var->precalc_results, (ocrpt_mem_free_t)ocrpt_result_free);
	ocrpt_expr_free(var->baseexpr);
	ocrpt_expr_free(var->ignoreexpr);
	ocrpt_expr_free(var->intermedexpr);
	ocrpt_expr_free(var->intermed2expr);
	ocrpt_expr_free(var->resultexpr);
	ocrpt_mem_free(var->name);
	ocrpt_mem_free(var->br_name);
	ocrpt_mem_free(var);
}

void ocrpt_variables_free(ocrpt_report *r) {
	if (!r)
		return;

	for (ocrpt_list *ptr = r->variables; ptr; ptr = ptr->next)
		ocrpt_variable_free((ocrpt_var *)ptr->data);

	ocrpt_list_free(r->variables);
	r->variables = NULL;
}

DLL_EXPORT_SYM void ocrpt_variable_set_precalculate(ocrpt_var *var, const char *expr_string) {
	if (!var || !var->r || !var->r->o || var->r->o->executing)
		return;

	ocrpt_expr_free(var->precalculate_expr);
	var->precalculate_expr = expr_string ? ocrpt_report_expr_parse(var->r, expr_string, NULL) : NULL;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_variable_get_precalculate(ocrpt_var *var) {
	return var ? var->precalculate_expr : NULL;
}

DLL_EXPORT_SYM void ocrpt_variable_resolve(ocrpt_var *v) {
	if (!v || !v->r || !v->r->o || v->r->o->executing || v->r->executing)
		return;

	if (v->baseexpr) {
		ocrpt_expr_resolve_worker(v->baseexpr, NULL, v->baseexpr, v, 0, true, NULL);
		ocrpt_expr_optimize(v->baseexpr);
	}
	if (v->ignoreexpr) {
		ocrpt_expr_resolve_worker(v->ignoreexpr, NULL, v->ignoreexpr, v, 0, true, NULL);
		ocrpt_expr_optimize(v->ignoreexpr);
	}
	if (v->intermedexpr) {
		ocrpt_expr_resolve_worker(v->intermedexpr, NULL, v->intermedexpr, v, 0, true, NULL);
		ocrpt_expr_optimize(v->intermedexpr);
	}
	if (v->intermed2expr) {
		ocrpt_expr_resolve_worker(v->intermed2expr, NULL, v->intermed2expr, v, 0, true, NULL);
		ocrpt_expr_optimize(v->intermed2expr);
	}

	ocrpt_expr_resolve_worker(v->resultexpr, NULL, v->resultexpr, v, 0, true, NULL);
	ocrpt_expr_optimize(v->resultexpr);

	if (v->br_name && !v->br) {
		ocrpt_break *br = ocrpt_break_get(v->r, v->br_name);

		if (br) {
			v->br = br;
			v->break_index = br->index;
		} else {
			ocrpt_err_printf("break '%s' not found, disabling resetonbreak for v.'%s'\n", v->br_name, v->name);
			ocrpt_mem_free(v->br_name);
			v->br_name = NULL;
		}
    }
}

DLL_EXPORT_SYM void ocrpt_variable_evaluate(ocrpt_var *v) {
	if (!v)
		return;

	if (v->r->o->precalculate || !v->precalculate) {
		if (v->baseexpr)
			ocrpt_expr_eval_worker(v->baseexpr, v->baseexpr, v);
		if (v->ignoreexpr)
			ocrpt_expr_eval_worker(v->ignoreexpr, v->ignoreexpr, v);
		if (v->intermedexpr)
			ocrpt_expr_eval_worker(v->intermedexpr, v->intermedexpr, v);
		if (v->intermed2expr)
			ocrpt_expr_eval_worker(v->intermed2expr, v->intermed2expr, v);
		ocrpt_expr_eval_worker(v->resultexpr, v->resultexpr, v);
	}
}

void ocrpt_variable_reset(ocrpt_var *v) {
	if (!v)
		return;

	/* Don't initialize ocrpt_result pointers on baseexpr */
	if (v->intermedexpr)
		ocrpt_expr_init_iterative_results(v->intermedexpr, v->basetype);
	if (v->intermed2expr)
		ocrpt_expr_init_iterative_results(v->intermed2expr, v->basetype);
	ocrpt_expr_init_iterative_results(v->resultexpr, v->basetype);
}

void ocrpt_variables_add_precalculated_results(ocrpt_report *r, ocrpt_list *brl_start, bool last_row) {
	if (!r)
		return;

	for (ocrpt_list *l = r->variables; l; l = l->next) {
		ocrpt_var *var = (ocrpt_var *)l->data;
		if (var->precalculate) {
			ocrpt_list *brl;
			bool var_br_triggered = false;

			if (var->br) {
				for (brl = brl_start; brl; brl = brl->next) {
					if (var->br == brl->data) {
						var_br_triggered = true;
						break;
					}
				}
			} else if (last_row)
				var_br_triggered = true;

			if (var_br_triggered) {
				ocrpt_result *dst = ocrpt_result_new(r->o);
				ocrpt_result_copy(dst, EXPR_RESULT(var->resultexpr));
				var->precalc_results = ocrpt_list_append(var->precalc_results, dst);
			}
		}
	}
}

void ocrpt_variables_advance_precalculated_results(ocrpt_report *r, ocrpt_list *brl_start) {
	if (!r)
		return;

	for (ocrpt_list *l = r->variables; l; l = l->next) {
		ocrpt_var *var = (ocrpt_var *)l->data;
		if (var->precalculate) {
			if (var->br) {
				if (!var->precalc_rptr)
					var->precalc_rptr = var->precalc_results;
				else {
					ocrpt_list *brl;
					bool var_br_triggered = false;

					for (brl = brl_start; brl; brl = brl->next) {
						if (var->br == brl->data) {
							var_br_triggered = true;
							break;
						}
					}

					if (var_br_triggered && var->precalc_rptr->next)
						var->precalc_rptr = var->precalc_rptr->next;
				}
			} else if (var->precalc_results && !var->precalc_rptr) {
				ocrpt_list *l1;
				for (l1 = var->precalc_results; l1 && l1->next; l1 = l1->next)
					;
				var->precalc_rptr = l1;
			}
		}
	}
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_variable_baseexpr(ocrpt_var *v) {
	return v ? v->baseexpr : NULL;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_variable_ignoreexpr(ocrpt_var *v) {
	return v ? v->ignoreexpr : NULL;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_variable_intermedexpr(ocrpt_var *v) {
	return v ? v->intermedexpr : NULL;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_variable_intermed2expr(ocrpt_var *v) {
	return v ? v->intermed2expr : NULL;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_variable_resultexpr(ocrpt_var *v) {
	return v ? v->resultexpr : NULL;
}

DLL_EXPORT_SYM ocrpt_var *ocrpt_variable_get_next(ocrpt_report *r, ocrpt_list **list) {
	if (!r || !list)
		return NULL;

	*list = *list ? (*list)->next : r->variables;
	return (ocrpt_var *)(*list ? (*list)->data : NULL);
}
