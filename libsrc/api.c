/*
 * OpenCReports main module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <langinfo.h>
#include <pthread.h>
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
#include "datasource.h"
#include "variables.h"
#include "exprutil.h"
#include "color.h"
#include "layout.h"
#include "pdf.h"

char cwdpath[PATH_MAX];
static ocrpt_paper *papersizes;
static const ocrpt_paper *system_paper;
int n_papersizes;

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
	ocrpt_set_locale(o, "C");

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

DLL_EXPORT_SYM void ocrpt_datasource_free(opencreport *o, ocrpt_datasource *source) {
	if (!ocrpt_datasource_validate(o, source))
		return;

	if (source->input && source->input->close)
		source->input->close(source);
	ocrpt_strfree(source->name);
	ocrpt_mem_free(source);

	o->datasources = ocrpt_list_remove(o->datasources, source);
}

DLL_EXPORT_SYM void ocrpt_free(opencreport *o) {
	int32_t i;

	if (!o)
		return;

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
		ocrpt_query_free(o, q);
	}

	while (o->datasources) {
		ocrpt_datasource *ds = (ocrpt_datasource *)o->datasources->data;
		ocrpt_datasource_free(o, ds);
	}

	ocrpt_parts_free(o);
	if (o->locale)
		freelocale(o->locale);

	ocrpt_list_free_deep(o->part_added_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->report_added_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->precalc_done_callbacks, ocrpt_mem_free);
	ocrpt_list_free_deep(o->part_iteration_callbacks, ocrpt_mem_free);

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

	ocrpt_mem_free(o);
}

DLL_EXPORT_SYM void ocrpt_set_numeric_precision_bits(opencreport *o, mpfr_prec_t prec) {
	if (!o)
		return;

	o->prec = prec;
}

DLL_EXPORT_SYM void ocrpt_set_rounding_mode(opencreport *o, mpfr_rnd_t rndmode) {
	if (!o)
		return;

	o->rndmode = rndmode;
}

DLL_EXPORT_SYM void ocrpt_add_part_added_cb(opencreport *o, ocrpt_part_cb func, void *data) {
	ocrpt_part_cb_data *ptr;

	if (!o || !func)
		return;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_part_cb_data));
	ptr->func = func;
	ptr->data = data;

	o->part_added_callbacks = ocrpt_list_append(o->part_added_callbacks, ptr);
}

DLL_EXPORT_SYM void ocrpt_add_report_added_cb(opencreport *o, ocrpt_report_cb func, void *data) {
	ocrpt_report_cb_data *ptr;

	if (!o || !func)
		return;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	ptr->func = func;
	ptr->data = data;

	o->report_added_callbacks = ocrpt_list_append(o->report_added_callbacks, ptr);
}

static unsigned int ocrpt_execute_one_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_query *q, double *page_indent, double *page_position) {
	ocrpt_list *brl_start = NULL;
	unsigned int rows = 0;
	bool have_row = ocrpt_query_navigate_next(o, q);

	r->current_column = 0;

	while (have_row) {
		ocrpt_list *brl;
		bool last_row = !ocrpt_query_navigate_next(o, q);

		o->residx = ocrpt_expr_prev_residx(o->residx);

		rows++;

		brl_start = NULL;
		for (brl = r->breaks; brl; brl = brl->next) {
			ocrpt_break *br = (ocrpt_break *)brl->data;

			br->cb_triggered = false;

			if (ocrpt_break_check_fields(o, r, br)) {
				if (!brl_start)
					brl_start = brl;
			}
		}

		if (brl_start) {
			if (rows > 1) {
				/* Use the previous row data temporarily */
				o->residx = ocrpt_expr_prev_residx(o->residx);

				if (o->precalculate && !last_row)
					ocrpt_variables_add_precalculated_results(o, r, brl_start, last_row);

				/* Switch back to the current row data */
				o->residx = ocrpt_expr_next_residx(o->residx);

				for (brl = brl_start; brl; brl = brl->next)
					ocrpt_break_reset_vars(o, r, (ocrpt_break *)brl->data);

				if (!o->precalculate && !last_row)
					ocrpt_variables_advance_precalculated_results(o, r, brl_start);
			}
		}

		ocrpt_report_evaluate_variables(o, r);
		ocrpt_report_evaluate_expressions(o, r);

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

		if (rows > 1 && brl_start) {
			/* Use the previous row data temporarily */
			o->residx = ocrpt_expr_prev_residx(o->residx);

			for (brl = r->breaks_reverse; brl; brl = brl->next) {
				ocrpt_break *br = (ocrpt_break *)brl->data;

				ocrpt_layout_output(o, p, pr, pd, r, br->footer, rows, page_indent, page_position);

				if (br == brl_start->data)
					break;
			}

			/* Switch back to the current row data */
			o->residx = ocrpt_expr_next_residx(o->residx);
		}

		for (brl = (rows == 1 ? r->breaks : brl_start); brl; brl = brl->next) {
			ocrpt_break *br __attribute__((unused)) = (ocrpt_break *)brl->data;

			if (br->cb_triggered)
				ocrpt_layout_output(o, p, pr, pd, r, br->header, rows, page_indent, page_position);
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
			ocrpt_layout_output(o, p, pr, pd, r, r->fieldheader, rows, page_indent, page_position);
		}
		ocrpt_layout_output(o, p, pr, pd, r, r->fielddetails, rows, page_indent, page_position);

		if (o->precalculate && last_row) {
			ocrpt_variables_add_precalculated_results(o, r, r->breaks, last_row);
			ocrpt_report_expressions_add_delayed_results(o, r);
		}

		have_row = !last_row;
		o->residx = ocrpt_expr_next_residx(o->residx);
	}

	if (rows) {
		/* Use the previous row data temporarily */
		o->residx = ocrpt_expr_prev_residx(o->residx);

		for (ocrpt_list *brl = r->breaks_reverse; brl; brl = brl->next) {
			ocrpt_break *br = (ocrpt_break *)brl->data;

			ocrpt_layout_output(o, p, pr, pd, r, br->footer, rows, page_indent, page_position);
		}

		ocrpt_layout_output(o, p, pr, pd, r, r->reportfooter, rows, page_indent, page_position);

		/* Switch back to the current row data */
		o->residx = ocrpt_expr_next_residx(o->residx);
	}

	return rows;
}

