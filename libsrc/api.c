/*
 * OpenCReports main module
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <libintl.h>
#include <locale.h>
#include <langinfo.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/random.h>
#include <sys/stat.h>

#include <cairo.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <utf8proc.h>
#include <paper.h>

#include "scanner.h"
#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "datasource.h"
#include "variables.h"
#include "breaks.h"
#include "exprutil.h"
#include "functions.h"
#include "parts.h"
#include "parsexml.h"
#include "color.h"
#include "layout.h"
#include "pdf-output.h"
#include "html-output.h"
#include "txt-output.h"
#include "csv-output.h"
#include "xml-output.h"
#include "json-output.h"

static int ocrpt_stderr_printf(const char *fmt, ...) {
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vfprintf(stderr, fmt, ap);
	va_end(ap);

	return ret;
}

DLL_EXPORT_SYM ocrpt_printf_func ocrpt_std_printf = printf;
DLL_EXPORT_SYM ocrpt_printf_func ocrpt_err_printf = ocrpt_stderr_printf;

DLL_EXPORT_SYM void ocrpt_set_printf_func(ocrpt_printf_func func) {
	if (func)
		ocrpt_std_printf = func;
}

DLL_EXPORT_SYM void ocrpt_set_err_printf_func(ocrpt_printf_func func) {
	if (func)
		ocrpt_err_printf = func;
}

char cwdpath[PATH_MAX];
static ocrpt_paper *papersizes;
static const ocrpt_paper *system_paper;
int n_papersizes;

DLL_EXPORT_SYM const char *ocrpt_version(void) {
	return "OpenCReports " VERSION;
}

DLL_EXPORT_SYM opencreport *ocrpt_init(void) {
	opencreport *o = (opencreport *)ocrpt_mem_malloc(sizeof(struct opencreport));
	unsigned long seed;

	if (!o)
		return NULL;

	/*
	 * If getrandom() fails, fill the random seed from
	 * the garbage data in the allocated area before
	 * zeroing it out.
	 */
	if (getrandom(&seed, sizeof(seed), GRND_NONBLOCK) < 0)
		memcpy(&seed, o, sizeof(seed));

	memset(o, 0, sizeof(struct opencreport));

	/* Default to PDF output */
	o->output_format = OCRPT_OUTPUT_PDF;
	o->rptformat = ocrpt_result_new(o);
	o->rptformat->type = OCRPT_RESULT_STRING;
	o->rptformat->string = ocrpt_mem_string_new_with_len("PDF", 8);
	o->rptformat->string_owned = true;

	o->paper = system_paper;
	o->prec = OCRPT_MPFR_PRECISION_BITS;
	o->rndmode = MPFR_RNDN;
	gmp_randinit_default(o->randstate);
	gmp_randseed_ui(o->randstate, seed);
	o->c_locale = o->locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);

	o->current_date = ocrpt_result_new(o);
	o->current_date->type = OCRPT_RESULT_DATETIME;

	o->current_timestamp = ocrpt_result_new(o);
	o->current_timestamp->type = OCRPT_RESULT_DATETIME;

	o->pageno = ocrpt_result_new(o);
	o->pageno->type = OCRPT_RESULT_NUMBER;
	mpfr_init2(o->pageno->number, o->prec);
	o->pageno->number_initialized = true;
	mpfr_set_ui(o->pageno->number, 1, o->rndmode);

	o->totpages = ocrpt_result_new(o);
	o->totpages->type = OCRPT_RESULT_NUMBER;
	mpfr_init2(o->totpages->number, o->prec);
	o->totpages->number_initialized = true;
	mpfr_set_ui(o->totpages->number, 1, o->rndmode);

	return o;
}

DLL_EXPORT_SYM void ocrpt_datasource_free(ocrpt_datasource *source) {
	if (!source)
		return;

	opencreport *o = source->o;

	if (source->input && source->input->close)
		source->input->close(source);
	ocrpt_strfree(source->name);
	ocrpt_mem_free(source);

	o->datasources = ocrpt_list_remove(o->datasources, source);
}

static void ocrpt_free_search_path(const void *ptr) {
	const ocrpt_search_path *p = ptr;

	ocrpt_mem_free(p->path);
	/* p->expr is not needed to be freed. */
	ocrpt_mem_free(p);
}

static void ocrpt_free_mvarentry(const void *ptr) {
	const ocrpt_mvarentry *e = ptr;
	ocrpt_mem_free(e->name);
	ocrpt_mem_free(e->value);
	ocrpt_mem_free(e);
}

DLL_EXPORT_SYM void ocrpt_free(opencreport *o) {
	if (!o)
		return;

	int32_t i;
	for (i = 0; i < o->n_functions; i++) {
		const ocrpt_function *f = o->functions[i];

		ocrpt_strfree(f->fname);
		ocrpt_mem_free(f);
	}
	ocrpt_mem_free(o->functions);

	/*
	 * ocrpt_free_query() or ocrpt_free_query0()
	 * must not be called from a List iterator on
	 * o->queries since ocrpt_free_query0() modifies
	 * o->queries.
	 */
	while (o->queries) {
		ocrpt_query *q = (ocrpt_query *)o->queries->data;
		ocrpt_query_free(q);
	}

	while (o->datasources) {
		ocrpt_datasource *ds = (ocrpt_datasource *)o->datasources->data;
		ocrpt_datasource_free(ds);
	}

	ocrpt_parts_free(o);

	for (ocrpt_list *l = o->exprs; l; l = l->next) {
		ocrpt_expr *e = (ocrpt_expr *)l->data;
		/*
		 * Any ocrpt_report pointer is freed at this point.
		 * Setting the lingering pointer to NULL avoids use-after-free.
		 */
		e->r = NULL;
		ocrpt_expr_free_internal(e, false);
	}
	ocrpt_list_free(o->exprs);

	if (o->locale != o->c_locale)
		freelocale(o->c_locale);
	if (o->locale)
		freelocale(o->locale);

	ocrpt_list_free_deep(o->part_added_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->part_iteration_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->report_added_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->report_start_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->report_iteration_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->report_done_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->report_newrow_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->report_precalc_done_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->precalc_done_callbacks, ocrpt_mem_free);

	ocrpt_list_free_deep(o->mvarlist, ocrpt_free_mvarentry);
	ocrpt_list_free_deep(o->search_paths, ocrpt_free_search_path);
	ocrpt_list_free_deep(o->images, ocrpt_image_free);

	gmp_randclear(o->randstate);
	ocrpt_mem_string_free(o->converted, true);

	ocrpt_result_free(o->current_date);
	ocrpt_result_free(o->current_timestamp);
	ocrpt_result_free(o->pageno);
	ocrpt_result_free(o->totpages);
	ocrpt_result_free(o->rptformat);

	ocrpt_mem_string_free(o->output_buffer, true);
	if (o->content_type)
		for (i = 0; o->content_type[i]; i++)
			ocrpt_mem_string_free((ocrpt_string *)o->content_type[i], true);
	ocrpt_mem_free(o->content_type);
	ocrpt_mem_free(o->html_meta);
	ocrpt_mem_free(o->html_docroot);
	ocrpt_mem_free(o->csv_filename);
	ocrpt_mem_free(o->csv_delimiter);
	ocrpt_mem_free(o->textdomain);
	ocrpt_mem_free(o->xlate_domain_s);
	ocrpt_mem_free(o->xlate_dir_s);

	ocrpt_mem_free(o);
}

DLL_EXPORT_SYM void ocrpt_set_numeric_precision_bits(opencreport *o, const char *expr_string) {
	if (!o)
		return;

	ocrpt_expr_free(o->precision_expr);
	o->precision_expr = NULL;
	if (expr_string) {
		char *err = NULL;
		o->precision_expr = ocrpt_expr_parse(o, expr_string, &err);
		if (err) {
			ocrpt_err_printf("ocrpt_set_numeric_precision_bits: %s\n", err);
			ocrpt_strfree(err);
		}

		ocrpt_expr_resolve(o->precision_expr);
		ocrpt_expr_optimize(o->precision_expr);
		if (o->precision_expr) {
			ocrpt_result *prec = ocrpt_expr_get_result(o->precision_expr);
			if (ocrpt_result_isnumber(prec))
			o->prec = ocrpt_expr_get_long(o->precision_expr);
		} else
			o->prec = OCRPT_MPFR_PRECISION_BITS;
	} else
		o->prec = OCRPT_MPFR_PRECISION_BITS;
}

DLL_EXPORT_SYM mpfr_prec_t ocrpt_get_numeric_precision_bits(opencreport *o) {
	return o ? o->prec : OCRPT_MPFR_PRECISION_BITS;
}

