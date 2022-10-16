/*
 * OpenCReports header
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _BREAKS_H_
#define _BREAKS_H_

#include "layout.h"

struct ocrpt_break {
	const char *name;
	ocrpt_list *breakfields;	/* list of ocrpt_expr pointers */
	ocrpt_list *callbacks;		/* list of ocrpt_break_trigger_cb_data pointers */
	ocrpt_expr *rownum;			/* row number of the break */
	ocrpt_output header;
	ocrpt_output footer;
	short index;
	bool attrs[OCRPT_BREAK_ATTRS_COUNT];
	bool cb_triggered;
};

struct ocrpt_break_trigger_cb_data {
	ocrpt_break_trigger_cb func;
	void *data;
};
typedef struct ocrpt_break_trigger_cb_data ocrpt_break_trigger_cb_data;

void ocrpt_break_free(opencreport *o, ocrpt_report *r, ocrpt_break *br);
void ocrpt_breaks_free(opencreport *o, ocrpt_report *r);
bool ocrpt_break_validate(opencreport *o, ocrpt_report *r, ocrpt_break *br);

#endif