static void ocrpt_execute_parts(opencreport *o) {
	const ocrpt_paper *paper = o->paper;
	double page_position = 0.0;

	o->current_page = NULL;
	mpfr_set_ui(o->pageno->number, 1, o->rndmode);

	for (ocrpt_list *pl = o->parts; pl; pl = pl->next) {
		ocrpt_part *p = (ocrpt_part *)pl->data;
		int32_t part_iter;
		bool newpage __attribute__((unused)) = false;

		if (!p->paper)
			p->paper = o->paper;

		if (p->orientation_set && p->landscape) {
			p->paper_width = paper->height;
			p->paper_height = paper->width;
		} else {
			p->paper_width = paper->width;
			p->paper_height = paper->height;
		}

		if (paper->width != p->paper->width || paper->height != p->paper->height) {
			paper = p->paper;
			newpage = true;
		}

		if (p->font_size_set)
			ocrpt_layout_set_font_sizes(o, p->font_name ? p->font_name : "Courier", p->font_size, false, false, NULL, &p->font_width);
		else {
			p->font_size = o->font_size;
			p->font_width = o->font_width;
		}

		double left_margin = ocrpt_layout_left_margin(o, p, NULL);
		double right_margin = ocrpt_layout_right_margin(o, p, NULL);

		p->page_width = p->paper_width - (left_margin + right_margin);

		p->page_header_height = 0.0;
		p->page_footer_height = 0.0;
		ocrpt_layout_output_internal(false, o, p, NULL, NULL, NULL, p->pageheader, p->page_width, left_margin, &p->page_header_height);
		ocrpt_layout_output_internal(false, o, p, NULL, NULL, NULL, p->pagefooter, p->page_width, left_margin, &p->page_footer_height);

		for (part_iter = 0; part_iter < p->iterations; part_iter++) {
			for (ocrpt_list *row = p->rows; row; row = row->next) {
				ocrpt_part_row *pr = (ocrpt_part_row *)row->data;

				for (ocrpt_list *pdl = (ocrpt_list *)pr->pd_list; pdl; pdl = pdl->next) {
					ocrpt_part_row_data *pd = (ocrpt_part_row_data *)pdl->data;

					for (ocrpt_list *rl = pd->reports; rl; rl = rl->next) {
						ocrpt_report *r = (ocrpt_report *)rl->data;
						ocrpt_query *q;
						ocrpt_list *cbl;
						int32_t rpt_iter;
						double page_indent;

						if (!ocrpt_report_validate(o, r)) {
							fprintf(stderr, "ocrpt_execute_parts: report not valid???\n");
							continue;
						}

						r->column_width = p->page_width;
						if (r->detail_columns > 1)
							r->column_width = (p->page_width - (r->column_pad * (r->detail_columns - 1))) / (double)r->detail_columns;

						if (r->font_size_set)
							ocrpt_layout_set_font_sizes(o, r->font_name ? r->font_name : (p->font_name ? p->font_name : "Courier"), r->font_size, false, false, NULL, &r->font_width);
						else {
							r->font_size = p->font_size;
							r->font_width = p->font_width;
						}

						q = (r->query ? r->query : (o->queries ? (ocrpt_query *)o->queries->data : NULL));

						if (!o->precalculate) {
							for (cbl = r->start_callbacks; cbl; cbl = cbl->next) {
								ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

								cbd->func(o, r, cbd->data);
							}
						}

						for (rpt_iter = 0; rpt_iter < r->iterations; rpt_iter++) {
							r->executing = true;

							left_margin = page_indent = r->page_indent = ocrpt_layout_left_margin(o, p, r);
							right_margin = ocrpt_layout_right_margin(o, p, r);

							if (q) {
								if (o->precalculate) {
									ocrpt_query_navigate_start(o, q);
									ocrpt_report_resolve_breaks(o, r);
									ocrpt_report_resolve_variables(o, r);
									ocrpt_report_resolve_expressions(o, r);

									if (r->have_delayed_expr) {
										r->data_rows = ocrpt_execute_one_report(o, p, pr, pd, r, q, &page_indent, &page_position);

										ocrpt_variables_advance_precalculated_results(o, r, NULL);

										/* Reset queries and breaks */
										ocrpt_query_navigate_start(o, q);
										for (ocrpt_list *brl = r->breaks; brl; brl = brl->next)
											ocrpt_break_reset_vars(o, r, (ocrpt_break *)brl->data);
									}
								} else {
									if (!r->have_delayed_expr || r->data_rows)
										r->data_rows = ocrpt_execute_one_report(o, p, pr, pd, r, q, &page_indent, &page_position);
								}

								if (!r->data_rows)
									ocrpt_layout_output(o, p, pr, pd, r, r->nodata, 0, &page_indent, &page_position);
							} else
								ocrpt_layout_output(o, p, pr, pd, r, r->nodata, 0, &page_indent, &page_position);

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
				}
			}

			if (!o->precalculate) {
				for (ocrpt_list *cbl = o->part_iteration_callbacks; cbl; cbl = cbl->next) {
					ocrpt_part_cb_data *cbd = (ocrpt_part_cb_data *)cbl->data;

					cbd->func(o, p, cbd->data);
				}

				/* Use the previous row data temporarily */
				o->residx = ocrpt_expr_prev_residx(o->residx);

				page_position = ocrpt_layout_top_margin(o, p);
				ocrpt_layout_output_internal(true, o, p, NULL, NULL, NULL, p->pageheader, p->page_width, left_margin, &page_position);

				page_position = p->paper_height - ocrpt_layout_bottom_margin(o, p) - p->page_footer_height;
				ocrpt_layout_output_internal(true, o, p, NULL, NULL, NULL, p->pagefooter, p->page_width, left_margin, &page_position);

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

DLL_EXPORT_SYM void ocrpt_spool(opencreport *o) {
	if (!o->output_buffer)
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
	struct stat st;

	if (!o || !filename)
		return NULL;

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
	const ocrpt_paper *paper = ocrpt_get_paper_by_name(papername);

	if (!o)
		return;

	if (!paper)
		paper = system_paper;

	o->paper = paper;
}

DLL_EXPORT_SYM const ocrpt_paper *ocrpt_paper_first(opencreport *o) {
	if (!o)
		return NULL;
	o->paper_iterator_idx = 0;
	return &papersizes[o->paper_iterator_idx];
}

DLL_EXPORT_SYM const ocrpt_paper *ocrpt_paper_next(opencreport *o) {
	if (!o)
		return NULL;

	if (o->paper_iterator_idx >= n_papersizes)
		return NULL;

	o->paper_iterator_idx++;
	return (o->paper_iterator_idx >= n_papersizes ? NULL : &papersizes[o->paper_iterator_idx]);
}

DLL_EXPORT_SYM void ocrpt_set_locale(opencreport *o, const char *locale) {
	o->locale = newlocale(LC_ALL_MASK, locale, o->locale);
	if (o->locale == (locale_t)0)
		o->locale = newlocale(LC_ALL_MASK, "C", o->locale);
}

DLL_EXPORT_SYM void ocrpt_set_caret_operator_is_pow(opencreport *o) {
	o->caret_is_pow = true;
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
