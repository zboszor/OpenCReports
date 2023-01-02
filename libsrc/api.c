/*
 * OpenCReports main module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
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
#include "color.h"
#include "layout.h"
#include "pdf.h"

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

mpfr_prec_t global_prec = OCRPT_MPFR_PRECISION_BITS;
mpfr_rnd_t global_rndmode = MPFR_RNDN;

char cwdpath[PATH_MAX];
static ocrpt_paper *papersizes;
static const ocrpt_paper *system_paper;
int n_papersizes;

DLL_EXPORT_SYM const char *ocrpt_version(void) {
	return VERSION;
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

	o->paper = system_paper;
	o->prec = OCRPT_MPFR_PRECISION_BITS;
	o->rndmode = MPFR_RNDN;
	gmp_randinit_default(o->randstate);
	gmp_randseed_ui(o->randstate, seed);
	o->locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
	o->noquery_show_nodata = true;

	o->current_date = ocrpt_mem_malloc(sizeof(ocrpt_result));
	memset(o->current_date, 0, sizeof(ocrpt_result));
	o->current_date->type = OCRPT_RESULT_DATETIME;

	o->current_timestamp = ocrpt_mem_malloc(sizeof(ocrpt_result));
	memset(o->current_timestamp, 0, sizeof(ocrpt_result));
	o->current_timestamp->type = OCRPT_RESULT_DATETIME;

	o->pageno = ocrpt_mem_malloc(sizeof(ocrpt_result));
	memset(o->pageno, 0, sizeof(ocrpt_result));
	o->pageno->type = OCRPT_RESULT_NUMBER;
	mpfr_init2(o->pageno->number, o->prec);
	o->pageno->number_initialized = true;
	mpfr_set_ui(o->pageno->number, 1, o->rndmode);

	o->totpages = ocrpt_mem_malloc(sizeof(ocrpt_result));
	memset(o->totpages, 0, sizeof(ocrpt_result));
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

DLL_EXPORT_SYM void ocrpt_free(opencreport *o) {
	if (!o)
		return;

	for (int32_t i = 0; i < o->n_functions; i++) {
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

	if (o->locale)
		freelocale(o->locale);

	ocrpt_list_free_deep(o->part_added_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->report_added_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->precalc_done_callbacks, ocrpt_mem_free);

	ocrpt_list_free_deep(o->search_paths, ocrpt_mem_free);
	ocrpt_list_free_deep(o->images, ocrpt_image_free);
	ocrpt_list_free_deep(o->pages, (ocrpt_mem_free_t)cairo_surface_destroy);

	gmp_randclear(o->randstate);
	ocrpt_mem_string_free(o->converted, true);

	ocrpt_result_free(o->current_date);
	ocrpt_result_free(o->current_timestamp);
	ocrpt_result_free(o->pageno);
	ocrpt_result_free(o->totpages);

	ocrpt_mem_string_free(o->output_buffer, true);
	ocrpt_mem_free(o->textdomain);

	cairo_destroy(o->cr);
	cairo_surface_destroy(o->nullpage_cs);

	ocrpt_mem_free(o);
}

DLL_EXPORT_SYM void ocrpt_set_numeric_precision_bits(opencreport *o, mpfr_prec_t prec) {
	global_prec = prec;
	if (!o)
		return;

	o->prec = prec;
	o->precision_set = true;
}

DLL_EXPORT_SYM void ocrpt_set_rounding_mode(opencreport *o, mpfr_rnd_t rndmode) {
	global_rndmode = rndmode;
	if (!o)
		return;

	o->rndmode = rndmode;
	o->rounding_mode_set = true;
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
		pr->start_page = o->current_page;
		pr->start_page_position = *page_position;
		pd->start_page_position = *page_position;
	}
	if (pd->border_width_set)
		*page_position += pd->border_width;
	ocrpt_layout_output_init(&r->reportheader);
	ocrpt_layout_output(o, p, pr, pd, r, &r->reportheader, rows, newpage, page_indent, page_position, old_page_position);
}

static unsigned int ocrpt_execute_one_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	ocrpt_list *brl_start = NULL;
	unsigned int rows = 0;
	bool have_row;

	ocrpt_query_navigate_start(r->query);
	have_row = ocrpt_query_navigate_next(r->query);

	ocrpt_expr_init_iterative_results(r->detailcnt, OCRPT_RESULT_NUMBER);

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
			}
		}

		if (brl_start) {
			if (rows > 1) {
				/* Use the previous row data temporarily */
				o->residx = ocrpt_expr_prev_residx(o->residx);

				if (o->precalculate && !last_row)
					ocrpt_variables_add_precalculated_results(r, brl_start, last_row);

				/* Switch back to the current row data */
				o->residx = ocrpt_expr_next_residx(o->residx);

				for (brl = brl_start; brl; brl = brl->next)
					ocrpt_break_reset_vars((ocrpt_break *)brl->data);

				if (!o->precalculate && !last_row)
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
			ocrpt_layout_output(o, p, pr, pd, r, &r->fieldheader, rows, newpage, page_indent, page_position, old_page_position);
		}

		if (rows > 1 && brl_start) {
			/* Use the previous row data temporarily */
			o->residx = ocrpt_expr_prev_residx(o->residx);

			for (brl = r->breaks_reverse; brl; brl = brl->next) {
				ocrpt_break *br = (ocrpt_break *)brl->data;

				ocrpt_layout_output_init(&br->footer);
				ocrpt_layout_output(o, p, pr, pd, r, &br->footer, rows, newpage, page_indent, page_position, old_page_position);

				if (br == brl_start->data)
					break;
			}

			/* Switch back to the current row data */
			o->residx = ocrpt_expr_next_residx(o->residx);
		}

		for (brl = (rows == 1 ? r->breaks : brl_start); brl; brl = brl->next) {
			ocrpt_break *br __attribute__((unused)) = (ocrpt_break *)brl->data;

			if (br->cb_triggered) {
				ocrpt_layout_output_init(&br->header);
				ocrpt_layout_output(o, p, pr, pd, r, &br->header, rows, newpage, page_indent, page_position, old_page_position);
			}
		}

		if (!o->precalculate) {
			for (ocrpt_list *cbl = r->newrow_callbacks; cbl; cbl = cbl->next) {
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
			ocrpt_layout_output(o, p, pr, pd, r, &r->fieldheader, rows, newpage, page_indent, page_position, old_page_position);
		}
		ocrpt_layout_output_init(&r->fielddetails);
		ocrpt_layout_output(o, p, pr, pd, r, &r->fielddetails, rows, newpage, page_indent, page_position, old_page_position);

		if (o->precalculate && last_row) {
			ocrpt_variables_add_precalculated_results(r, r->breaks, last_row);
			ocrpt_report_expressions_add_delayed_results(r);
		}

		have_row = !last_row;
		o->residx = ocrpt_expr_next_residx(o->residx);
	}

	if (rows) {
		/* Use the previous row data temporarily */
		o->residx = ocrpt_expr_prev_residx(o->residx);

		for (ocrpt_list *brl = r->breaks_reverse; brl; brl = brl->next) {
			ocrpt_break *br = (ocrpt_break *)brl->data;

			ocrpt_layout_output_init(&br->footer);
			ocrpt_layout_output(o, p, pr, pd, r, &br->footer, rows, newpage, page_indent, page_position, old_page_position);
		}

		ocrpt_layout_output_init(&r->reportfooter);
		ocrpt_layout_output(o, p, pr, pd, r, &r->reportfooter, rows, newpage, page_indent, page_position, old_page_position);

		/* Switch back to the current row data */
		o->residx = ocrpt_expr_next_residx(o->residx);
	}

	return rows;
}