DLL_EXPORT_SYM void ocrpt_set_rounding_mode(opencreport *o, const char *expr_string) {
	if (!o)
		return;

	ocrpt_expr_free(o->rounding_mode_expr);
	o->rounding_mode_expr = NULL;
	if (expr_string) {
		char *err = NULL;
		o->rounding_mode_expr = ocrpt_expr_parse(o, expr_string, &err);
		if (err) {
			ocrpt_err_printf("ocrpt_set_rounding_mode: %s\n", err);
			ocrpt_strfree(err);
		}
	}
}

DLL_EXPORT_SYM bool ocrpt_add_part_added_cb(opencreport *o, ocrpt_part_cb func, void *data) {
	if (!o || !func)
		return false;

	ocrpt_part_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_part_cb_data));
	ptr->func = func;
	ptr->data = data;

	o->part_added_callbacks = ocrpt_list_append(o->part_added_callbacks, ptr);

	return true;
}

DLL_EXPORT_SYM bool ocrpt_add_report_added_cb(opencreport *o, ocrpt_report_cb func, void *data) {
	if (!o || !func)
		return false;

	ocrpt_report_cb_data *ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	ptr->func = func;
	ptr->data = data;

	o->report_added_callbacks = ocrpt_list_append(o->report_added_callbacks, ptr);
	return true;
}

static void ocrpt_print_reportheader(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, uint32_t rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	if (!pr->start_page && !r->current_iteration) {
		if (o->output_functions.get_current_page)
			pr->start_page = o->output_functions.get_current_page(o);
		pr->start_page_position = *page_position;
		pd->start_page_position = *page_position;
	}
	if (pd->border_width_expr)
		*page_position += pd->border_width;
	ocrpt_layout_output_init(&r->reportheader);
	ocrpt_layout_output(o, p, pr, pd, r, NULL, &r->reportheader, rows, newpage, page_indent, page_position, old_page_position);
}

static unsigned int ocrpt_execute_one_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	ocrpt_list *brl_start = NULL;
	unsigned int rows = 0;
	bool have_row;

	ocrpt_query_navigate_start(r->query);
	have_row = ocrpt_query_navigate_next(r->query);

	ocrpt_expr_init_iterative_results(r->detailcnt, OCRPT_RESULT_NUMBER);

	ocrpt_expr_set_plain_iterative_to_null(r);

	for (ocrpt_list *vl = r->variables; vl; vl = vl->next)
		ocrpt_variable_reset((ocrpt_var *)vl->data);

	while (have_row) {
		ocrpt_list *brl;
		bool last_row = !ocrpt_query_navigate_next(r->query);

		o->residx = ocrpt_expr_prev_residx(o->residx);

		rows++;

		brl_start = NULL;
		for (brl = r->breaks; brl; brl = brl->next) {
			ocrpt_break *br = (ocrpt_break *)brl->data;

			br->cb_triggered = false;

			if (ocrpt_break_check_fields(br)) {
				if (!brl_start)
					brl_start = brl;

				br->blank = br->suppressblank && ocrpt_break_check_blank(br, false);
			}
		}

		if (brl_start) {
			if (rows > 1) {
				/* Use the previous row data temporarily */
				o->residx = ocrpt_expr_prev_residx(o->residx);

				if (o->precalculate)
					ocrpt_variables_add_precalculated_results(r, brl_start, last_row);

				/* Switch back to the current row data */
				o->residx = ocrpt_expr_next_residx(o->residx);

				for (brl = brl_start; brl; brl = brl->next)
					ocrpt_break_reset_vars((ocrpt_break *)brl->data);

				if (!o->precalculate)
					ocrpt_variables_advance_precalculated_results(r, brl_start);
			}
		}

		ocrpt_report_evaluate_variables(r);
		ocrpt_report_evaluate_expressions(r);

		for (brl = last_row ? r->breaks : brl_start; brl; brl = brl->next) {
			ocrpt_break *br = (ocrpt_break *)brl->data;
			ocrpt_list *brcbl;

			br->cb_triggered = true;
			if (!o->precalculate) {
				for (brcbl = br->callbacks; brcbl; brcbl = brcbl->next) {
					ocrpt_break_trigger_cb_data *cbd = (ocrpt_break_trigger_cb_data *)brcbl->data;

					cbd->func(o, r, br, cbd->data);
				}
			}
		}

		if (rows == 1 && !*newpage)
			ocrpt_print_reportheader(o, p, pr, pd, r, rows, newpage, page_indent, page_position, old_page_position);

		if (r->fieldheader_high_priority && rows == 1) {
			ocrpt_layout_output_init(&r->fieldheader);
			ocrpt_layout_output(o, p, pr, pd, r, NULL, &r->fieldheader, rows, newpage, page_indent, page_position, old_page_position);
		}

		if (rows > 1 && brl_start) {
			/* Use the previous row data temporarily */
			o->residx = ocrpt_expr_prev_residx(o->residx);

			for (brl = r->breaks_reverse; brl; brl = brl->next) {
				ocrpt_break *br = (ocrpt_break *)brl->data;

				if (!br->suppressblank || (br->suppressblank && !br->blank_prev)) {
					ocrpt_layout_output_init(&br->footer);
					ocrpt_layout_output(o, p, pr, pd, r, br, &br->footer, rows, newpage, page_indent, page_position, old_page_position);
				}

				if (br == brl_start->data)
					break;
			}

			/* Switch back to the current row data */
			o->residx = ocrpt_expr_next_residx(o->residx);
		}

		for (brl = (rows == 1 ? r->breaks : brl_start); brl; brl = brl->next) {
			ocrpt_break *br __attribute__((unused)) = (ocrpt_break *)brl->data;

			if (br->cb_triggered) {
				if (!br->suppressblank || (br->suppressblank && !br->blank)) {
					if (rows > 1 && br->headernewpage) {
						if (o->output_functions.supports_page_break)
							*newpage = true;
					}
					ocrpt_layout_output_init(&br->header);
					ocrpt_layout_output(o, p, pr, pd, r, br, &br->header, rows, newpage, page_indent, page_position, old_page_position);
				}
				br->blank_prev = br->blank;
			}
		}

		if (!o->precalculate) {
			ocrpt_list *cbl;

			for (cbl = r->newrow_callbacks; cbl; cbl = cbl->next) {
				ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

				cbd->func(o, r, cbd->data);
			}

			for (cbl = o->report_newrow_callbacks; cbl; cbl = cbl->next) {
				ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

				cbd->func(o, r, cbd->data);
			}
		}

		if (!r->fieldheader_high_priority && (rows == 1 || brl_start)) {
			/*
			 * Debatable preference in taste:
			 * a) field headers have higher precedence than break headers
			 *    and break footers, meaning the field headers are printed
			 *    once per page at the top, with break headers and footers
			 *    printed after it, or
			 * b) break headers and footers have higher precedence than
			 *    field headers, with break headers printed first, then
			 *    the field headers, followed by all the field details,
			 *    then finally the break footers.
			 *
			 * It is configurable via <Report field_header_preference="high/low">
			 * with the default "high" value.
			 */
			if (rows > 1) {
				ocrpt_expr_init_iterative_results(r->detailcnt, OCRPT_RESULT_NUMBER);
				ocrpt_expr_eval(r->detailcnt);
				ocrpt_report_evaluate_detailcnt_dependees(r);
			}
			ocrpt_layout_output_init(&r->fieldheader);
			ocrpt_layout_output(o, p, pr, pd, r, NULL, &r->fieldheader, rows, newpage, page_indent, page_position, old_page_position);
		}
		ocrpt_layout_output_init(&r->fielddetails);
		ocrpt_layout_output(o, p, pr, pd, r, NULL, &r->fielddetails, rows, newpage, page_indent, page_position, old_page_position);

		have_row = !last_row;
		o->residx = ocrpt_expr_next_residx(o->residx);
	}

	if (rows) {
		/* Use the previous row data temporarily */
		o->residx = ocrpt_expr_prev_residx(o->residx);

		if (o->precalculate) {
			ocrpt_variables_add_precalculated_results(r, r->breaks, true);
			ocrpt_report_expressions_add_delayed_results(r);
		}

		for (ocrpt_list *brl = r->breaks_reverse; brl; brl = brl->next) {
			ocrpt_break *br = (ocrpt_break *)brl->data;

			ocrpt_layout_output_init(&br->footer);
			ocrpt_layout_output(o, p, pr, pd, r, br, &br->footer, rows, newpage, page_indent, page_position, old_page_position);
		}

		ocrpt_layout_output_init(&r->reportfooter);
		ocrpt_layout_output(o, p, pr, pd, r, NULL, &r->reportfooter, rows, newpage, page_indent, page_position, old_page_position);

		/* Switch back to the current row data */
		o->residx = ocrpt_expr_next_residx(o->residx);
	}

	return rows;
}

