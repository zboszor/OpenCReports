/*
 * OpenCReports header
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _PARTS_H_
#define _PARTS_H_

#include <stdint.h>
#include "layout.h"

struct ocrpt_report_cb_data {
	ocrpt_report_cb func;
	void *data;
};
typedef struct ocrpt_report_cb_data ocrpt_report_cb_data;

struct ocrpt_part_cb_data {
	ocrpt_part_cb func;
	void *data;
};
typedef struct ocrpt_part_cb_data ocrpt_part_cb_data;

struct ocrpt_cb_data {
	ocrpt_cb func;
	void *data;
};
typedef struct ocrpt_cb_data ocrpt_cb_data;

struct ocrpt_report {
	const char *font_name;
	double font_size;
	double font_width;
	double height;

	/* Temporaries */
	double start_page_position;
	double remaining_height;

	/* Output elements */
	ocrpt_output nodata;
	ocrpt_output reportheader;
	ocrpt_output reportfooter;
	ocrpt_output fieldheader;
	ocrpt_output fielddetails;

	/* Parent structure */
	opencreport *o;
	ocrpt_part *p;
	ocrpt_part_row *pr;
	ocrpt_part_column *pd;

	/* For r.detailcnt */
	ocrpt_expr *detailcnt;
	ocrpt_list *detailcnt_dependees;

	/* Parent part */
	ocrpt_part *part;
	/* Toplevel query */
	ocrpt_query *query;
	/* rownum() expression for the toplevel query */
	ocrpt_expr *query_rownum;
	/* List of ocrpt_var elements */
	ocrpt_list *variables;
	/* List of ocrpt_break elements */
	ocrpt_list *breaks;
	ocrpt_list *breaks_reverse;
	/* List of expressions */
	ocrpt_list *exprs;
	ocrpt_list *exprs_last;
	/* List of ocrpt_report_cb_data pointers */
	ocrpt_list *start_callbacks;
	ocrpt_list *done_callbacks;
	ocrpt_list *newrow_callbacks;
	ocrpt_list *precalc_done_callbacks;
	ocrpt_list *iteration_callbacks;
	/* Layout accounting */
	ocrpt_list *start_page;

	/* Parameter expressions */
	ocrpt_expr *filename_expr;
	ocrpt_expr *query_expr;
	ocrpt_expr *height_expr;
	ocrpt_expr *font_name_expr;
	ocrpt_expr *font_size_expr;
	ocrpt_expr *iterations_expr;
	ocrpt_expr *suppress_expr;
	ocrpt_expr *fieldheader_priority_expr;

	/*
	 * How many times should this report run
	 */
	uint32_t iterations;
	uint32_t current_iteration;

	/*
	 * Internal accounting for number of data rows
	 */
	uint32_t data_rows;
	/*
	 * Number of expression in the report
	 * including internal ones created for variables
	 */
	uint32_t num_expressions;
	bool have_delayed_expr:1;
	bool suppress:1;
	bool executing:1;
	bool dont_add_exprs:1;
	bool fieldheader_high_priority:1;
	bool finished:1;
	bool noquery_show_nodata:1;
	bool rlib_compat:1;
	bool height_valid:1;
};

struct ocrpt_part_column {
	double width;
	double real_width;
	double height;
	double real_height;
	double border_width;
	double column_pad;

	/* Temporaries */
	double column_width;
	double max_page_position;
	double page_indent;
	double page_indent0;
	double start_page_position;
	double remaining_height;

	ocrpt_color border_color;

	/* Parent structure */
	opencreport *o;

	/* Parameter expressions */
	ocrpt_expr *width_expr;
	ocrpt_expr *height_expr;
	ocrpt_expr *border_width_expr;
	ocrpt_expr *border_color_expr;
	ocrpt_expr *detail_columns_expr;
	ocrpt_expr *column_pad_expr;
	ocrpt_expr *suppress_expr;

	ocrpt_list *reports;
	ocrpt_list *last_report;
	uint32_t detail_columns;
	uint32_t current_column;
	bool suppress:1;
	bool finished:1;
};

struct ocrpt_part_row {
	double start_page_position;
	double end_page_position;

	/* Parent structure */
	opencreport *o;

	/* Parameter expressions */
	ocrpt_expr *layout_expr;
	ocrpt_expr *newpage_expr;
	ocrpt_expr *suppress_expr;

	ocrpt_list *start_page;
	ocrpt_list *end_page;
	ocrpt_list *pd_list;
	ocrpt_list *pd_last;
	bool newpage:1;
	bool suppress:1;
	bool fixed:1;
};

struct ocrpt_part {
	double font_size;
	double font_width;
	double top_margin;
	double bottom_margin;
	double left_margin;
	double right_margin;
	double page_header_height;
	double page_footer_height;

	/* Temporaries */
	double paper_width;
	double paper_height;
	double page_width;
	double left_margin_value;
	double right_margin_value;

	/* Common header and footer for all reports in this part */
	ocrpt_output pageheader;
	ocrpt_output pagefooter;

	/* Parent structure */
	opencreport *o;

	/* Part iteration callback */
	ocrpt_list *iteration_callbacks;

	/* Global parameters */
	ocrpt_expr *iterations_expr;
	ocrpt_expr *suppress_expr;
	ocrpt_expr *suppress_pageheader_firstpage_expr;
	ocrpt_expr *paper_type_expr;
	ocrpt_expr *orientation_expr;
	ocrpt_expr *font_name_expr;
	ocrpt_expr *font_size_expr;
	ocrpt_expr *top_margin_expr;
	ocrpt_expr *bottom_margin_expr;
	ocrpt_expr *left_margin_expr;
	ocrpt_expr *right_margin_expr;

	/* Paper */
	const ocrpt_paper *paper;

	/* Font name, non-parsed original and actual value */
	const char *font_name;

	/*
	 * List of ocrpt_part_row structures
	 * Each child list is the columns for the given row,
	 * which in turn are ocrpt_report elements.
	 */
	ocrpt_list *rows;
	ocrpt_list *row_last;

	/*
	 * How many times should this part run
	 */
	uint32_t iterations;
	uint32_t current_iteration;

	bool landscape:1;
	bool suppress:1;
	bool suppress_pageheader_firstpage:1;
	bool pageheader_printed:1;
};

void ocrpt_report_evaluate_detailcnt_dependees(ocrpt_report *r);

void ocrpt_part_free(struct ocrpt_part *p);
void ocrpt_parts_free(opencreport *o);
void ocrpt_report_free(ocrpt_report *r);

#endif
