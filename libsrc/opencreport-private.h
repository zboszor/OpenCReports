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
	int allocated;
	int parsed;
	const char *path;
};

struct opencreport {
	/* Paper name and size */
	const ocrpt_paper *paper;
	ocrpt_paper paper0;
	int paper_iterator_idx;

	/* List and array of struct ocrpt_datasource */
	List *datasources;

	/* List and array of struct ocrpt_query */
	List *queries;

	/* List of struct opencreports_part */
	List *parts;

	mpfr_prec_t prec;
	mpfr_rnd_t rndmode;
};

extern char cwdpath[PATH_MAX];

void ocrpt_free_part(const struct opencreport_part *part);
void ocrpt_free_parts(opencreport *o);

#endif
