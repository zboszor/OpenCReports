/*
 * OpenCReports main module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <langinfo.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/random.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <paper.h>

#include "scanner.h"
#include "opencreport.h"
#include "datasource.h"
#include "variables.h"
#include "exprutil.h"

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

	ocrpt_list_free_deep(o->report_added_callbacks, ocrpt_mem_free);

	gmp_randclear(o->randstate);
	ocrpt_mem_string_free(o->converted, true);

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

DLL_EXPORT_SYM void ocrpt_add_report_added_cb(opencreport *o, ocrpt_report_cb func, void *data) {
	ocrpt_report_cb_data *ptr;

	if (!o || !func)
		return;

	ptr = ocrpt_mem_malloc(sizeof(ocrpt_report_cb_data));
	ptr->func = func;
	ptr->data = data;

	o->report_added_callbacks = ocrpt_list_append(o->report_added_callbacks, ptr);
}

static void ocrpt_execute_tail(opencreport *o, ocrpt_report *r, ocrpt_list *brl_start) {
	ocrpt_list *brl;

	/* Use the previous row data temporarily */
	o->residx = !o->residx;

	if (!o->precalculate) {
		for (brl = r->breaks; brl; brl = brl->next) {
			ocrpt_break *br = (ocrpt_break *)brl->data;
			ocrpt_list *brcbl;

			if (br->cb_triggered)
				continue;

			for (brcbl = br->callbacks; brcbl; brcbl = brcbl->next) {
				ocrpt_break_trigger_cb_data *cbd = (ocrpt_break_trigger_cb_data *)brcbl->data;

				cbd->func(o, r, br, cbd->data);
			}

			br->cb_triggered = true;
		}
	}

	if (brl_start == r->breaks)
		brl = r->breaks_reverse;
	else
		for (brl = r->breaks_reverse; brl; brl = brl->next)
			if (brl->data == brl_start->data)
				break;

	if (o->precalculate) {
		ocrpt_variables_add_precalculated_results(o, r, brl);
		ocrpt_report_expressions_add_delayed_results(o, r);
	}

	for (; brl; brl = brl->next) {
		ocrpt_break *br __attribute__((unused)) = (ocrpt_break *)brl->data;

		/* TODO: process break footer here */
		//printf("ocrpt_execute_tail: r %p break '%s' footer\n", r, br->name);
	}

	/* Switch back to the current row data */
	o->residx = !o->residx;
}

static unsigned int ocrpt_execute_one_report(opencreport *o, ocrpt_report *r, ocrpt_query *q) {
	ocrpt_list *brl_start = NULL;
	unsigned int rows = 0;

	while (ocrpt_query_navigate_next(o, q)) {
		ocrpt_list *brl;
		ocrpt_list *cbl;

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
				o->residx = !o->residx;

				if (o->precalculate)
					ocrpt_variables_add_precalculated_results(o, r, brl_start);

				for (brl = r->breaks_reverse; brl; brl = brl->next) {
					ocrpt_break *br = (ocrpt_break *)brl->data;

					/* TODO: process break footer here */
					//printf("ocrpt_execute: r %p break '%s' footer\n", r, br->name);

					if (br == brl_start->data)
						break;
				}
				/* Switch back to the current row data */
				o->residx = !o->residx;

				for (brl = brl_start; brl; brl = brl->next)
					ocrpt_break_reset_vars(o, r, (ocrpt_break *)brl->data);

				if (!o->precalculate)
					ocrpt_variables_advance_precalculated_results(o, r, brl_start);
			}

			for (brl = (rows == 1 ? r->breaks : brl_start); brl; brl = brl->next) {
				ocrpt_break *br __attribute__((unused)) = (ocrpt_break *)brl->data;

				/* TODO: process break header here */
				//printf("ocrpt_execute: r %p break '%s' header\n", r, br->name);
			}
		}

		ocrpt_report_evaluate_variables(o, r);
		ocrpt_report_evaluate_expressions(o, r);

		if (!o->precalculate) {
			for (cbl = r->newrow_callbacks; cbl; cbl = cbl->next) {
				ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

				cbd->func(o, r, cbd->data);
			}
		}

		/* TODO: process row FieldDetails here */
		//printf("ocrpt_execute: r %p process FieldDetails\n", r);

		if (brl_start && !o->precalculate) {
			for (brl = brl_start; brl; brl = brl->next) {
				ocrpt_break *br = (ocrpt_break *)brl->data;
				ocrpt_list *brcbl;

				br->cb_triggered = true;
				for (brcbl = br->callbacks; brcbl; brcbl = brcbl->next) {
					ocrpt_break_trigger_cb_data *cbd = (ocrpt_break_trigger_cb_data *)brcbl->data;

					cbd->func(o, r, br, cbd->data);
				}
			}
		}
	}

	ocrpt_execute_tail(o, r, brl_start);

	return rows;
}

