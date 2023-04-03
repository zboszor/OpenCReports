/*
 * OpenCReports header
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _BREAKS_H_
#define _BREAKS_H_

#include "layout.h"

struct ocrpt_break {
	ocrpt_report *r;
	const char *name;
	ocrpt_expr *headernewpage_expr;
	ocrpt_expr *suppressblank_expr;
	ocrpt_list *breakfields;	/* list of ocrpt_expr pointers */
	ocrpt_list *callbacks;		/* list of ocrpt_break_trigger_cb_data pointers */
	ocrpt_expr *rownum;			/* row number of the break */
	ocrpt_output header;
	ocrpt_output footer;
	short index;
	bool headernewpage:1;
	bool suppressblank:1;
	bool cb_triggered:1;
	bool blank:1;
	bool blank_prev:1;
};

struct ocrpt_break_trigger_cb_data {
	ocrpt_break_trigger_cb func;
	void *data;
};
typedef struct ocrpt_break_trigger_cb_data ocrpt_break_trigger_cb_data;

void ocrpt_break_free(ocrpt_break *br);
void ocrpt_breaks_free(ocrpt_report *r);

#endif