static void ocrpt_execute_parts_resolve_all_reports(opencreport *o, ocrpt_part *p) {
	for (ocrpt_list *row = p->rows; row; row = row->next) {
		ocrpt_part_row *pr = (ocrpt_part_row *)row->data;
		ocrpt_list *pdl;

		for (pdl = pr->pd_list; pdl; pdl = pdl->next) {
			ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

			for (ocrpt_list *rl = pd->reports; rl; rl = rl->next) {
				ocrpt_report *r = (ocrpt_report *)rl->data;

				ocrpt_query_navigate_start(r->query);

				if (o->precalculate) {
					ocrpt_report_resolve_breaks(r);
					ocrpt_report_resolve_variables(r);
					ocrpt_report_resolve_expressions(r);
				}
			}
		}
	}
}

static void ocrpt_execute_evaluate_global_params(opencreport *o) {
	/*
	 * Make all queries stand on their first row
	 * so part, part row, column and part report parameters
	 * and settings in their headers can use values of
	 * query columns. This is expected by RLIB semantics:
	 * some queries may be independent/standalone instead
	 * of followers to the report's main query. Usually
	 * such queries have a single row and they may be
	 * used by contexts outside a part report.
	 */
	for (ocrpt_list *ql = o->queries; ql; ql = ql->next) {
		ocrpt_query *q = (ocrpt_query *)ql->data;

		if (q->leader)
			continue;

		ocrpt_query_navigate_start(q);
		ocrpt_query_navigate_next(q);
	}

	ocrpt_resolve_search_paths(o);

	ocrpt_expr_resolve_nowarn(o->size_unit_expr);
	ocrpt_expr_optimize(o->size_unit_expr);
	o->size_in_points = false;
	if (o->size_unit_expr) {
		const ocrpt_string *str = ocrpt_expr_get_string(o->size_unit_expr);
		o->size_in_points = str->str ? strcasecmp(str->str, "points") == 0 : false;
	}

	ocrpt_expr_resolve(o->noquery_show_nodata_expr);
	ocrpt_expr_optimize(o->noquery_show_nodata_expr);
	if (o->noquery_show_nodata_expr)
		o->noquery_show_nodata = !!ocrpt_expr_get_long(o->noquery_show_nodata_expr);

	ocrpt_expr_resolve(o->report_height_after_last_expr);
	ocrpt_expr_optimize(o->report_height_after_last_expr);
	if (o->report_height_after_last_expr)
		o->report_height_after_last = !!ocrpt_expr_get_long(o->report_height_after_last_expr);

	ocrpt_expr_resolve(o->follower_match_single_expr);
	ocrpt_expr_optimize(o->follower_match_single_expr);
	if (o->follower_match_single_expr)
		o->follower_match_single = !!ocrpt_expr_get_long(o->follower_match_single_expr);

	ocrpt_expr_resolve(o->precision_expr);
	ocrpt_expr_optimize(o->precision_expr);
	if (o->precision_expr) {
		ocrpt_result *prec = ocrpt_expr_get_result(o->precision_expr);
		if (ocrpt_result_isnumber(prec))
			o->prec = ocrpt_expr_get_long(o->precision_expr);
	}

	ocrpt_expr_resolve_nowarn(o->rounding_mode_expr);
	ocrpt_expr_optimize(o->rounding_mode_expr);
	if (o->rounding_mode_expr) {
		const ocrpt_string *str = ocrpt_expr_get_string(o->rounding_mode_expr);
		const char *mode = str->str;
		if (strcasecmp(mode, "nearest") == 0)
			o->rndmode = MPFR_RNDN;
		else if (strcasecmp(mode, "to_minus_inf") == 0)
			o->rndmode = MPFR_RNDD;
		else if (strcasecmp(mode, "to_inf") == 0)
			o->rndmode = MPFR_RNDU;
		else if (strcasecmp(mode, "to_zero") == 0)
			o->rndmode = MPFR_RNDZ;
		else if (strcasecmp(mode, "away_from_zero") == 0)
			o->rndmode = MPFR_RNDA;
		else if (strcasecmp(mode, "faithful") == 0)
			o->rndmode = MPFR_RNDF;
		else /* default "nearest" */
			o->rndmode = MPFR_RNDN;
	}

	ocrpt_expr_resolve(o->locale_expr);
	ocrpt_expr_optimize(o->locale_expr);
	if (o->locale == o->c_locale && o->locale_expr) {
		const ocrpt_string *l = ocrpt_expr_get_string(o->locale_expr);

		ocrpt_set_locale(o, l->str);
	}

	if (!o->textdomain && o->xlate_domain_s && o->xlate_dir_s) {
		ocrpt_expr *domain_expr, *dir_expr;
		char *err;
		const ocrpt_string *domain0, *dir0;
		const char *domain, *dir;

		err = NULL;
		domain_expr = ocrpt_expr_parse(o, o->xlate_domain_s, &err);
		if (domain_expr) {
			ocrpt_expr_resolve_nowarn(domain_expr);
			ocrpt_expr_optimize(domain_expr);
			domain0 = ocrpt_expr_get_string(domain_expr);
			domain = domain0->str;
		} else {
			ocrpt_err_printf("ocrpt_execute_evaluate_global_params: %s\n", err);
			ocrpt_strfree(err);
			domain = o->xlate_domain_s;
		}

		err = NULL;
		dir_expr = ocrpt_expr_parse(o, o->xlate_dir_s, &err);
		if (dir_expr) {
			ocrpt_expr_resolve(dir_expr);
			ocrpt_expr_optimize(dir_expr);
			dir0 = ocrpt_expr_get_string(dir_expr);
			dir = dir0->str;
		} else {
			ocrpt_err_printf("ocrpt_execute_evaluate_global_params: %s\n", err);
			ocrpt_strfree(err);
			dir = o->xlate_dir_s;
		}

		bindtextdomain(domain, dir);
		bind_textdomain_codeset(domain, "UTF-8");
		o->textdomain = ocrpt_mem_strdup(domain);

		ocrpt_expr_free(domain_expr);
		ocrpt_expr_free(dir_expr);
	}
}

