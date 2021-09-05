/*
 * OpenCReports main module
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

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

static pthread_mutex_t ocrpt_locale_mtx = PTHREAD_MUTEX_INITIALIZER;

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

static void ocrpt_free_locale(opencreport *o) {
	int32_t i;

	ocrpt_mem_free(o->current_locale);

	ocrpt_mem_free(o->currency_symbol);
	ocrpt_mem_free(o->int_curr_symbol);
	ocrpt_mem_free(o->decimal_point);
	ocrpt_mem_free(o->thousands_sep);
	ocrpt_mem_free(o->negative_sign);
	ocrpt_mem_free(o->positive_sign);
	ocrpt_mem_free(o->mon_grouping);
	ocrpt_mem_free(o->mon_decimal_point);
	ocrpt_mem_free(o->mon_thousands_sep);
	ocrpt_mem_free(o->amstr);
	ocrpt_mem_free(o->pmstr);
	ocrpt_mem_free(o->ampm_fmt);
	ocrpt_mem_free(o->d_t_fmt);
	ocrpt_mem_free(o->d_fmt);
	ocrpt_mem_free(o->t_fmt);

	for (i = 0; i < 7; i++) {
		ocrpt_mem_free(o->dayname[i]);
		ocrpt_mem_free(o->abdayname[i]);
	}
	for (i = 0; i < 12; i++) {
		ocrpt_mem_free(o->monthname[i]);
		ocrpt_mem_free(o->abmonthname[i]);
	}
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
	ocrpt_free_locale(o);

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

DLL_EXPORT_SYM bool ocrpt_execute(opencreport *o) {
	if (!o)
		return false;

	return false;
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

DLL_EXPORT_SYM void ocrpt_lock_global_locale_mutex(void) {
	pthread_mutex_lock(&ocrpt_locale_mtx);
}

DLL_EXPORT_SYM void ocrpt_unlock_global_locale_mutex(void) {
	pthread_mutex_unlock(&ocrpt_locale_mtx);
}

DLL_EXPORT_SYM void ocrpt_set_locale(opencreport *o, const char *locale) {
	char *oldlocale;
	struct lconv *lc;
	int32_t i;
	static struct { int dn; int adn; } dayname[7] = {
		{ DAY_1, ABDAY_1 },
		{ DAY_2, ABDAY_2 },
		{ DAY_3, ABDAY_3 },
		{ DAY_4, ABDAY_4 },
		{ DAY_5, ABDAY_5 },
		{ DAY_6, ABDAY_6 },
		{ DAY_7, ABDAY_7 },
	};
	static struct {int mn; int amn; } monname[12] = {
		{ MON_1, ABMON_1 },
		{ MON_2, ABMON_2 },
		{ MON_3, ABMON_3 },
		{ MON_4, ABMON_4 },
		{ MON_5, ABMON_5 },
		{ MON_6, ABMON_6 },
		{ MON_7, ABMON_7 },
		{ MON_8, ABMON_8 },
		{ MON_9, ABMON_9 },
		{ MON_10, ABMON_10 },
		{ MON_11, ABMON_11 },
		{ MON_12, ABMON_12 },
	};

	if (o->current_locale && strcmp(locale, o->current_locale) == 0)
		return;

	ocrpt_lock_global_locale_mutex();
	oldlocale = ocrpt_mem_strdup(setlocale(LC_ALL, NULL));

	ocrpt_free_locale(o);

	o->current_locale = ocrpt_mem_strdup(locale);

	setlocale(LC_ALL, locale);

	lc = localeconv();

	o->currency_symbol = ocrpt_mem_strdup(lc->currency_symbol);
	o->int_curr_symbol = ocrpt_mem_strdup(lc->int_curr_symbol);
	o->decimal_point = ocrpt_mem_strdup(lc->decimal_point);
	o->thousands_sep = ocrpt_mem_strdup(lc->thousands_sep);
	o->negative_sign = ocrpt_mem_strdup(lc->negative_sign);
	o->positive_sign = ocrpt_mem_strdup(lc->positive_sign);
	o->mon_grouping = ocrpt_mem_strdup(lc->mon_grouping);
	o->mon_decimal_point = ocrpt_mem_strdup(lc->mon_decimal_point);
	o->mon_thousands_sep = ocrpt_mem_strdup(lc->mon_thousands_sep);
	o->amstr = ocrpt_mem_strdup(nl_langinfo(AM_STR));
	o->pmstr = ocrpt_mem_strdup(nl_langinfo(PM_STR));
	o->ampm_fmt = ocrpt_mem_strdup(nl_langinfo(T_FMT_AMPM));
	o->d_t_fmt = ocrpt_mem_strdup(nl_langinfo(D_T_FMT));
	o->d_fmt = ocrpt_mem_strdup(nl_langinfo(D_FMT));
	o->t_fmt = ocrpt_mem_strdup(nl_langinfo(T_FMT));

	o->int_n_cs_precedes = lc->int_n_cs_precedes;
	o->int_n_sep_by_space = lc->int_n_sep_by_space;
	o->int_n_sign_posn = lc->int_n_sign_posn;
	o->int_p_cs_precedes = lc->int_p_cs_precedes;
	o->int_p_sep_by_space = lc->int_p_sep_by_space;
	o->int_p_sign_posn = lc->int_p_sign_posn;
	o->n_cs_precedes = lc->n_cs_precedes;
	o->n_sep_by_space = lc->n_sep_by_space;
	o->n_sign_posn = lc->n_sign_posn;
	o->p_cs_precedes = lc->p_cs_precedes;
	o->p_sep_by_space = lc->p_sep_by_space;
	o->p_sign_posn = lc->p_sign_posn;
	o->int_frac_digits = lc->int_frac_digits;
	o->frac_digits = lc->frac_digits;

	for (i = 0; i < 7; i++) {
		o->dayname[i] = ocrpt_mem_strdup(nl_langinfo(dayname[i].dn));
		o->abdayname[i] = ocrpt_mem_strdup(nl_langinfo(dayname[i].adn));
	}
	for (i = 0; i < 12; i++) {
		o->monthname[i] = ocrpt_mem_strdup(nl_langinfo(monname[i].mn));
		o->abmonthname[i] = ocrpt_mem_strdup(nl_langinfo(monname[i].amn));
	}

	setlocale(LC_ALL, oldlocale);
	ocrpt_mem_free(oldlocale);
	ocrpt_unlock_global_locale_mutex();
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
