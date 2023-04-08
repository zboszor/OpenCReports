/*
 * OpenCReports private main header
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_PRIVATE_H_
#define _OCRPT_PRIVATE_H_

#include <time.h>
#include "opencreport.h"
#include "output.h"
#include "color.h"

enum ocrpt_var_type_bit {
	OCRPT_VARIABLE_EXPRESSION_BIT = (1 << OCRPT_VARIABLE_EXPRESSION),
	OCRPT_VARIABLE_COUNT_BIT = (1 << OCRPT_VARIABLE_COUNT),
	OCRPT_VARIABLE_COUNTALL_BIT = (1 << OCRPT_VARIABLE_COUNT),
	OCRPT_VARIABLE_SUM_BIT = (1 << OCRPT_VARIABLE_SUM),
	OCRPT_VARIABLE_AVERAGE_BIT = (1 << OCRPT_VARIABLE_AVERAGE),
	OCRPT_VARIABLE_AVERAGEALL_BIT = (1 << OCRPT_VARIABLE_AVERAGEALL),
	OCRPT_VARIABLE_LOWEST_BIT = (1 << OCRPT_VARIABLE_LOWEST),
	OCRPT_VARIABLE_HIGHEST_BIT = (1 << OCRPT_VARIABLE_HIGHEST),
	OCRPT_VARIABLE_CUSTOM_BIT = (1 << OCRPT_VARIABLE_CUSTOM),
	OCRPT_VARIABLE_UNKNOWN_BIT = (1 << (OCRPT_VARIABLE_CUSTOM + 1)),
	OCRPT_IDENT_UNKNOWN_BIT = (1 << (OCRPT_VARIABLE_CUSTOM + 2)),
};
#define OCRPT_VARIABLE_MASK_ALL ( \
	OCRPT_VARIABLE_UNKNOWN_BIT | \
	OCRPT_VARIABLE_EXPRESSION_BIT | \
	OCRPT_VARIABLE_COUNT_BIT | \
	OCRPT_VARIABLE_COUNTALL_BIT | \
	OCRPT_VARIABLE_SUM_BIT | \
	OCRPT_VARIABLE_AVERAGE_BIT | \
	OCRPT_VARIABLE_LOWEST_BIT | \
	OCRPT_VARIABLE_HIGHEST_BIT | \
	OCRPT_VARIABLE_CUSTOM_BIT \
)

struct opencreport {
	/* Paper name and size */
	const ocrpt_paper *paper;
	ocrpt_paper paper0;

	ocrpt_function **functions;
	int32_t n_functions;

	/* List of expressions not assigned to an ocrpt_report structure */
	ocrpt_list *exprs;
	ocrpt_list *exprs_last;

	/* Expressions for settings */
	ocrpt_expr *size_unit_expr;
	ocrpt_expr *noquery_show_nodata_expr;
	ocrpt_expr *report_height_after_last_expr;

	/* List and array of struct ocrpt_datasource */
	ocrpt_list *datasources;

	/* List and array of struct ocrpt_query */
	ocrpt_list *queries;

	/* Internal area for encoding conversion */
	ocrpt_string *converted;

	/* File search paths */
	ocrpt_list *search_paths;

	/* List of struct ocrpt_part elements */
	ocrpt_list *parts;
	ocrpt_list *last_part;

	/* List of ocrpt_report_cb elements */
	ocrpt_list *part_added_callbacks;
	ocrpt_list *report_added_callbacks;
	ocrpt_list *precalc_done_callbacks;

	/* Output buffer for spooling */
	ocrpt_string *output_buffer;

	/* Page handling for PDF output, lists of cairo_surface_t pointers */
	ocrpt_output_functions output_functions;
	ocrpt_list *images;
	ocrpt_list *pages;
	ocrpt_list *last_page;
	ocrpt_list *current_page;
	ocrpt_list *drawing_page;
	cairo_surface_t *nullpage_cs;
	cairo_t *cr;

	/* The result of date() and now() functions */
	ocrpt_result *current_date;
	ocrpt_result *current_timestamp;
	/* The result of r.pageno and r.totpages */
	ocrpt_result *pageno;
	ocrpt_result *totpages;

	/* Locale specific data */
	char *textdomain;
	locale_t locale;

	/* Internal math defaults and states */
	mpfr_prec_t prec;
	mpfr_rnd_t rndmode;
	gmp_randstate_t randstate;

	/* Global (default) font size and approximate width */
	double font_size;
	double font_width;

	/* Alternating datasource row result index  */
	unsigned int residx:3;
	unsigned int output_format:3;
	bool precalculate:1;
	bool size_in_points:1;
	bool noquery_show_nodata:1;
	bool report_height_after_last:1;
	bool precision_set:1;
	bool rounding_mode_set:1;
	bool locale_set:1;
};

struct ocrpt_result {
	opencreport *o;
	/* Original lexer token or (computed) string value for expression */
	ocrpt_string *string;
	/* Converted numeric constant or computed numeric value for expression */
	mpfr_t number;
	/* Datetime value */
	struct tm datetime;
	/* Group indicators together as bitfields for space saving */
	enum ocrpt_result_type type:2;
	enum ocrpt_result_type orig_type:2;
	bool number_initialized:1;
	bool string_owned:1;
	bool date_valid:1;
	bool time_valid:1;
	bool interval:1;
	bool isnull:1;
	/*
	 * Date +/- month carry bits for handling invalid day-of-month.
	 * E.g. yyyy-01-31 + 1 month -> yyyy-02-31 which is an invalid date.
	 * How to handle this? Neither truncating to the last day of the month
	 * (i.e. yyyy-02-28), nor automatically wrapping over to the beginning
	 * of the next month (e.g. yyyy-03-03) are semantically valid.
	 * Instead, truncate to last day of month but keep the surplus days
	 * as carry bits. The next addition would also add the carry bits to
	 * the day-of-month value, which would make adding 1 month to a date
	 * associative, in other words these should be equivalent:
	 * (yyyy-01-31 +  1 month) + 1 month
	 *  yyyy-01-31 + (1 month  + 1 month)
	 */
	uint32_t day_carry:2;
};

struct ocrpt_query_result {
	const char *name;
	bool name_allocated;
	ocrpt_result result;
};

/* Default print debugger functions */
extern ocrpt_printf_func ocrpt_std_printf;
extern ocrpt_printf_func ocrpt_err_printf;

#endif