static void ocrpt_execute_parts_evaluate_global_params(opencreport *o, ocrpt_part *p) {
	/*
	 * Make all queries stand on their first row
	 * so part, part row, column and part report parameters
	 * and settings in their headers can use values of
	 * query columns. This is expected by RLIB semantics:
	 * some queries may be independent/standalone instead
	 * of followers to the report's main query. Usually
	 * such queries have a single row and they may be
	 * used by contexts outside a part report.
	 */
	for (ocrpt_list *ql = o->queries; ql; ql = ql->next) {
		ocrpt_query *q = (ocrpt_query *)ql->data;

		if (q->leader)
			continue;

		ocrpt_query_navigate_start(q);
		ocrpt_query_navigate_next(q);
	}

	ocrpt_layout_output_evaluate_expr_params(&p->pageheader);
	ocrpt_layout_output_evaluate_expr_params(&p->pagefooter);

	ocrpt_layout_output_resolve(&p->pageheader);
	ocrpt_layout_output_resolve(&p->pagefooter);

	ocrpt_expr_resolve_nowarn(p->paper_type_expr);
	ocrpt_expr_optimize(p->paper_type_expr);
	if (p->paper_type_expr) {
		const ocrpt_string *paper = ocrpt_expr_get_string(p->paper_type_expr);
		if (paper)
			p->paper = ocrpt_get_paper_by_name(paper->str);
	}
	if (!p->paper)
		p->paper = o->paper;

	ocrpt_expr_resolve_nowarn(p->font_name_expr);
	ocrpt_expr_optimize(p->font_name_expr);
	if (o->output_functions.support_any_font) {
		if (!p->font_name && p->font_name_expr)
			p->font_name = ocrpt_expr_get_string(p->font_name_expr)->str;
		if (!p->font_name)
			p->font_name = "Courier";
	} else
		p->font_name = "Courier";

	ocrpt_expr_resolve(p->font_size_expr);
	ocrpt_expr_optimize(p->font_size_expr);
	if (o->output_functions.support_any_font) {
		if (p->font_size_expr) {
			double font_size = ocrpt_expr_get_double(p->font_size_expr);

			p->font_size = (font_size > 0.0 ? font_size : o->font_size);
			if (o->output_functions.set_font_sizes)
				o->output_functions.set_font_sizes(o, p->font_name, p->font_size, false, false, NULL, &p->font_width);
		} else {
			p->font_size = o->font_size;
			p->font_width = o->font_width;
		}
	} else
		p->font_size = OCRPT_DEFAULT_FONT_SIZE;

	ocrpt_expr_resolve(p->top_margin_expr);
	ocrpt_expr_optimize(p->top_margin_expr);
	if (p->top_margin_expr) {
		double margin = ocrpt_expr_get_double(p->top_margin_expr);
		if (margin > 0.0)
			p->top_margin = margin;
	}

	ocrpt_expr_resolve(p->bottom_margin_expr);
	ocrpt_expr_optimize(p->bottom_margin_expr);
	if (p->bottom_margin_expr) {
		double margin = ocrpt_expr_get_double(p->bottom_margin_expr);
		if (margin > 0.0)
			p->bottom_margin = margin;
	}

	ocrpt_expr_resolve(p->left_margin_expr);
	ocrpt_expr_optimize(p->left_margin_expr);
	if (p->left_margin_expr) {
		double margin = ocrpt_expr_get_double(p->left_margin_expr);
		if (margin > 0.0)
			p->left_margin = margin;
	}

	ocrpt_expr_resolve(p->right_margin_expr);
	ocrpt_expr_optimize(p->right_margin_expr);
	if (p->right_margin_expr) {
		double margin = ocrpt_expr_get_double(p->right_margin_expr);
		if (margin > 0.0)
			p->right_margin = margin;
	}

	ocrpt_expr_resolve(p->iterations_expr);
	ocrpt_expr_optimize(p->iterations_expr);
	p->iterations = 1;
	if (p->iterations_expr) {
		long it = ocrpt_expr_get_long(p->iterations_expr);
		if (it > 1)
			p->iterations = it;
	}

	ocrpt_expr_resolve(p->suppress_expr);
	ocrpt_expr_optimize(p->suppress_expr);
	if (p->suppress_expr)
		p->suppress = !!ocrpt_expr_get_long(p->suppress_expr);

	ocrpt_expr_resolve(p->suppress_pageheader_firstpage_expr);
	ocrpt_expr_optimize(p->suppress_pageheader_firstpage_expr);
	if (p->suppress_pageheader_firstpage_expr)
		p->suppress_pageheader_firstpage = !!ocrpt_expr_get_long(p->suppress_pageheader_firstpage_expr);

	ocrpt_expr_resolve_nowarn(p->orientation_expr);
	ocrpt_expr_optimize(p->orientation_expr);
	if (p->orientation_expr) {
		const ocrpt_string *orientation = ocrpt_expr_get_string(p->orientation_expr);
		p->landscape = orientation && (strcasecmp(orientation->str, "landscape") == 0);
	}

	if (p->landscape) {
		p->paper_width = p->paper->height;
		p->paper_height = p->paper->width;
	} else {
		p->paper_width = p->paper->width;
		p->paper_height = p->paper->height;
	}

	for (ocrpt_list *row = p->rows; row; row = row->next) {
		ocrpt_part_row *pr = (ocrpt_part_row *)row->data;

		ocrpt_expr_resolve_nowarn(pr->layout_expr);
		ocrpt_expr_optimize(pr->layout_expr);
		if (pr->layout_expr) {
			const ocrpt_string *layout = ocrpt_expr_get_string(pr->layout_expr);
			pr->fixed = layout && (strcasecmp(layout->str, "fixed") == 0);
		}

		ocrpt_expr_resolve_nowarn(pr->suppress_expr);
		ocrpt_expr_optimize(pr->suppress_expr);
		if (pr->suppress_expr)
			pr->suppress = !!ocrpt_expr_get_long(pr->suppress_expr);

		ocrpt_expr_resolve_nowarn(pr->newpage_expr);
		ocrpt_expr_optimize(pr->newpage_expr);
		if (pr->newpage_expr)
			pr->newpage = o->output_functions.supports_page_break && !!ocrpt_expr_get_long(pr->newpage_expr);

		for (ocrpt_list *pdl = pr->pd_list; pdl; pdl = pdl->next) {
			ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

			ocrpt_expr_resolve(pd->width_expr);
			ocrpt_expr_optimize(pd->width_expr);
			if (pd->width_expr)
				pd->width = ocrpt_expr_get_double(pd->width_expr);

			ocrpt_expr_resolve(pd->height_expr);
			ocrpt_expr_optimize(pd->height_expr);
			if (pd->height_expr)
				pd->height = ocrpt_expr_get_double(pd->height_expr);

			ocrpt_expr_resolve(pd->border_width_expr);
			ocrpt_expr_optimize(pd->border_width_expr);
			if (pd->border_width_expr)
				pd->border_width = ocrpt_expr_get_double(pd->border_width_expr);

			ocrpt_expr_resolve_nowarn(pd->border_color_expr);
			ocrpt_expr_optimize(pd->border_color_expr);
			if (pd->border_color_expr) {
				const ocrpt_string *color = ocrpt_expr_get_string(pd->border_color_expr);
				if (color)
					ocrpt_get_color(color->str, &pd->border_color, false);
			}

			ocrpt_expr_resolve(pd->detail_columns_expr);
			ocrpt_expr_optimize(pd->detail_columns_expr);
			pd->detail_columns = 1;
			if (pd->detail_columns_expr) {
				long dc = ocrpt_expr_get_long(pd->detail_columns_expr);
				if (dc > 1)
					pd->detail_columns = dc;
			}
			if (!o->output_functions.supports_column_break)
				pd->detail_columns = 1;

			ocrpt_expr_resolve(pd->column_pad_expr);
			ocrpt_expr_optimize(pd->column_pad_expr);
			if (pd->column_pad_expr)
				pd->column_pad = ocrpt_expr_get_double(pd->column_pad_expr);

			ocrpt_expr_resolve(pd->suppress_expr);
			ocrpt_expr_optimize(pd->suppress_expr);
			if (pd->suppress_expr)
				pd->suppress = !!ocrpt_expr_get_long(pd->suppress_expr);

			for (ocrpt_list *rl = pd->reports; rl; rl = rl->next) {
				ocrpt_report *r = (ocrpt_report *)rl->data;

				ocrpt_expr_resolve_nowarn(r->filename_expr);
				ocrpt_expr_optimize(r->filename_expr);
				if (r->filename_expr) {
					ocrpt_parse_report_node_for_load(r);
					/* Don't load the report again. The expression is freed by ocrpt_free() */
					r->filename_expr = NULL;
				}

				ocrpt_expr_resolve_nowarn(r->query_expr);
				ocrpt_expr_optimize(r->query_expr);
				if (r->query_expr) {
					const ocrpt_string *query = ocrpt_expr_get_string(r->query_expr);
					if (query)
						ocrpt_report_set_main_query(r, ocrpt_query_get(o, query->str));
				} else if (o->queries)
					ocrpt_report_set_main_query(r, (ocrpt_query *)o->queries->data);

				ocrpt_expr_resolve(r->height_expr);
				ocrpt_expr_optimize(r->height_expr);
				if (r->height_expr) {
					double height = ocrpt_expr_get_double(r->height_expr);
					if (height > 0.0)
						r->height = height;
					else {
						/* This does not cause a leak */
						r->height_expr = NULL;
					}
				}

				ocrpt_expr_resolve_nowarn(r->font_name_expr);
				ocrpt_expr_optimize(r->font_name_expr);
				if (r->font_name_expr)
					r->font_name = ocrpt_expr_get_string(r->font_name_expr)->str;

				ocrpt_expr_resolve(r->font_size_expr);
				ocrpt_expr_optimize(r->font_size_expr);
				if (r->font_size_expr) {
					double font_size = ocrpt_expr_get_double(r->font_size_expr);

					if (font_size > 0.0)
						r->font_size = font_size;
					else {
						/* This does not cause a leak */
						r->font_size_expr = NULL;
					}
				}

				ocrpt_expr_resolve(r->iterations_expr);
				ocrpt_expr_optimize(r->iterations_expr);
				r->iterations = 1;
				if (r->iterations_expr) {
					long it = ocrpt_expr_get_long(r->iterations_expr);
					if (it > 1)
						r->iterations = it;
				}

				ocrpt_expr_resolve(r->suppress_expr);
				ocrpt_expr_optimize(r->suppress_expr);
				if (r->suppress_expr)
					r->suppress = !!ocrpt_expr_get_long(r->suppress_expr);

				ocrpt_expr_resolve_nowarn(r->fieldheader_priority_expr);
				ocrpt_expr_optimize(r->fieldheader_priority_expr);
				if (r->fieldheader_priority_expr) {
					const ocrpt_string *fhprio = ocrpt_expr_get_string(r->fieldheader_priority_expr);
					r->fieldheader_high_priority = fhprio && (strcasecmp(fhprio->str, "high") == 0);
				}

				ocrpt_layout_output_evaluate_expr_params(&r->nodata);
				ocrpt_layout_output_evaluate_expr_params(&r->reportheader);
				ocrpt_layout_output_evaluate_expr_params(&r->reportfooter);
				ocrpt_layout_output_evaluate_expr_params(&r->fieldheader);
				ocrpt_layout_output_evaluate_expr_params(&r->fielddetails);

				for (ocrpt_list *brl = r->breaks; brl; brl = brl->next) {
					ocrpt_break *br = (ocrpt_break *)brl->data;

					ocrpt_layout_output_evaluate_expr_params(&br->header);
					ocrpt_layout_output_evaluate_expr_params(&br->footer);
				}

				for (ocrpt_list *vl = r->variables; vl; vl = vl->next) {
					ocrpt_var *var = (ocrpt_var *)vl->data;

					ocrpt_expr_resolve(var->precalculate_expr);
					ocrpt_expr_optimize(var->precalculate_expr);
					if (var->precalculate_expr)
						var->precalculate = !!ocrpt_expr_get_long(var->precalculate_expr);
				}
			}
		}
	}
}

