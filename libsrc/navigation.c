/*
 * OpenCReports main module
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <opencreport.h>

#include "opencreport-private.h"
#include "datasource.h"

static void ocrpt_navigate_start_private(opencreport *o, ocrpt_query *q);
static bool ocrpt_navigate_next_private(opencreport *o, ocrpt_query *q);

static bool ocrpt_navigate_n_to_1_check_current(opencreport *o, ocrpt_query *q) {
	List *fw;

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

		r = ocrpt_expr_eval(o, fo->expr);
#if 0
		printf("%s:%d: after ocrpt_expr_eval:\n", __func__, __LINE__);
		ocrpt_expr_result_deep_print(o, fo->expr);
#endif
		if (r->type == OCRPT_RESULT_NUMBER) {
			if (mpfr_cmp_si(r->number, 0) == 0)
				return false;
		} else
			return false;
	}

	return true;
}

static bool ocrpt_navigate_n_to_1_check_ended(opencreport *o, ocrpt_query *q) {
	List *fw;
	bool ended = true;

	for (fw = q->followers_n_to_1; fw; fw = fw->next) {
		ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
		ocrpt_query *f = fo->follower;

		if (!f->source || !f->source->input || !f->source->input->isdone) {
			/* TODO: report that the input source is not OK? */
			continue;
		}

		if (!f->source->input->isdone(f)) {
			ended = false;
			break;
		}
	}

	return ended;
}

static bool ocrpt_navigate_next_n_to_1(opencreport *o, ocrpt_query *q) {
	bool retval = false;

	if (list_length(q->followers_n_to_1) == 0)
		return retval;

	if (!q->n_to_1_started) {
		List *fw;

		/*
		 * If the n_to_1 followers were not started yet,
		 * advance all of them with 1 rows, because
		 * they are at the start, i.e. before the first row.
		 */
		for (fw = q->followers_n_to_1; fw; fw = fw->next) {
			ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
			ocrpt_query *f = fo->follower;

			ocrpt_navigate_next_private(o, f);

			if (!f->source || !f->source->input || !f->source->input->isdone) {
				/* TODO: report that the input source is not OK? */
				continue;
			}

			if (f->source->input->isdone(f))
				f->n_to_1_empty = true;
		}
	} else {
		List *fw;

		/*
		 * If the n_to_1 followers were started in a previous round,
		 * advance only the first non-empty resultset.
		 */
		for (fw = q->followers_n_to_1; fw; fw = fw->next) {
			ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
			ocrpt_query *f = fo->follower;

			if (f->n_to_1_empty)
				continue;

			ocrpt_navigate_next_private(o, f);
			break;
		}
	}

	do {
		List *fw, *fw1;

		if (ocrpt_navigate_n_to_1_check_ended(o, q))
			break;

		retval = ocrpt_navigate_n_to_1_check_current(o, q);
		if (retval)
			break;

		for (fw = q->followers_n_to_1; fw; fw = fw->next) {
			ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
			ocrpt_query *f = fo->follower;

			if (f->n_to_1_empty)
				continue;

			if (!f->source || !f->source->input || !f->source->input->isdone) {
				/* TODO: report that the input source is not OK? */
				continue;
			}

			if (f->source->input->isdone(f)) {
				for (fw1 = q->followers_n_to_1; fw1 && fw1 != fw; fw1 = fw1->next) {
					ocrpt_query_follower *fo1 = (ocrpt_query_follower *)fw1->data;
					ocrpt_query *f1 = fo1->follower;

					ocrpt_navigate_start_private(o, f1);
					ocrpt_navigate_next_private(o, f1);
				}

				ocrpt_navigate_start_private(o, f);
				ocrpt_navigate_next_private(o, f);
				continue;
			}

			ocrpt_navigate_next_private(o, f);
			break;
		}
	} while (1);

	return retval;
}

static bool ocrpt_navigate_next_private(opencreport *o, ocrpt_query *q) {
	List *fw;
	bool retval, retval11;

	if (!o || !q || !q->source || !q->source->o || o != q->source->o) {
		fprintf(stderr, "%s:%d: opencreport and ocrpt_query structures do not match\n", __func__, __LINE__);
		return false;
	}

	q->source->input->populate_result(q);

	if (q->n_to_1_started) {
		retval = ocrpt_navigate_next_n_to_1(o, q);
		q->n_to_1_matched |= retval;

		if (retval)
			return true;

		if (ocrpt_navigate_n_to_1_check_ended(o, q)) {
			q->n_to_1_started = false;
			if (!q->n_to_1_matched)
				return true;
		}
	}

	if (!q->n_to_1_started) {
		if (!q->source->input || !q->source->input->next) {
			fprintf(stderr, "%s:%d: datasource doesn't have ->next() function\n", __func__, __LINE__);
			return false;
		}
		if (!q->source->input->next(q)) {
			//fprintf(stderr, "%s:%d: query '%s' has no more rows\n", __func__, __LINE__, q->name);
			return false;
		}
		q->current_row++;
	}

	/* 1:1 followers  */
	retval11 = false;
	for (fw = q->followers; fw; fw = fw->next) {
		ocrpt_query *f = (ocrpt_query *)fw->data;
		retval = ocrpt_navigate_next_private(o, f);
		if (f->followers_n_to_1)
			retval11 = (retval11 || retval);
	}

	/* n:1 followers */
	for (fw = q->followers_n_to_1; fw; fw = fw->next) {
		ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
		ocrpt_query *f = fo->follower;

		ocrpt_navigate_start_private(o, f);
	}

	retval = ocrpt_navigate_next_n_to_1(o, q);
	q->n_to_1_matched |= (retval || retval11);
	q->n_to_1_started = (retval || retval11);

	return true;
}

static void ocrpt_navigate_start_private(opencreport *o, ocrpt_query *q) {
	List *fw;
	ocrpt_query_result *qr;
	int32_t cols;

	if (!o || !q || !q->source || !q->source->o || o != q->source->o) {
		fprintf(stderr, "%s:%d: opencreport and ocrpt_query structures do not match\n", __func__, __LINE__);
		return;
	}

	qr = ocrpt_query_get_result(q, &cols);
	if (!qr || !cols) {
		q->navigation_failed = true;
		return;
	}

	if (q->source && q->source->input && q->source->input->rewind)
		q->source->input->rewind(q);
	else
		fprintf(stderr, "%s:%d: '%s' doesn't have ->rewind() function\n", __func__, __LINE__, q->name);

	q->current_row = -1;
	q->n_to_1_empty = false;
	q->n_to_1_started = false;
	q->n_to_1_matched = false;

	for (fw = q->followers; fw; fw = fw->next) {
		ocrpt_query *f = (ocrpt_query *)fw->data;

		ocrpt_navigate_start_private(o, f);
	}

	for (fw = q->followers_n_to_1; fw; fw = fw->next) {
		ocrpt_query_follower *fo = (ocrpt_query_follower *)fw->data;
		ocrpt_query *f = fo->follower;

		ocrpt_navigate_start_private(o, f);
	}
}

DLL_EXPORT_SYM void ocrpt_navigate_start(opencreport *o, ocrpt_query *q) {
	o->residx = true;
	ocrpt_navigate_start_private(o, q);
}

DLL_EXPORT_SYM bool ocrpt_navigate_next(opencreport *o, ocrpt_query *q) {
	o->residx = !o->residx;
	return ocrpt_navigate_next_private(o, q);
}
