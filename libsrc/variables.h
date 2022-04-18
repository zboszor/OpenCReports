/*
 * Variable utilities
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _VARIABLES_H_
#define _VARIABLES_H_

#include <opencreport.h>

void ocrpt_variable_reset(opencreport *o, ocrpt_var *v);
void ocrpt_variables_add_precalculated_results(opencreport *o, ocrpt_report *r, ocrpt_list *brl_start, bool last_row);
void ocrpt_variables_advance_precalculated_results(opencreport *o, ocrpt_report *r, ocrpt_list *brl_start);

#endif