static void ocrpt_execute_parts(opencreport *o) {
	double page_position = 0.0;
	double old_page_position = 0.0;

	o->current_page = NULL;
	mpfr_set_ui(o->pageno->number, 1, o->rndmode);

	for (ocrpt_list *pl = o->parts; pl; pl = pl->next) {
		ocrpt_part *p = (ocrpt_part *)pl->data;

		if (p->suppress)
			continue;

		if (!p->paper)
			p->paper = o->paper;

		if (p->orientation_set && p->landscape) {
			p->paper_width = p->paper->height;
			p->paper_height = p->paper->width;
		} else {
			p->paper_width = p->paper->width;
			p->paper_height = p->paper->height;
		}

		ocrpt_layout_output_resolve(o, p, NULL, &p->pageheader);
		ocrpt_layout_output_resolve(o, p, NULL, &p->pagefooter);

		if (p->font_size_set)
			ocrpt_layout_set_font_sizes(o, p->font_name ? p->font_name : "Courier", p->font_size, false, false, NULL, &p->font_width);
		else {
			p->font_size = o->font_size;
			p->font_width = o->font_width;
		}

		p->left_margin_value = ocrpt_layout_left_margin(o, p);
		p->right_margin_value = ocrpt_layout_right_margin(o, p);

		p->page_width = p->paper_width - (p->left_margin_value + p->right_margin_value);

		p->page_header_height = 0.0;
		p->page_footer_height = 0.0;
		ocrpt_layout_output_init(&p->pageheader);
		ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &p->page_header_height);
		ocrpt_layout_output_internal(false, o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &p->page_header_height);
		ocrpt_layout_output_init(&p->pagefooter);
		ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &p->page_footer_height);
		ocrpt_layout_output_internal(false, o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &p->page_footer_height);

		for (p->current_iteration = 0; p->current_iteration < p->iterations; p->current_iteration++) {
			bool newpage = true; /* <Part>'s every iteration must start on a new page */

			for (ocrpt_list *row = p->rows; row; row = row->next) {
				ocrpt_part_row *pr = (ocrpt_part_row *)row->data;
				ocrpt_list *pdl;
				double page_indent = ocrpt_layout_left_margin(o, p);

				if (pr->suppress)
					continue;

				newpage = newpage || pr->newpage;

				if (newpage)
					pr->start_page = NULL;
				else
					pr->start_page = o->current_page;
				pr->start_page_position = page_position;
				pr->end_page = NULL;
				pr->end_page_position = 0.0;

				uint32_t pds_without_width = 0;
				double pds_total_width = 0;

				for (pdl = pr->pd_list; pdl; pdl = pdl->next) {
					ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

					pd->finished = false;
					if (pd->width_set) {
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

							if (!pd->width_set) {
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

							if (!pd->width_set)
								pd->suppress = true;
						}
					}
				}

				for (pdl = pr->pd_list; pdl; pdl = pdl->next) {
					ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

					if (pd->suppress)
						continue;

					if (pdl != pr->pd_list) {
						o->current_page = pr->start_page;
						page_position = pr->start_page_position;
					}
					pd->start_page_position = pr->start_page_position;

					pd->page_indent0 = pd->page_indent = page_indent;
					if (pd->border_width_set)
						page_indent += pd->border_width;

					pd->column_width = pd->real_width - (pd->border_width_set ? 2 * pd->border_width : 0.0);
					if (pd->detail_columns > 1)
						pd->column_width = (pd->column_width - ((o->size_in_points ? pd->column_pad : pd->column_pad * 72.0) * (pd->detail_columns - 1))) / (double)pd->detail_columns;

					pd->current_column = 0;

					if (pd->height_set) {
						pd->real_height = (o->size_in_points ? 1.0 : p->font_size) * pd->height;
						pd->remaining_height = pd->real_height;
					}

					for (ocrpt_list *rl = pd->reports; rl; rl = rl->next) {
						ocrpt_report *r = (ocrpt_report *)rl->data;
						ocrpt_list *cbl;

						if (r->font_size_set)
							ocrpt_layout_set_font_sizes(o, r->font_name ? r->font_name : (p->font_name ? p->font_name : "Courier"), r->font_size, false, false, NULL, &r->font_width);
						else {
							r->font_size = p->font_size;
							r->font_width = p->font_width;
						}

						if (!o->precalculate) {
							for (cbl = r->start_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}
						}

						for (r->current_iteration = 0; !r->suppress && (r->current_iteration < r->iterations); r->current_iteration++) {
							r->executing = true;

							r->finished = false;
							if (r->height_set)
								r->remaining_height = r->height * (o->size_in_points ? 1.0 : (r->font_size_set ? r->font_size : p->font_size));

							ocrpt_query_navigate_start(r->query);

							if (o->precalculate) {
								ocrpt_report_resolve_breaks(r);
								ocrpt_report_resolve_variables(r);
								ocrpt_report_resolve_expressions(r);
							}

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
									ocrpt_layout_output_resolve(o, p, r, &r->nodata);
									ocrpt_layout_output_evaluate(o, p, r, &r->nodata);
									ocrpt_layout_output(o, p, pr, pd, r, &r->nodata, 0, &newpage, &page_indent, &page_position, &old_page_position);
								}
							} else {
								ocrpt_print_reportheader(o, p, pr, pd, r, 0, &newpage, &page_indent, &page_position, &old_page_position);
								if (o->noquery_show_nodata) {
									ocrpt_layout_output_resolve(o, p, r, &r->nodata);
									ocrpt_layout_output_evaluate(o, p, r, &r->nodata);
									ocrpt_layout_output(o, p, pr, pd, r, &r->nodata, 0, &newpage, &page_indent, &page_position, &old_page_position);
								}
							}

							if (r->height_set) {
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
							}
						}

						if (o->precalculate) {
							for (ocrpt_list *cbl = r->precalc_done_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}
						} else {
							for (cbl = r->done_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}
						}
					}

					if (!o->precalculate && pd->border_width_set && o->output_functions.draw_rectangle) {
						double bx, by, bw, bh;
						double page_end_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height - pd->border_width;

						bx = pd->page_indent0;
						by = pd->start_page_position;
						bw = pd->real_width;

						if (pd->height_set) {
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

						if (pd->border_width_set) {
							bx += 0.5 * pd->border_width;
							by += 0.5 * pd->border_width;
							bw -= pd->border_width;
						}

						o->output_functions.draw_rectangle(o, p, pr, pd, NULL,
														&pd->border_color, pd->border_width,
														bx, by, bw, bh);
					}

					if (pd->height_set && pd->finished) {
						page_position = pd->start_page_position + pd->remaining_height;
						if (pd->border_width_set)
							page_position += pd->border_width;
					} else if (pd->border_width_set)
						page_position += pd->border_width;

					if (!pr->end_page) {
						pr->end_page = o->current_page;
						pr->end_page_position = page_position;
					} else {
						if (pr->end_page == o->current_page) {
							if (pr->end_page_position < page_position)
								pr->end_page_position = page_position;
						} else {
							ocrpt_list *pagel;
							bool earlier = false;

							for (pagel = pr->start_page; pagel && pagel != pr->end_page; pagel = pagel->next) {
								if (pagel == o->current_page) {
									earlier = true;
									break;
								}
							}
							if (!earlier) {
								pr->end_page = o->current_page;
								pr->end_page_position = page_position;
							}
						}
					}

					page_indent = pd->page_indent;
					page_indent += pd->real_width;
				}

				o->current_page = pr->end_page;
				page_position = pr->end_page_position;
			}

			if (!o->precalculate) {
				for (ocrpt_list *cbl = p->iteration_callbacks; cbl; cbl = cbl->next) {
					ocrpt_part_cb_data *cbd = (ocrpt_part_cb_data *)cbl->data;

					cbd->func(o, p, cbd->data);
				}

				/* Use the previous row data temporarily */
				o->residx = ocrpt_expr_prev_residx(o->residx);

				page_position = ocrpt_layout_top_margin(o, p);
				if (!p->suppress_pageheader_firstpage || (p->suppress_pageheader_firstpage && o->current_page != o->pages)) {
					ocrpt_layout_output_evaluate(o, p, NULL, &p->pageheader);
					ocrpt_layout_output_init(&p->pageheader);
					ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &page_position);
					ocrpt_layout_output_internal(true, o, p, NULL, NULL, NULL, &p->pageheader, p->page_width, p->left_margin_value, &page_position);
				}

				page_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
				ocrpt_layout_output_evaluate(o, p, NULL, &p->pagefooter);
				ocrpt_layout_output_init(&p->pagefooter);
				ocrpt_layout_output_internal_preamble(o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &page_position);
				ocrpt_layout_output_internal(true, o, p, NULL, NULL, NULL, &p->pagefooter, p->page_width, p->left_margin_value, &page_position);

				/* Switch back to the current row data */
				o->residx = ocrpt_expr_next_residx(o->residx);
			}
		}
	}
}

