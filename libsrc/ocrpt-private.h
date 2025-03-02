/*
 * OpenCReports private main header
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_PRIVATE_H_
#define _OCRPT_PRIVATE_H_

#include <time.h>
#include "opencreport.h"
#include "output.h"
#include "color.h"

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

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

struct ocrpt_search_path {
	const char *path;
	ocrpt_expr *expr;
};
typedef struct ocrpt_search_path ocrpt_search_path;

struct ocrpt_mvarentry {
	const char *name;
	const char *value;
};
typedef struct ocrpt_mvarentry ocrpt_mvarentry;

struct opencreport {
	/* Paper name and size */
	const ocrpt_paper *paper;
	ocrpt_paper paper0;

	ocrpt_function **functions;
	int32_t n_functions;

	/* "m" domain variables added by ocrpt_add_mvariable */
	ocrpt_list *mvarlist;
	ocrpt_list *mvarlist_end;

	/* List of expressions not assigned to an ocrpt_report structure */
	ocrpt_list *exprs;
	ocrpt_list *exprs_last;

	/* Expressions for settings */
	ocrpt_expr *size_unit_expr;
	ocrpt_expr *noquery_show_nodata_expr;
	ocrpt_expr *report_height_after_last_expr;
	ocrpt_expr *follower_match_single_expr;
	ocrpt_expr *precision_expr;
	ocrpt_expr *rounding_mode_expr;
	ocrpt_expr *locale_expr;

	/* Delayed domain settings */
	char *xlate_domain_s;
	char *xlate_dir_s;

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
	ocrpt_list *part_iteration_callbacks;
	ocrpt_list *report_added_callbacks;
	ocrpt_list *report_start_callbacks;
	ocrpt_list *report_iteration_callbacks;
	ocrpt_list *report_done_callbacks;
	ocrpt_list *report_newrow_callbacks;
	ocrpt_list *report_precalc_done_callbacks;
	ocrpt_list *precalc_done_callbacks;

	/* Output buffer for spooling */
	ocrpt_string *output_buffer;
	const ocrpt_string **content_type;

	/* Output parameters */
	char *html_meta;
	char *html_docroot;
	char *csv_filename;
	char *csv_delimiter;

	/* Page handling for PDF output, lists of cairo_surface_t pointers */
	ocrpt_output_functions output_functions;
	void *output_private;
	ocrpt_list *images;

	/* The result of date() and now() functions */
	ocrpt_result *current_date;
	ocrpt_result *current_timestamp;
	/* The result of r.pageno and r.totpages */
	ocrpt_result *pageno;
	ocrpt_result *totpages;
	/* The result of r.format */
	ocrpt_result *rptformat;
	/* Helper for ocrpt_inc and ocrpt_dec */
	ocrpt_result *one;

	/* Locale specific data */
	char *textdomain;
	locale_t locale;
	locale_t c_locale;

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
	bool n_to_1_lists_invalid:1;
	bool precalculate:1;
	bool size_in_points:1;
	bool noquery_show_nodata:1;
	bool report_height_after_last:1;
	bool follower_match_single:1;
	/* Bools for output parameters */
	bool suppress_html_head:1;
	bool csv_as_text:1;
	bool no_quotes:1;
	bool only_quote_strings:1;
	bool xml_rlib_compat:1;
	bool executing:1;
};

/* Default print debugger functions */
extern ocrpt_printf_func ocrpt_std_printf;
extern ocrpt_printf_func ocrpt_err_printf;

#endif
