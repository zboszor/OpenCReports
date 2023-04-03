/*
 * OpenCReports main module
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <opencreport.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "datasource.h"
#include "exprutil.h"

static void ocrpt_navigate_start_private(ocrpt_query *q);
static bool ocrpt_navigate_next_private(ocrpt_query *q);

static bool ocrpt_navigate_n_to_1_check_current(ocrpt_query *q) {
	ocrpt_list *fw;

	if (!q || !q->source || !q->source->input || !q->source->input->isdone)
		return false;

	if (q->source->input->isdone(q))
		return false;

	for (fw = q->followers_n_to_1; fw; fw = fw->next) {
		ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
		ocrpt_query *f = fo->follower;
		ocrpt_result *r;

		if (!f->source || !f->source->input || !f->source->input->isdone)
			continue;

		if (f->source->input->isdone(f))
			return false;

		r = ocrpt_expr_eval(fo->expr);
		if (r->isnull)
			return false;
		if (r->type == OCRPT_RESULT_NUMBER) {
			if (mpfr_cmp_si(r->number, 0) == 0)
				return false;
		} else
			return false;
	}

	return true;
}

static bool ocrpt_navigate_n_to_1_check_ended(ocrpt_query *q) {
	ocrpt_list *fw;
	bool ended = true;

	for (fw = q->followers_n_to_1; fw; fw = fw->next) {
		ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
		ocrpt_query *f = fo->follower;

		if (!f->source || !f->source->input || !f->source->input->isdone)
			continue;

		if (!f->source->input->isdone(f)) {
			ended = false;
			break;
		}
	}

	return ended;
}

static bool ocrpt_navigate_next_n_to_1(ocrpt_query *q) {
	bool retval = false;

	if (ocrpt_list_length(q->followers_n_to_1) == 0)
		return retval;

	if (!q->n_to_1_started) {
		ocrpt_list *fw;

		/*
		 * If the n_to_1 followers were not started yet,
		 * advance all of them with 1 rows, because
		 * they are at the start, i.e. before the first row.
		 */
		for (fw = q->followers_n_to_1; fw; fw = fw->next) {
			ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
			ocrpt_query *f = fo->follower;

			ocrpt_navigate_next_private(f);

			if (!f->source || !f->source->input || !f->source->input->isdone)
				continue;

			if (f->source->input->isdone(f))
				f->n_to_1_empty = true;
		}
	} else {
		ocrpt_list *fw;

		/*
		 * If the n_to_1 followers were started in a previous round,
		 * advance only the first non-empty resultset.
		 */
		for (fw = q->followers_n_to_1; fw; fw = fw->next) {
			ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
			ocrpt_query *f = fo->follower;

			if (f->n_to_1_empty)
				continue;

			ocrpt_navigate_next_private(f);
			break;
		}
	}

	do {
		ocrpt_list *fw, *fw1;

		if (ocrpt_navigate_n_to_1_check_ended(q))
			break;

		retval = ocrpt_navigate_n_to_1_check_current(q);
		if (retval)
			break;

		for (fw = q->followers_n_to_1; fw; fw = fw->next) {
			ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
			ocrpt_query *f = fo->follower;

			if (f->n_to_1_empty)
				continue;

			if (!f->source || !f->source->input || !f->source->input->isdone)
				continue;

			if (f->source->input->isdone(f)) {
				for (fw1 = q->followers_n_to_1; fw1 && fw1 != fw; fw1 = fw1->next) {
					ocrpt_query_follower *fo1 = (ocrpt_query_follower *)fw1->data;
					ocrpt_query *f1 = fo1->follower;

					ocrpt_navigate_start_private(f1);
					ocrpt_navigate_next_private(f1);
				}

				ocrpt_navigate_start_private(f);
				ocrpt_navigate_next_private(f);
				continue;
			}

			ocrpt_navigate_next_private(f);
			break;
		}
	} while (1);

	return retval;
}

static bool ocrpt_navigate_next_private(ocrpt_query *q) {
	ocrpt_list *fw;
	bool retval, retval11;

	if (!q || !q->source || !q->source->o)
		return false;

	q->source->input->populate_result(q);

	if (q->n_to_1_started) {
		retval = ocrpt_navigate_next_n_to_1(q);
		q->n_to_1_matched |= retval;

		if (retval)
			return true;

		if (ocrpt_navigate_n_to_1_check_ended(q)) {
			q->n_to_1_started = false;
			if (!q->n_to_1_matched)
				return true;
		}
	}

	if (!q->n_to_1_started) {
		if (!q->source->input || !q->source->input->next) {
			ocrpt_err_printf("datasource doesn't have ->next() function\n");
			return false;
		}
		if (!q->source->input->next(q)) {
			ocrpt_expr_init_result(q->rownum, OCRPT_RESULT_NUMBER);
			q->rownum->result[q->source->o->residx]->isnull = true;
			return false;
		}
		q->current_row++;
		ocrpt_expr_init_result(q->rownum, OCRPT_RESULT_NUMBER);
		mpfr_set_si(q->rownum->result[q->source->o->residx]->number, q->current_row + 1, q->source->o->rndmode);
	}

	/* 1:1 followers  */
	retval11 = false;
	for (fw = q->followers; fw; fw = fw->next) {
		ocrpt_query *f = (ocrpt_query *)fw->data;
		retval = ocrpt_navigate_next_private(f);
		if (f->followers_n_to_1)
			retval11 = (retval11 || retval);
	}

	/* n:1 followers */
	for (fw = q->followers_n_to_1; fw; fw = fw->next) {
		ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
		ocrpt_query *f = fo->follower;

		ocrpt_navigate_start_private(f);
	}

	retval = ocrpt_navigate_next_n_to_1(q);
	q->n_to_1_matched |= (retval || retval11);
	q->n_to_1_started = (retval || retval11);

	return true;
}

static void ocrpt_navigate_start_private(ocrpt_query *q) {
	ocrpt_list *fw;
	ocrpt_query_result *qr;
	int32_t cols;

	if (!q)
		return;

	assert(q->source);
	assert(q->source->o);

	qr = ocrpt_query_get_result(q, &cols);
	if (!qr || !cols) {
		q->navigation_failed = true;
		return;
	}

	if (q->source->input && q->source->input->rewind)
		q->source->input->rewind(q);
	else
		ocrpt_err_printf("'%s' doesn't have ->rewind() function\n", q->name);

	q->current_row = -1;
	ocrpt_expr_init_result(q->rownum, OCRPT_RESULT_NUMBER);
	q->rownum->result[q->source->o->residx]->isnull = true;
	q->n_to_1_empty = false;
	q->n_to_1_started = false;
	q->n_to_1_matched = false;

	for (fw = q->followers; fw; fw = fw->next) {
		ocrpt_query *f = (ocrpt_query *)fw->data;

		ocrpt_navigate_start_private(f);
	}

	for (fw = q->followers_n_to_1; fw; fw = fw->next) {
		ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
		ocrpt_query *f = fo->follower;

		ocrpt_navigate_start_private(f);
	}
}

DLL_EXPORT_SYM void ocrpt_query_navigate_start(ocrpt_query *q) {
	if (!q || !q->source || !q->source->o)
		return;

	q->source->o->residx = 0;
	ocrpt_navigate_start_private(q);
}

DLL_EXPORT_SYM bool ocrpt_query_navigate_next(ocrpt_query *q) {
	if (!q || !q->source || !q->source->o)
		return false;

	q->source->o->residx = ocrpt_expr_next_residx(q->source->o->residx);
	return ocrpt_navigate_next_private(q);
}
