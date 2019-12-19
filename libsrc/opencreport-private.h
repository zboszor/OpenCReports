/*
 * OpenCReports main header
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORTS_PRIVATE_H_
#define _OPENCREPORTS_PRIVATE_H_

#include <stdlib.h>
#include <opencreport.h>
#include "listutil.h"
#include "exprutil.h"

#define UNUSED __attribute__((unused))

struct opencreport_part {
	const char *xmlbuf;
	const char *path;
	bool allocated:1;
	bool parsed:1;
};

struct opencreport {
	/* Paper name and size */
	const ocrpt_paper *paper;
	ocrpt_paper paper0;
	int32_t paper_iterator_idx;

	/* List and array of struct ocrpt_datasource */
	List *datasources;

	/* List and array of struct ocrpt_query */
	List *queries;

	/* List of struct opencreports_part */
	List *parts;

	mpfr_prec_t prec;
	mpfr_rnd_t rndmode;
	gmp_randstate_t randstate;

	/* Alternating datasource row result index  */
	bool residx:1;
};

extern char cwdpath[PATH_MAX];

void ocrpt_free_part(const struct opencreport_part *part);
void ocrpt_free_parts(opencreport *o);

static inline void ocrpt_expr_set_result_owned(opencreport *o, ocrpt_expr *e, bool owned) {
	if (o->residx)
		e->result_owned1 = owned;
	else
		e->result_owned0 = owned;
}

static inline bool ocrpt_expr_get_result_owned(opencreport *o, ocrpt_expr *e) {
	if (o->residx)
		return e->result_owned1;
	else
		return e->result_owned0;
}

#endif