DLL_EXPORT_SYM bool ocrpt_execute(opencreport *o) {
	if (!o)
		return false;

	switch (o->output_format) {
	case OCRPT_OUTPUT_PDF:
		ocrpt_pdf_init(o);
		break;
	default:
		break;
	}

	/* Initialize date() and now() values */
	time_t t = time(NULL);
	localtime_r(&t, &o->current_date->datetime);
	o->current_date->date_valid = true;
	localtime_r(&t, &o->current_timestamp->datetime);
	o->current_timestamp->date_valid = true;
	o->current_timestamp->time_valid = true;

	/* Set global default font sizes */
	ocrpt_layout_set_font_sizes(o, "Courier", OCRPT_DEFAULT_FONT_SIZE, false, false, &o->font_size, &o->font_width);

	/* Run all reports in precalculate mode if needed */
	o->precalculate = true;
	ocrpt_execute_parts(o);

	for (ocrpt_list *cbl = o->precalc_done_callbacks; cbl; cbl = cbl->next) {
		ocrpt_cb_data *cbd = (ocrpt_cb_data *)cbl->data;

		cbd->func(o, cbd->data);
	}

	/* Run all reports in layout mode */
	o->precalculate = false;
	ocrpt_execute_parts(o);

	if (o->output_functions.finalize)
		o->output_functions.finalize(o);

	return true;
}

DLL_EXPORT_SYM char *ocrpt_get_output(opencreport *o, size_t *length) {
	if (!o || !o->output_buffer) {
		if (length)
			*length = 0;
		return NULL;
	}

	if (length)
		*length = o->output_buffer->len;

	return o->output_buffer->str;
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
			ocrpt_mem_free(last_element->data);
			last_element->data = c1;
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
		if (!strcmp(cpath, (char *)spl->data)) {
			found = true;
			break;
		}
	}

	if (found)
		ocrpt_mem_free(cpath);
	else
		o->search_paths = ocrpt_list_append(o->search_paths, cpath);
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
		ocrpt_string *s = ocrpt_mem_string_new((char *)l->data, true);

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

	bindtextdomain(domainname, dirname);
	bind_textdomain_codeset(domainname, "UTF-8");
	o->textdomain = ocrpt_mem_strdup(domainname);
}

DLL_EXPORT_SYM void ocrpt_set_locale(opencreport *o, const char *locale) {
	if (!o || !locale)
		return;

	o->locale = newlocale(LC_ALL_MASK, locale, o->locale);
	if (o->locale == (locale_t)0)
		o->locale = newlocale(LC_ALL_MASK, "C", o->locale);
	o->locale_set = true;
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
