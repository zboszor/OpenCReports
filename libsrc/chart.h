/*
 * Output structure
 *
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _CHART_H_
#define _CHART_H_

#include <opencreport.h>

struct ocrpt_chart_hdrdata {
	ocrpt_result field;
	double cell_width;
	int colspan;
};
typedef struct ocrpt_chart_hdrdata ocrpt_chart_hdrdata;

struct ocrpt_chart_rowdata {
	ocrpt_result row;
	double cell_height;
	bool label_printed;
};

struct ocrpt_chart {
	opencreport *o;
	ocrpt_report *r;

	/* Settings from <Chart> */
	char *name; /* If specified in the report XML, it must be a string constant expression. */
	ocrpt_expr *title; /* Constant-ish. Default is empty string / not rendered. */
	ocrpt_expr *title_font_name; /* Constant-ish. Default is taken from the main <Report> node. */
	ocrpt_expr *title_font_size; /* Constant-ish. Default is taken from the main <Report> node. */
	ocrpt_expr *cell_width; /* Constant or header row (i.e. column) data. Default width is computed from the widest cell label. */
	ocrpt_expr *cell_height; /* Constant or row data. Default height is computed from the current font. */
	ocrpt_expr *cell_width_padding; /* Constant or header row (i.e. column) data. Default 0. */
	ocrpt_expr *cell_height_padding; /* Constant or row data. Default 0. */
	ocrpt_expr *grid_line_width; /* Constant or header row data. Default 0.1 points. Grid is always rendered. */
	ocrpt_expr *label_width; /* Constant or header row data. Default is the widest label's rendered width. */
	ocrpt_expr *header_row; /* Constant-ish, boolean. Default yes. */
#if 0
	/* These exist in the XML DTD for RLIB compatibility but ignored. */
	ocrpt_expr *cols; /* Automatically deduced from the header row query data */
	ocrpt_expr *rows; /* Automatically deduced from the report query data */
#endif

	/* Expressions from <HeaderRow> */
	ocrpt_query *header_row_query;
	ocrpt_expr *header_row_field; /* Bar identifier value. It should be unique. bar_start and bar_end in "row" below are matched against it. */
	ocrpt_expr *header_row_label; /* Displayed column (bar) identifier. Default is the header_row_field value. */
	ocrpt_expr *header_row_colspan; /* Constant or header row data. Integer expression. Default 1. */

	/* Expressions from <Row> */
	ocrpt_expr *row; /* The row identifier. Multiple rows may use the same identifier. */
	ocrpt_expr *bar_start; /* Matches a header_row_field value. */
	ocrpt_expr *bar_end; /* Matches a header_row_field value. */
	ocrpt_expr *label; /* The label at the start of the row in the chart. Only the label of first row that has the same "row" value is rendered. */
	ocrpt_expr *bar_label; /* The label in the bar in the chart. */
	ocrpt_expr *label_color;
	ocrpt_expr *bar_color;
	ocrpt_expr *bar_label_color;

	/*
	 * List of struct ocrpt_chart_hdrdata
	 */
	ocrpt_list *hdrdata;
	ocrpt_list *hdrdata_end;

	ocrpt_list *row_params;
	ocrpt_list *row_params_end;

	bool valid:1;
};

void ocrpt_chart_gather_header_row_data(ocrpt_chart *chart);

#endif
