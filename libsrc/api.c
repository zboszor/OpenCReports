/*
 * OpenCReports main module
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/random.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <paper.h>

#include "scanner.h"
#include "opencreport-private.h"
#include "datasource.h"

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

	ocrpt_free_parts(o);
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

__attribute__((constructor))
static void initialize_ocrpt(void) {
	char *cwd;
	const char *system_paper_name;
	const struct paper *paper_info;
	int i;

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
