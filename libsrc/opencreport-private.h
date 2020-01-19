/*
 * OpenCReports main header
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORTS_PRIVATE_H_
#define _OPENCREPORTS_PRIVATE_H_

#include <stdlib.h>
#include <opencreport.h>
#include "exprutil.h"

#define UNUSED __attribute__((unused))

struct ocrpt_break {
	const char *name;
	ocrpt_expr *e_newpage;
	ocrpt_expr *e_headernewpage;
	ocrpt_expr *e_suppressblank;
	/* TODO: add details for header and footer */
	ocrpt_list *breakfields;	/* list of ocrpt_expr pointers */
	bool newpage:1;
	bool headernewpage:1;
	bool suppressblank:1;
};

struct opencreport_part {
	const char *xmlbuf;
	const char *path;
	bool allocated:1;
	bool parsed:1;
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