static void ocrpt_execute_parts(opencreport *o) {
	double page_position = 0.0;
	double old_page_position = 0.0;

	if (o->output_functions.set_current_page)
		o->output_functions.set_current_page(o, NULL);
	mpfr_set_ui(o->pageno->number, 1, o->rndmode);

	ocrpt_execute_evaluate_global_params(o);

	for (ocrpt_list *pl = o->parts; pl; pl = pl->next) {
		ocrpt_part *p = (ocrpt_part *)pl->data;

		if (p->suppress)
			continue;

		ocrpt_execute_parts_evaluate_global_params(o, p);

		/*
		 * Resolve reports' breaks, variables and expressions in advance
		 * because <PageHeader>s and <PageFooter>s may contain report
		 * specific variables.
		 */
		ocrpt_execute_parts_resolve_all_reports(o, p);

		p->left_margin_value = ocrpt_layout_left_margin(o, p);
		p->right_margin_value = ocrpt_layout_right_margin(o, p);

		p->page_width = p->paper_width - (p->left_margin_value + p->right_margin_value);

		p->page_header_height = 0.0;
		p->page_footer_height = 0.0;
		ocrpt_layout_output_init(&p->pageheader);
		ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &p->page_header_height);
		ocrpt_layout_output_internal(false, o, p, NULL, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &p->page_header_height);
		ocrpt_layout_output_init(&p->pagefooter);
		ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &p->page_footer_height);
		ocrpt_layout_output_internal(false, o, p, NULL, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &p->page_footer_height);

		for (p->current_iteration = 0; p->current_iteration < p->iterations; p->current_iteration++) {
			if (o->output_functions.start_part && !o->precalculate)
				o->output_functions.start_part(o, p);

			/*
			 * The first <Part>'s first iteration must start on a new page.
			 * Every other iteration of the first part and subsequent
			 * <Part>'s  every iteration must start on a new page if the
			 * output driver supports it.
			 */
			bool newpage = (pl == o->parts && p->current_iteration == 0) || o->output_functions.supports_page_break;

			for (ocrpt_list *row = p->rows; row; row = row->next) {
				ocrpt_part_row *pr = (ocrpt_part_row *)row->data;
				ocrpt_list *pdl;
				double page_indent = ocrpt_layout_left_margin(o, p);

				if (pr->suppress)
					continue;

				if (o->output_functions.start_part_row && !o->precalculate)
					o->output_functions.start_part_row(o, p, pr);

				newpage = newpage || pr->newpage;

				if (newpage)
					pr->start_page = NULL;
				else {
					if (o->output_functions.get_current_page)
						pr->start_page = o->output_functions.get_current_page(o);
				}
				pr->start_page_position = page_position;
				pr->end_page = NULL;
				pr->end_page_position = 0.0;

				uint32_t pds_without_width = 0;
				double pds_total_width = 0;

				for (pdl = pr->pd_list; pdl; pdl = pdl->next) {
					ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

					pd->finished = false;
					if (pd->width_expr) {
						pd->real_width = (o->size_in_points ? 1.0 : p->font_width) * pd->width;
						if (pds_total_width >= p->page_width)
							pd->suppress = true;
						else if (pds_total_width + pd->real_width >= p->page_width)
							pd->real_width = p->page_width - pds_total_width;
						pds_total_width += pd->real_width;
					} else
						pds_without_width++;
				}

				if (pds_without_width > 0) {
					if (p->page_width >= pds_total_width) {
						double pdw = (p->page_width - pds_total_width) / (double)pds_without_width;

						for (pdl = pr->pd_list; pdl; pdl = pdl->next) {
							ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

							if (!pd->width_expr) {
								pd->width = pdw;
								pd->real_width = pdw;
							}
						}
					} else {
						ocrpt_err_printf(
							"ocrpt_execute_parts: <pd> sections with width set exceed page width.\n"
							"ocrpt_execute_parts: Suppressing <pd> sections with no widths set.\n");
						for (pdl = pr->pd_list; pdl; pdl = pdl->next) {
							ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

							if (!pd->width_expr)
								pd->suppress = true;
						}
					}
				}

				for (pdl = pr->pd_list; pdl; pdl = pdl->next) {
					ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

					if (pd->suppress)
						continue;

					if (o->output_functions.start_part_column && !o->precalculate)
						o->output_functions.start_part_column(o, p, pr, pd);

					if (pdl != pr->pd_list) {
						if (o->output_functions.set_current_page)
							o->output_functions.set_current_page(o, pr->start_page);
						page_position = pr->start_page_position;
					}
					pd->start_page_position = pr->start_page_position;

					pd->page_indent0 = pd->page_indent = page_indent;
					if (pd->border_width_expr)
						page_indent += pd->border_width;

					pd->column_width = pd->real_width - (pd->border_width_expr ? 2 * pd->border_width : 0.0);
					if (pd->detail_columns > 1)
						pd->column_width = (pd->column_width - ((o->size_in_points ? pd->column_pad : pd->column_pad * 72.0) * (pd->detail_columns - 1))) / (double)pd->detail_columns;

					pd->current_column = 0;

					if (pd->height_expr) {
						pd->real_height = (o->size_in_points ? 1.0 : p->font_size) * pd->height;
						pd->remaining_height = pd->real_height;
					}

					for (ocrpt_list *rl = pd->reports; rl; rl = rl->next) {
						ocrpt_report *r = (ocrpt_report *)rl->data;
						ocrpt_list *cbl;

						if (r->font_size_expr) {
							if (o->output_functions.set_font_sizes)
								o->output_functions.set_font_sizes(o, r->font_name ? r->font_name : p->font_name, r->font_size, false, false, NULL, &r->font_width);
						} else {
							r->font_size = p->font_size;
							r->font_width = p->font_width;
						}

						if (!o->precalculate) {
							for (cbl = r->start_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}

							for (cbl = o->report_start_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}
						}

						for (r->current_iteration = 0; !r->suppress && (r->current_iteration < r->iterations); r->current_iteration++) {
							if (o->output_functions.start_report && !o->precalculate)
								o->output_functions.start_report(o, p, pr, pd, r);

							r->executing = true;

							r->finished = false;
							if (r->height_expr)
								r->remaining_height = r->height * (o->size_in_points ? 1.0 : (r->font_size_expr ? r->font_size : p->font_size));

							ocrpt_query_navigate_start(r->query);
							if (r->query) {
								r->data_rows = ocrpt_execute_one_report(o, p, pr, pd, r, &newpage, &page_indent, &page_position, &old_page_position);

								if (o->precalculate) {
									ocrpt_variables_advance_precalculated_results(r, NULL);

									/* Reset queries and breaks */
									ocrpt_query_navigate_start(r->query);
									for (ocrpt_list *brl = r->breaks; brl; brl = brl->next)
										ocrpt_break_reset_vars((ocrpt_break *)brl->data);
								}

								if (!r->data_rows) {
									ocrpt_print_reportheader(o, p, pr, pd, r, 0, &newpage, &page_indent, &page_position, &old_page_position);
									ocrpt_layout_output_resolve(&r->nodata);
									ocrpt_layout_output_evaluate(&r->nodata);
									ocrpt_layout_output(o, p, pr, pd, r, NULL, &r->nodata, 0, &newpage, &page_indent, &page_position, &old_page_position);
								}
							} else {
								ocrpt_print_reportheader(o, p, pr, pd, r, 0, &newpage, &page_indent, &page_position, &old_page_position);
								if ((o->noquery_show_nodata_expr && o->noquery_show_nodata) || (!o->noquery_show_nodata_expr && r->noquery_show_nodata)) {
									ocrpt_layout_output_resolve(&r->nodata);
									ocrpt_layout_output_evaluate(&r->nodata);
									ocrpt_layout_output(o, p, pr, pd, r, NULL, &r->nodata, 0, &newpage, &page_indent, &page_position, &old_page_position);
								}
							}
							if (o->output_functions.end_report && !o->precalculate)
								o->output_functions.end_report(o, p, pr, pd, r);

							if (r->height_expr) {
								if (rl->next || o->report_height_after_last) {
									page_position += r->remaining_height;

									if (pd->max_page_position < page_position)
										pd->max_page_position = page_position;
								}
							}

							r->executing = false;

							if (!o->precalculate) {
								for (cbl = r->iteration_callbacks; cbl; cbl = cbl->next) {
									ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

									cbd->func(o, r, cbd->data);
								}

								for (cbl = o->report_iteration_callbacks; cbl; cbl = cbl->next) {
									ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

									cbd->func(o, r, cbd->data);
								}
							}
						}

						if (o->precalculate) {
							ocrpt_list *cbl;
							for (cbl = r->precalc_done_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}

							for (cbl = o->report_precalc_done_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}
						} else {
							for (cbl = r->done_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}

							for (cbl = o->report_done_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}
						}
					}

					if (!o->precalculate && pd->border_width_expr && o->output_functions.draw_rectangle) {
						double bx, by, bw, bh;
						double page_end_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height - pd->border_width;

						bx = pd->page_indent0;
						by = pd->start_page_position;
						bw = pd->real_width;

						if (pd->height_expr) {
							/*
							 * Rely on the precalculation phase to calculate the same
							 * visual data but not draw anything yet.
							 */
							double max_end_position = 0.0;
							for (ocrpt_list *pdl1 = pr->pd_list; pdl1; pdl1 = pdl1->next) {
								ocrpt_part_column *pd1 = (ocrpt_part_column *)pdl1->data;

								if (max_end_position < pd1->max_page_position)
									max_end_position = pd1->max_page_position;
							}

							if (pd->finished) {
								if (pd->start_page_position + pd->remaining_height < page_end_position) {
									bh = pd->remaining_height;
								} else {
									bh = page_end_position - pd->start_page_position;
								}
							} else
								bh = max_end_position - pd->start_page_position;
						} else {
							bh = pd->max_page_position - pd->start_page_position;
						}

						if (pd->border_width_expr) {
							bx += 0.5 * pd->border_width;
							by += 0.5 * pd->border_width;
							bw -= pd->border_width;
						}

						o->output_functions.draw_rectangle(o, p, pr, pd, NULL,
														&pd->border_color, pd->border_width,
														bx, by, bw, bh);
					}

					if (pd->height_expr && pd->finished) {
						page_position = pd->start_page_position + pd->remaining_height;
						if (pd->border_width_expr)
							page_position += pd->border_width;
					} else if (pd->border_width_expr)
						page_position += pd->border_width;

					if (!pr->end_page) {
						if (o->output_functions.get_current_page)
							pr->end_page = o->output_functions.get_current_page(o);
						pr->end_page_position = page_position;
					} else {
						void *currpage = NULL;
						if (o->output_functions.get_current_page)
							currpage = o->output_functions.get_current_page(o);
						if (pr->end_page == currpage) {
							if (pr->end_page_position < page_position)
								pr->end_page_position = page_position;
						} else {
							ocrpt_list *pagel;
							bool earlier = false;

							for (pagel = pr->start_page; pagel && pagel != pr->end_page; pagel = pagel->next) {
								if (pagel == currpage) {
									earlier = true;
									break;
								}
							}
							if (!earlier) {
								pr->end_page = currpage;
								pr->end_page_position = page_position;
							}
						}
					}

					page_indent = pd->page_indent;
					page_indent += pd->real_width;

					if (o->output_functions.end_part_column && !o->precalculate)
						o->output_functions.end_part_column(o, p, pr, pd);
				}

				if (o->output_functions.end_part_row && !o->precalculate)
					o->output_functions.end_part_row(o, p, pr);

				if (o->output_functions.set_current_page)
					o->output_functions.set_current_page(o, pr->end_page);

				page_position = pr->end_page_position;
			}

			if (!o->precalculate) {
				ocrpt_list *cbl;

				for (cbl = p->iteration_callbacks; cbl; cbl = cbl->next) {
					ocrpt_part_cb_data *cbd = (ocrpt_part_cb_data *)cbl->data;

					cbd->func(o, p, cbd->data);
				}

				for (cbl = o->part_iteration_callbacks; cbl; cbl = cbl->next) {
					ocrpt_part_cb_data *cbd = (ocrpt_part_cb_data *)cbl->data;

					cbd->func(o, p, cbd->data);
				}

				if (o->output_functions.reopen_tags_across_pages && !o->precalculate) {
					if (o->output_functions.start_part_row)
						o->output_functions.start_part_row(o, p, NULL);

					if (o->output_functions.start_part_column)
						o->output_functions.start_part_column(o, p, NULL, NULL);
				}

				if (!o->precalculate && p->pageheader.output_list && o->output_functions.start_output)
					o->output_functions.start_output(o, p, NULL, NULL, NULL, NULL, &p->pagefooter);

				/* Use the previous row data temporarily */
				o->residx = ocrpt_expr_prev_residx(o->residx);

				page_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
				ocrpt_layout_output_evaluate(&p->pagefooter);
				ocrpt_layout_output_init(&p->pagefooter);
				ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &page_position);
				ocrpt_layout_output_internal(true, o, p, NULL, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &page_position);

				/* Switch back to the current row data */
				o->residx = ocrpt_expr_next_residx(o->residx);

				if (!o->precalculate && p->pageheader.output_list && o->output_functions.end_output)
					o->output_functions.end_output(o, p, NULL, NULL, NULL, NULL, &p->pagefooter);

				if (o->output_functions.reopen_tags_across_pages && !o->precalculate) {
					if (o->output_functions.end_part_column)
						o->output_functions.end_part_column(o, p, NULL, NULL);

					if (o->output_functions.end_part_row)
						o->output_functions.end_part_row(o, p, NULL);
				}
			}

			if (o->output_functions.end_part && !o->precalculate)
				o->output_functions.end_part(o, p);
		}
	}
}