DLL_EXPORT_SYM bool ocrpt_execute(opencreport *o) {
	if (!o)
		return false;

	/* Run all reports in precalculate mode if needed */
	o->precalculate = true;
	for (ocrpt_list *pl = o->parts; pl; pl = pl->next) {
		ocrpt_part *p = (ocrpt_part *)pl->data;

		for (ocrpt_list *row = p->rows; row; row = row->next) {
			for (ocrpt_list *pdl = (ocrpt_list *)row->data; pdl; pdl = pdl->next) {
				ocrpt_report *r = (ocrpt_report *)pdl->data;
				ocrpt_query *q;

				if (!ocrpt_report_validate(o, r))
					continue;

				q = (r->query ? r->query : (o->queries ? (ocrpt_query *)o->queries->data : NULL));

				if (!q) {
					/*
					 * TODO: no query, output the the NoData alternative part if it
					 * exists in precalculate mode: no output, just placeholder
					 * for layout
					 */
					continue;
				}

				r->executing = true;

				ocrpt_query_navigate_start(o, q);
				ocrpt_report_resolve_breaks(o, r);
				ocrpt_report_resolve_variables(o, r);
				ocrpt_report_resolve_expressions(o, r);

				if (r->have_delayed_expr) {
					r->data_rows = ocrpt_execute_one_report(o, r, q);

					ocrpt_variables_advance_precalculated_results(o, r, NULL);

					/* Reset queries and breaks */
					ocrpt_query_navigate_start(o, q);
					for (ocrpt_list *brl = r->breaks; brl; brl = brl->next)
						ocrpt_break_reset_vars(o, r, (ocrpt_break *)brl->data);
				}

				if (!r->data_rows) {
					/*
					 * TODO: no data rows, output the the NoData alternative part
					 * if it exists in precalculate mode: no output, just placeholder
					 * for layout
					 */
				}

				r->executing = false;
			}
		}
	}

	/* Run all reports in layout mode */
	o->precalculate = false;
	for (ocrpt_list *pl = o->parts; pl; pl = pl->next) {
		ocrpt_part *p = (ocrpt_part *)pl->data;

		for (ocrpt_list *row = p->rows; row; row = row->next) {
			for (ocrpt_list *pdl = (ocrpt_list *)row->data; pdl; pdl = pdl->next) {
				ocrpt_report *r = (ocrpt_report *)pdl->data;
				ocrpt_query *q;
				ocrpt_list *cbl;

				if (!ocrpt_report_validate(o, r))
					continue;

				q = (r->query ? r->query : (o->queries ? (ocrpt_query *)o->queries->data : NULL));

				if (!q) {
					/* TODO: no query, output the the NoData alternative part if it exists */
					continue;
				}

				r->executing = true;

				for (cbl = r->start_callbacks; cbl; cbl = cbl->next) {
					ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

					cbd->func(o, r, cbd->data);
				}

				if (!r->have_delayed_expr || r->data_rows)
					r->data_rows = ocrpt_execute_one_report(o, r, q);

				if (!r->data_rows) {
					/* TODO: no query rows, output the the NoData alternative part if it exists */
				}

				for (cbl = r->done_callbacks; cbl; cbl = cbl->next) {
					ocrpt_report_cb_data *cbd = (ocrpt_report_cb_data *)cbl->data;

					cbd->func(o, r, cbd->data);
				}

				r->executing = false;
			}
		}
	}

	return true;
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
	if (o->locale)
		freelocale(o->locale);

	o->locale = newlocale(LC_ALL_MASK, locale, (locale_t)0);
}

__attribute__((constructor))
static void initialize_ocrpt(void) {
	char *cwd;
	const char *system_paper_name;
	const struct paper *paper_info;
	int i;

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
