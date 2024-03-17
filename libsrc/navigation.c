/*
 * OpenCReports query navigation handling
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <opencreport.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"
#include "datasource.h"
#include "exprutil.h"

static void ocrpt_navigate_start_private(ocrpt_query *topq, ocrpt_query *q);

static bool ocrpt_navigate_n_to_1_check_current(ocrpt_query *q) {
	ocrpt_list *l;
	size_t len = ocrpt_list_length(q->global_followers_n_to_1);
	size_t n_match = 0;

	for (l = q->global_followers_n_to_1; l; l = l->next) {
		ocrpt_query *q1 = (ocrpt_query *)l->data;

		if (q1->source->input->isdone(q1) && (q1->n_to_1_empty || !q1->n_to_1_matched))
			n_match++;
		else {
			ocrpt_result *r = ocrpt_expr_eval(q1->match);
			if (!r->isnull && r->type == OCRPT_RESULT_NUMBER) {
				int cmpres = mpfr_cmp_si(r->number, 0);
				if (cmpres != 0) {
					q1->n_to_1_matched = true;
					n_match++;
				}
			}
		}
	}

	return (len == n_match);
}

static bool ocrpt_navigate_n_to_1_check_ended(ocrpt_query *q, bool *all_isdone, bool *had_match) {
	ocrpt_list *l;
	size_t len = ocrpt_list_length(q->global_followers_n_to_1);
	size_t n_match = 0;
	size_t n_isdone = 0;
	bool isdone_matched = false;

	for (l = q->global_followers_n_to_1; l; l = l->next) {
		ocrpt_query *q1 = (ocrpt_query *)l->data;

		if (q1->source->input->isdone(q1)) {
			n_isdone++;
			if (q1->n_to_1_matched)
				isdone_matched = true;
		} else if (q1->n_to_1_matched) {
			n_match++;
		}
	}

	if (all_isdone)
		*all_isdone = (len == n_isdone);
	if (had_match)
		*had_match = isdone_matched;
	return (len == n_match + n_isdone);
}

static bool ocrpt_navigate_next_with_followers(ocrpt_query *q) {
	if (!q->source->input || !q->source->input->next) {
		ocrpt_err_printf("datasource doesn't have ->next() function\n");
		return false;
	}

	bool has_row = q->source->input->next(q);

	q->source->input->populate_result(q);
	q->current_row++;

	ocrpt_expr_init_result(q->rownum, OCRPT_RESULT_NUMBER);
	if (has_row) {
		mpfr_set_si(q->rownum->result[q->source->o->residx]->number, q->current_row + 1, q->source->o->rndmode);

		for (ocrpt_list *l = q->followers; l; l = l->next) {
			ocrpt_query *q1 = (ocrpt_query *)l->data;

			ocrpt_navigate_next_with_followers(q1);
		}
	} else {
		q->rownum->result[q->source->o->residx]->isnull = true;

		for (ocrpt_list *l = q->followers; l; l = l->next) {
			ocrpt_query *q1 = (ocrpt_query *)l->data;

			q1->source->input->rewind(q1);
			q1->source->input->populate_result(q1);
		}
	}

	return has_row;
}

static void ocrpt_query_navigate_init_n_to_1(ocrpt_query *q, ocrpt_list *end) {
	for (ocrpt_list *l = q->global_followers_n_to_1; l; l = l->next) {
		ocrpt_query *q1 = (ocrpt_query *)l->data;
		ocrpt_navigate_start_private(q, q1);
		q1->n_to_1_empty = !ocrpt_navigate_next_with_followers(q1);

		if (l->next == end)
			break;
	}
}

static bool ocrpt_navigate_next_n_to_1(ocrpt_query *q, ocrpt_list *start, ocrpt_list **next) {
	if (!start)
		return false;

	for (; start; start = start->next) {
		ocrpt_query *q1 = (ocrpt_query *)start->data;

		if (q1->n_to_1_empty)
			continue;

		if (ocrpt_navigate_next_with_followers(q1))
			return true;

		*next = start->next;
		return false;
	}

	return false;
}

static void ocrpt_navigate_start_private(ocrpt_query *topq, ocrpt_query *q) {
	ocrpt_list *l;
	ocrpt_query_result *qr;
	int32_t cols;

	if (!q)
		return;

	assert(q->source);
	assert(q->source->o);

	qr = ocrpt_query_get_result(q, &cols);
	if (!qr || !cols)
		return;

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
	q->global_followers_n_to_1_next = NULL;

	for (l = q->followers; l; l = l->next) {
		ocrpt_query *q1 = (ocrpt_query *)l->data;
		ocrpt_navigate_start_private(topq, q1);
	}

	if (topq == q)
		for (l = q->global_followers_n_to_1; l; l = l->next) {
			ocrpt_query *q1 = (ocrpt_query *)l->data;
			ocrpt_navigate_start_private(topq, q1);
		}
}

static void ocrpt_query_navigate_populate_followers(ocrpt_query *topq, ocrpt_query *q) {
	q->source->input->populate_result(q);

	ocrpt_list *l;

	for (l = q->followers; l; l = l->next) {
		ocrpt_query *q1 = (ocrpt_query *)l->data;

		ocrpt_query_navigate_populate_followers(topq, q1);
	}

	for (l = (topq == q ? q->global_followers_n_to_1 : NULL); l; l = l->next) {
		ocrpt_query *q1 = (ocrpt_query *)l->data;

		ocrpt_query_navigate_populate_followers(topq, q1);
	}
}

DLL_EXPORT_SYM void ocrpt_query_navigate_start(ocrpt_query *q) {
	if (!q || !q->source || !q->source->o)
		return;

	ocrpt_query_finalize_followers(q);

	q->source->o->residx = 0;
	ocrpt_navigate_start_private(q, q);
}

DLL_EXPORT_SYM bool ocrpt_query_navigate_next(ocrpt_query *q) {
	if (!q || !q->source || !q->source->o)
		return false;

	q->source->o->residx = ocrpt_expr_next_residx(q->source->o->residx);

	bool has_row = false;
	bool n_to_1_has_row = false;

	do {
		if (q->n_to_1_started) {
			has_row = true;

			ocrpt_query_navigate_populate_followers(q, q);

			bool all_isdone = false;
			bool had_match = false;
			bool ended = false;

			do {
				do {
					if (q->global_followers_n_to_1_next) {
						ocrpt_query_navigate_init_n_to_1(q, q->global_followers_n_to_1_next);
						ocrpt_list *dummy UNUSED = NULL;
						n_to_1_has_row = ocrpt_navigate_next_n_to_1(q, q->global_followers_n_to_1_next, &dummy);
						q->global_followers_n_to_1_next = NULL;
					} else
						n_to_1_has_row = ocrpt_navigate_next_n_to_1(q, q->global_followers_n_to_1, &q->global_followers_n_to_1_next);

					bool n_to_1_matched = ocrpt_navigate_n_to_1_check_current(q);
					if (n_to_1_has_row) {
						if (n_to_1_matched) {
							q->n_to_1_matched = true;
							if (q->source->o->follower_match_single)
								q->n_to_1_started = false;
							return true;
						}
					}
				} while (n_to_1_has_row);

				ended = ocrpt_navigate_n_to_1_check_ended(q, &all_isdone, &had_match);
				for (ocrpt_list *l = q->global_followers_n_to_1; l; l = l->next) {
					ocrpt_query *q1 = (ocrpt_query *)l->data;

					if (!q1->source->input->isdone(q1)) {
						q1->n_to_1_matched = false;
						break;
					}

					q->global_followers_n_to_1_next = l->next;
				}

				if (all_isdone)
					q->n_to_1_started = false;

				if (ended && ((!all_isdone && !had_match) || (all_isdone && !had_match && !q->n_to_1_matched))) {
					if (ended && !all_isdone && !had_match)
						q->n_to_1_matched = true;
					if (q->source->o->follower_match_single)
						q->n_to_1_started = false;
					return true;
				}
			} while (!all_isdone);

			q->n_to_1_started = false;
			q->n_to_1_matched = false;
			q->global_followers_n_to_1_next = NULL;
		}

		if (!q->n_to_1_started) {
			has_row = ocrpt_navigate_next_with_followers(q);

			if (!q->global_followers_n_to_1)
				break;

			q->n_to_1_started = true;
			q->n_to_1_matched = false;

			ocrpt_query_navigate_init_n_to_1(q, NULL);

			bool n_to_1_mached = ocrpt_navigate_n_to_1_check_current(q);
			if (n_to_1_mached)
				break;

			if (ocrpt_navigate_n_to_1_check_ended(q, NULL, NULL))
				break;

			if (!has_row)
				break;
		}
	} while (has_row);

	return has_row;
}

DLL_EXPORT_SYM void ocrpt_query_navigate_use_prev_row(ocrpt_query *q) {
	if (!q || !q->source || !q->source->o)
		return;

	opencreport *o = q->source->o;
	o->residx = ocrpt_expr_prev_residx(o->residx);
}

DLL_EXPORT_SYM void ocrpt_query_navigate_use_next_row(ocrpt_query *q) {
	if (!q || !q->source || !q->source->o)
		return;

	opencreport *o = q->source->o;
	o->residx = ocrpt_expr_next_residx(o->residx);
}