DLL_EXPORT_SYM bool ocrpt_execute(opencreport *o) {
	if (!o)
		return false;

	if (o->output_buffer)
		return true;

	switch (o->output_format) {
	case OCRPT_OUTPUT_PDF:
		ocrpt_pdf_init(o);
		break;
	case OCRPT_OUTPUT_HTML:
		ocrpt_html_init(o);
		break;
	case OCRPT_OUTPUT_TXT:
		ocrpt_txt_init(o);
		break;
	case OCRPT_OUTPUT_CSV:
		ocrpt_csv_init(o);
		break;
	case OCRPT_OUTPUT_XML:
		ocrpt_xml_init(o);
		break;
	case OCRPT_OUTPUT_JSON:
		ocrpt_json_init(o);
		break;
	default:
		ocrpt_err_printf("invalid output format: %d\n", o->output_format);
		return false;
	}

	/* Initialize date() and now() values */
	time_t t = time(NULL);
	localtime_r(&t, &o->current_date->datetime);
	o->current_date->date_valid = true;
	localtime_r(&t, &o->current_timestamp->datetime);
	o->current_timestamp->date_valid = true;
	o->current_timestamp->time_valid = true;

	/* Set global default font sizes */
	if (o->output_functions.set_font_sizes)
		o->output_functions.set_font_sizes(o, "Courier", OCRPT_DEFAULT_FONT_SIZE, false, false, &o->font_size, &o->font_width);

	/* Run all reports in precalculate mode if needed */
	o->precalculate = true;
	ocrpt_execute_parts(o);

	for (ocrpt_list *cbl = o->precalc_done_callbacks; cbl; cbl = cbl->next) {
		ocrpt_cb_data *cbd = (ocrpt_cb_data *)cbl->data;

		cbd->func(o, cbd->data);
	}

	/* Run all reports in layout mode */
	o->precalculate = false;
	if (o->output_functions.reset_state)
		o->output_functions.reset_state(o);
	ocrpt_execute_parts(o);

	if (o->output_functions.finalize)
		o->output_functions.finalize(o);

	return true;
}

DLL_EXPORT_SYM const char *ocrpt_get_output(opencreport *o, size_t *length) {
	if (!o || !o->output_buffer) {
		if (length)
			*length = 0;
		return NULL;
	}

	if (length)
		*length = o->output_buffer->len;

	return o->output_buffer->str;
}

DLL_EXPORT_SYM const ocrpt_string **ocrpt_get_content_type(opencreport *o) {
	if (!o || !o->content_type)
		return NULL;

	return o->content_type;
}

DLL_EXPORT_SYM void ocrpt_spool(opencreport *o) {
	if (!o || !o->output_buffer)
		return;

	ssize_t ret = write(fileno(stdout), o->output_buffer->str, o->output_buffer->len);
	assert(ret == o->output_buffer->len);
}

DLL_EXPORT_SYM void ocrpt_set_output_format(opencreport *o, ocrpt_format_type format) {
	if (!o)
		return;
	o->output_format = format;
	o->rptformat->string->len = 0;
	switch (format) {
	case OCRPT_OUTPUT_PDF:
		ocrpt_mem_string_append_printf(o->rptformat->string, "PDF");
		break;
	case OCRPT_OUTPUT_HTML:
		ocrpt_mem_string_append_printf(o->rptformat->string, "HTML");
		break;
	case OCRPT_OUTPUT_TXT:
		ocrpt_mem_string_append_printf(o->rptformat->string, "TXT");
		break;
	case OCRPT_OUTPUT_CSV:
		ocrpt_mem_string_append_printf(o->rptformat->string, "CSV");
		break;
	case OCRPT_OUTPUT_XML:
		ocrpt_mem_string_append_printf(o->rptformat->string, "XML");
		break;
	case OCRPT_OUTPUT_JSON:
		ocrpt_mem_string_append_printf(o->rptformat->string, "JSON");
		break;
	}
}

DLL_EXPORT_SYM char *ocrpt_canonicalize_path(const char *path) {
	bool free_path = false;

	again:

	if (!path)
		return NULL;

	int len0 = strlen(path);
	if (!len0)
		return NULL;

	char *path_copy = NULL;
	int len = 0;
	if (*path != '/') {
		char *cwd = ocrpt_mem_malloc(PATH_MAX);

		if (!getcwd(cwd, PATH_MAX)) {
			ocrpt_mem_free(cwd);
			return NULL;
		}

		path_copy = ocrpt_mem_malloc(strlen(cwd) + len0 + 2);
		if (!path_copy) {
			ocrpt_mem_free(cwd);
			return NULL;
		}

		len = sprintf(path_copy, "%s/", cwd);
		ocrpt_mem_free(cwd);
	} else {
		path_copy = ocrpt_mem_malloc(len0 + 1);
		if (!path_copy)
			return NULL;
	}

	int i;
	for (i = 0; i < len0; i++, len++) {
		char c = path[i];
#ifdef _WIN32
		if (c == '\\')
			c = '/';
		while (i < len0 - 1 && c == '/' && (path[i + 1] == '/' || path[i + 1] == '\\'))
#else
		while (i < len0 - 1 && c == '/' && path[i + 1] == '/')
#endif
			i++;

		path_copy[len] = c;
	}

	if (!len) {
		ocrpt_mem_free(path_copy);
		return NULL;
	}

	if (path_copy[len - 1] == '/')
		len--;
	path_copy[len] = '\0';

	ocrpt_list *path_elements = NULL, *last_element = NULL;
	char *c = path_copy + 1;
	bool dotdot = false;

	while (c) {
		char *nc = strchr(c + 1, '/');
		ssize_t elem_len = nc ? nc - c: strlen(c);
		char *c1 = ocrpt_mem_malloc(elem_len + 1);
		strncpy(c1, c, elem_len);
		c1[elem_len] = '\0';

		/* Leave out path elements referencing the current directory */
		if (dotdot) {
			if (last_element) {
				ocrpt_mem_free(last_element->data);
				last_element->data = c1;
			}
			dotdot = false;
		} else if (!strcmp(c1, "."))
			ocrpt_mem_free(c1);
		else if (!strcmp(c1, "..")) {
			dotdot = true;
			ocrpt_mem_free(c1);
		} else
			path_elements = ocrpt_list_end_append(path_elements, &last_element, c1);
		c = nc ? nc + 1 : NULL;
	}

	ocrpt_mem_free(path_copy);

	ocrpt_string *result = ocrpt_mem_string_new(NULL, true);
	bool valid = true;

	for (ocrpt_list *l = path_elements; l; l = l->next) {
		ocrpt_mem_string_append_printf(result, "/%s", (char *)l->data);
		struct stat st;

		if (valid) {
			if (stat(result->str, &st))
				valid = false;
		}

		if (valid) {
			if (!lstat(result->str, &st)) {
				if (S_ISLNK(st.st_mode)) {
					ocrpt_string *link = ocrpt_mem_string_new_with_len(NULL, result->len);
					ssize_t linksz = readlink(result->str, link->str, link->allocated_len);
					if (linksz > link->allocated_len) {
						ocrpt_mem_string_resize(link, linksz);
						linksz = readlink(result->str, link->str, link->allocated_len);
						assert(linksz <= link->allocated_len);
					}
					link->str[linksz] = '\0';

					result->len = 0;
					if (link->str[0] == '/') {
						ocrpt_mem_string_append_printf(result, "%s", link->str);
					} else {
						for (ocrpt_list *ll = path_elements; ll && ll != l; ll = ll->next)
							ocrpt_mem_string_append_printf(result, "/%s", (char *)ll->data);

						ocrpt_mem_string_append_printf(result, "/%s", link->str);
					}
					for (l = l->next; l; l = l->next)
						ocrpt_mem_string_append_printf(result, "/%s", (char *)l->data);

					ocrpt_mem_string_free(link, true);

					if (free_path)
						ocrpt_mem_free(path);

					path = ocrpt_mem_string_free(result, false);
					ocrpt_list_free_deep(path_elements, ocrpt_mem_free);
					free_path = true;
					goto again;
				}
			} else
				valid = false;
		}
	}

	ocrpt_list_free_deep(path_elements, ocrpt_mem_free);

	if (free_path)
		ocrpt_mem_free(path);

	return ocrpt_mem_string_free(result, false);
}

DLL_EXPORT_SYM void ocrpt_add_search_path(opencreport *o, const char *path) {
	if (!o || !path)
		return;

	char *cpath = ocrpt_canonicalize_path(path);

	if (!cpath)
		return;

	bool found = false;
	for (ocrpt_list *spl = o->search_paths; spl; spl = spl->next) {
		ocrpt_search_path *p = (ocrpt_search_path *)spl->data;
		if (p->path && !strcmp(cpath, p->path)) {
			found = true;
			break;
		}
	}

	if (found)
		ocrpt_mem_free(cpath);
	else {
		ocrpt_search_path *p = ocrpt_mem_malloc(sizeof(ocrpt_search_path));
		p->path = cpath;
		p->expr = NULL;
		o->search_paths = ocrpt_list_append(o->search_paths, p);
	}
}

DLL_EXPORT_SYM void ocrpt_add_search_path_from_expr(opencreport *o, const char *expr_string) {
	if (!o || !expr_string)
		return;

	ocrpt_expr *expr = ocrpt_expr_parse(o, expr_string, NULL);
	char *path = NULL;
	if (!expr)
		path = ocrpt_mem_strdup(expr_string);

	ocrpt_search_path *p = ocrpt_mem_malloc(sizeof(ocrpt_search_path));
	p->path = path;
	p->expr = expr;
	o->search_paths = ocrpt_list_append(o->search_paths, p);
}

DLL_EXPORT_SYM void ocrpt_resolve_search_paths(opencreport *o) {
	for (ocrpt_list *l = o->search_paths; l; l = l->next) {
		ocrpt_search_path *p = (ocrpt_search_path *)l->data;

		if (p->expr && !p->path) {
			ocrpt_expr_resolve(p->expr);
			ocrpt_expr_optimize(p->expr);
			const ocrpt_string *s = ocrpt_expr_get_string(p->expr);
			if (s)
				p->path = ocrpt_mem_strdup(s->str);
		}
	}
}

DLL_EXPORT_SYM char *ocrpt_find_file(opencreport *o, const char *filename) {
	if (!o || !filename)
		return NULL;

	struct stat st;

	if (*filename == '/') {
		if (stat(filename, &st) == 0 && S_ISREG(st.st_mode))
			return ocrpt_mem_strdup(filename);
		return NULL;
	}

	for (ocrpt_list *l = o->search_paths; l; l = l->next) {
		ocrpt_search_path *p = (ocrpt_search_path *)l->data;
		ocrpt_string *s = ocrpt_mem_string_new(p->path, true);

		ocrpt_mem_string_append_printf(s, "/%s", filename);

		char *file = ocrpt_canonicalize_path(s->str);

		if (stat(file, &st) == 0 && S_ISREG(st.st_mode)) {
			ocrpt_mem_string_free(s, true);
			return file;
		}
		ocrpt_mem_string_free(s, true);
		ocrpt_mem_free(file);
	}

	char *cwd = ocrpt_mem_malloc(PATH_MAX);

	if (!getcwd(cwd, PATH_MAX)) {
		ocrpt_mem_free(cwd);
		return NULL;
	}

	ocrpt_string *s = ocrpt_mem_string_new(cwd, false);
	ocrpt_mem_string_append_printf(s, "/%s", filename);

	char *file = ocrpt_canonicalize_path(s->str);

	if (stat(file, &st) == 0 && S_ISREG(st.st_mode)) {
		ocrpt_mem_string_free(s, true);
		return file;
	}
	ocrpt_mem_string_free(s, true);
	ocrpt_mem_free(file);

	return NULL;
}

DLL_EXPORT_SYM void ocrpt_set_output_parameter(opencreport *o, const char *param, const char *value) {
	if (!o)
		return;

	/* HTML output parameters */
	if (strcmp(param, "suppress_head") == 0)
		o->suppress_html_head = strcasecmp(value, "yes") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "on") == 0 || atoi(value) > 0;
	else if (strcmp(param, "document_root") == 0) {
		ocrpt_mem_free(o->html_docroot);
		o->html_docroot = ocrpt_mem_strdup(o->html_docroot);
	} else if (strcmp(param, "meta") == 0) {
		/*
		 * Parse the value:
		 * - allow a complete <meta ...> syntax with or the closing '>'
		 * - swallow the charset="..." specification in it
		 *   as the HTML output driver uses utf-8
		 */
		ocrpt_mem_free(o->html_meta);
		o->html_meta = NULL;

		ocrpt_string *meta;
		ocrpt_string *html_meta = ocrpt_mem_string_new("", true);

		char *meta0 = strstr(value, "<meta");
		if (meta0) {
			meta = ocrpt_mem_string_new(meta0 + 5, true);

			/* Remove the last '>' character, ignore it if it doesn't exist. */
			char *closegt = strrchr(meta->str, '>');
			if (closegt)
				*closegt = 0;
		} else
			meta = ocrpt_mem_string_new(value, true);

		char *charset = strstr(meta->str, "charset=\"");
		char *charsetend = NULL;
		if (charset) {
			charsetend = strchr(charset + 9, '"');
			*charset = 0;
		}

		ocrpt_mem_string_append(html_meta, meta->str);
		if (charsetend)
			ocrpt_mem_string_append(html_meta, charsetend + 1);

		o->html_meta = ocrpt_mem_string_free(html_meta, false);
		ocrpt_mem_string_free(meta, true);
	} else if (strcmp(param, "csv_file_name") == 0 || strcmp(param, "file_name") == 0) {
		ocrpt_mem_free(o->csv_filename);
		o->csv_filename = ocrpt_mem_strdup(value);
	} else if (strcmp(param, "csv_delimiter") == 0 || (strcmp(param, "csv_delimeter") == 0)) {
		utf8proc_int32_t c;
		utf8proc_ssize_t bytes_read = utf8proc_iterate((utf8proc_uint8_t *)value, strlen(value), &c);

		ocrpt_mem_free(o->csv_delimiter);
		o->csv_delimiter = ocrpt_mem_malloc(bytes_read + 1);
		memcpy(o->csv_delimiter, value, bytes_read);
		o->csv_delimiter[bytes_read] = 0;
	} else if (strcmp(param, "csv_as_text") == 0)
		o->csv_as_text = strcasecmp(value, "yes") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "on") == 0 || atoi(value) > 0;
	else if (strcmp(param, "no_quotes") == 0)
		o->no_quotes = strcasecmp(value, "yes") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "on") == 0 || atoi(value) > 0;
	else if (strcmp(param, "only_quote_strings") == 0)
		o->only_quote_strings = strcasecmp(value, "yes") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "on") == 0 || atoi(value) > 0;
	else if (strcmp(param, "xml_rlib_compat") == 0)
		o->xml_rlib_compat = strcasecmp(value, "yes") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "on") == 0 || atoi(value) > 0;
}

static int papersortcmp(const void *a, const void *b) {
	return strcasecmp(((ocrpt_paper *)a)->name, ((ocrpt_paper *)b)->name);
}

static int paperfindcmp(const void *key, const void *a) {
	return strcasecmp(key, ((ocrpt_paper *)a)->name);
}

DLL_EXPORT_SYM const ocrpt_paper *ocrpt_get_paper_by_name(const char *paper) {
	if (!paper)
		return NULL;

	return bsearch(paper, papersizes, n_papersizes, sizeof(ocrpt_paper), paperfindcmp);
}

DLL_EXPORT_SYM const ocrpt_paper *ocrpt_get_system_paper(void) {
	return system_paper;
}

DLL_EXPORT_SYM const ocrpt_paper *ocrpt_get_paper(opencreport *o) {
	if (!o)
		return ocrpt_get_system_paper();

	return o->paper;
}

DLL_EXPORT_SYM void ocrpt_set_paper(opencreport *o, const ocrpt_paper *paper) {
	if (!o)
		return;

	o->paper0 = *paper;
	o->paper = &o->paper0;
}

DLL_EXPORT_SYM void ocrpt_set_paper_by_name(opencreport *o, const char *papername) {
	if (!o)
		return;

	const ocrpt_paper *paper = ocrpt_get_paper_by_name(papername);

	if (!paper)
		paper = system_paper;

	o->paper = paper;
}

DLL_EXPORT_SYM const ocrpt_paper *ocrpt_paper_next(opencreport *o, void **iter) {
	if (!o || !iter)
		return NULL;

	size_t idx = (size_t)*iter;
	ocrpt_paper *p = (idx >= n_papersizes ? NULL : &papersizes[idx]);
	*iter = (void *)(idx + 1);

	return p;
}

DLL_EXPORT_SYM void ocrpt_bindtextdomain(opencreport *o, const char *domainname, const char *dirname) {
	if (!o || !domainname || !dirname)
		return;

	ocrpt_strfree(o->textdomain);
	bindtextdomain(domainname, dirname);
	bind_textdomain_codeset(domainname, "UTF-8");
	o->textdomain = ocrpt_mem_strdup(domainname);
}

DLL_EXPORT_SYM void ocrpt_bindtextdomain_from_expr(opencreport *o, const char *domainname, const char *dirname) {
	if (!o || !domainname || !dirname)
		return;

	ocrpt_strfree(o->xlate_domain_s);
	ocrpt_strfree(o->xlate_dir_s);

	o->xlate_domain_s = ocrpt_mem_strdup(domainname);
	o->xlate_dir_s = ocrpt_mem_strdup(dirname);
}

DLL_EXPORT_SYM void ocrpt_set_locale(opencreport *o, const char *locale) {
	if (!o || !locale)
		return;

	if (o->locale != o->c_locale)
		freelocale(o->locale);
	o->locale = newlocale(LC_ALL_MASK, locale, (locale_t)0);
	if (o->locale == (locale_t)0)
		o->locale = o->c_locale;
}

DLL_EXPORT_SYM void ocrpt_set_locale_from_expr(opencreport *o, const char *expr_string) {
	if (!o)
		return;

	ocrpt_expr_free(o->locale_expr);
	o->locale_expr = NULL;
	if (expr_string) {
		char *err = NULL;
		o->locale_expr = ocrpt_expr_parse(o, expr_string, &err);
		if (err) {
			ocrpt_err_printf("ocrpt_set_rounding_mode: %s\n", err);
			ocrpt_strfree(err);
		}
	}
}

DLL_EXPORT_SYM locale_t ocrpt_get_locale(opencreport *o) {
	return o ? o->locale : (locale_t)0;
}

__attribute__((constructor))
static void initialize_ocrpt(void) {
	char *cwd;
	const char *system_paper_name;
	const struct paper *paper_info;
	int i;

	/* Initialize time utilities */
	tzset();

	/* Use the OpenCReport allocator functions in GMP/MPFR */
	mpfr_mp_memory_cleanup();
	mp_set_memory_functions(ocrpt_mem_malloc0, ocrpt_mem_reallocarray0, (ocrpt_mem_free_size_t)ocrpt_mem_free0);

	/*
	 * When we add a toplevel report part from buffer,
	 * we can't rely on file path, we can only rely
	 * on the application's current working directory.
	 */
	cwd = getcwd(cwdpath, sizeof(cwdpath));

	if (cwd == NULL)
		cwdpath[0] = 0;

	paperinit();

	i = 0;
	paper_info = paperfirst();
	while (paper_info) {
		paper_info = papernext(paper_info);
		i++;
	}

	n_papersizes = i;
	/* Cannot use ocrpt_mem_malloc() here and in the destructor */
	papersizes = malloc(i * sizeof(ocrpt_paper));

	for (i = 0, paper_info = paperfirst(); paper_info; i++, paper_info = papernext(paper_info)) {
		papersizes[i].name = strdup(papername(paper_info));
		papersizes[i].width = paperpswidth(paper_info);
		papersizes[i].height = paperpsheight(paper_info);
	}

	qsort(papersizes, n_papersizes, sizeof(ocrpt_paper), papersortcmp);

	system_paper_name = systempapername();
	system_paper = ocrpt_get_paper_by_name(system_paper_name);
	free((void *)system_paper_name);

	paperdone();

	ocrpt_init_color();

	ocrpt_input_register(&ocrpt_array_input);
	ocrpt_input_register(&ocrpt_csv_input);
	ocrpt_input_register(&ocrpt_xml_input);
	ocrpt_input_register(&ocrpt_json_input);
	ocrpt_input_register(&ocrpt_mariadb_input);
	ocrpt_input_register(&ocrpt_postgresql_input);
	ocrpt_input_register(&ocrpt_odbc_input);

	LIBXML_TEST_VERSION;
	xmlInitParser();
}

__attribute__((destructor))
static void uninitialize_ocrpt(void) {
	int i;

	for (i = 0; i < n_papersizes; i++)
		free((void *)papersizes[i].name);
	free(papersizes);

	xmlCleanupParser();
}
