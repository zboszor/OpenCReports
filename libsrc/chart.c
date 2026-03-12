/*
 * Report part utilities
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <opencreport.h>
#include "parts.h"
#include "exprutil.h"

DLL_EXPORT_SYM ocrpt_chart *ocrpt_layout_report_chart(ocrpt_report *r) {
	if (!r)
		return NULL;

	return &r->chart;
}

DLL_EXPORT_SYM bool ocrpt_chart_set_name(ocrpt_chart *chart, const char *name) {
	if (!chart)
		return false;

	ocrpt_chart *chart1 = ocrpt_chart_get_by_name(chart->o, name);
	if (chart1) {
		ocrpt_err_printf("Another chart in the same report already has name \"%s\"\n", name);
		return false;
	}

	if (chart->name)
		ocrpt_strfree(chart->name);

	chart->name = name ? ocrpt_mem_strdup(name) : NULL;
	return true;
}

DLL_EXPORT_SYM const char *ocrpt_chart_get_name(ocrpt_chart *chart) {
	return chart ? chart->name : NULL;
}

DLL_EXPORT_SYM ocrpt_chart *ocrpt_chart_get_by_name(opencreport *o, const char *name) {
	if (!o || !name)
		return NULL;

	for (ocrpt_list *pl = o->parts; pl; pl = pl->next) {
		ocrpt_part *p = (ocrpt_part *)pl->data;

		for (ocrpt_list *prl = p->rows; prl; prl = prl->next) {
			ocrpt_part_row *pr = (ocrpt_part_row *)prl->data;

			for (ocrpt_list *pdl = pr->pd_list; pdl; pdl = pdl->next) {
				ocrpt_part_column *pd = (ocrpt_part_column *)pdl->data;

				for (ocrpt_list *rl = pd->reports; rl; rl = rl->next) {
					ocrpt_report *r = (ocrpt_report *)rl->data;

					if (r->chart.name && strcmp(r->chart.name, name) == 0)
						return &r->chart;
				}
			}
		}
	}

	return NULL;
}

#define SET_CHART_EXPR(exprname)  {\
		if (!chart) \
			return NULL; \
		if (chart->exprname) \
			ocrpt_expr_free(chart->exprname); \
		chart->exprname = expr_string ? ocrpt_layout_expr_parse(chart->o, chart->r, expr_string, true, false) : NULL; \
		return chart->exprname; \
	}

#define GET_CHART_EXPR(exprname) \
	return chart ? chart->exprname : NULL

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_title(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(title);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_title(ocrpt_chart *chart) {
	GET_CHART_EXPR(title);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_title_font_name(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(title_font_name);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_title_font_name(ocrpt_chart *chart) {
	GET_CHART_EXPR(title_font_name);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_title_font_size(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(title_font_size);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_title_font_size(ocrpt_chart *chart) {
	GET_CHART_EXPR(title_font_size);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_cell_width(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(cell_width);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_cell_width(ocrpt_chart *chart) {
	GET_CHART_EXPR(cell_width);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_cell_height(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(cell_height);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_cell_height(ocrpt_chart *chart) {
	GET_CHART_EXPR(cell_height);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_cell_width_padding(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(cell_width_padding);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_cell_width_padding(ocrpt_chart *chart) {
	GET_CHART_EXPR(cell_width_padding);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_cell_height_padding(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(cell_height_padding);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_cell_height_padding(ocrpt_chart *chart) {
	GET_CHART_EXPR(cell_height_padding);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_label_width(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(label_width);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_label_width(ocrpt_chart *chart) {
	GET_CHART_EXPR(label_width);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_header_row_enabled(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(header_row);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_header_row_enabled(ocrpt_chart *chart) {
	GET_CHART_EXPR(header_row);
}

DLL_EXPORT_SYM void ocrpt_chart_set_header_row_query(ocrpt_chart *chart, ocrpt_query *query) {
	if (!chart || !chart->o || !chart->r)
		return;

	chart->header_row_query = query;
}

DLL_EXPORT_SYM ocrpt_query *ocrpt_chart_get_header_row_query(ocrpt_chart *chart) {
	if (!chart || !chart->o || !chart->r)
		return NULL;

	return chart->header_row_query;
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_header_row_field(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(header_row_field);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_header_row_field(ocrpt_chart *chart) {
	GET_CHART_EXPR(header_row_field);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_header_row_colspan(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(header_row_colspan);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_header_row_colspan(ocrpt_chart *chart) {
	GET_CHART_EXPR(header_row_colspan);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_row(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(row);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_row(ocrpt_chart *chart) {
	GET_CHART_EXPR(row);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_bar_start(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(bar_start);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_bar_start(ocrpt_chart *chart) {
	GET_CHART_EXPR(bar_start);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_bar_end(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(bar_end);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_bar_end(ocrpt_chart *chart) {
	GET_CHART_EXPR(bar_end);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_label(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(label);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_label(ocrpt_chart *chart) {
	GET_CHART_EXPR(label);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_bar_label(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(bar_label);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_bar_label(ocrpt_chart *chart) {
	GET_CHART_EXPR(bar_label);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_label_color(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(label_color);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_label_color(ocrpt_chart *chart) {
	GET_CHART_EXPR(label_color);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_bar_color(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(bar_color);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_bar_color(ocrpt_chart *chart) {
	GET_CHART_EXPR(bar_color);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_set_bar_label_color(ocrpt_chart *chart, const char *expr_string) {
	SET_CHART_EXPR(bar_label_color);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_chart_get_bar_label_color(ocrpt_chart *chart) {
	GET_CHART_EXPR(bar_label_color);
}

DLL_EXPORT_SYM void ocrpt_report_resolve_chart(ocrpt_chart *chart) {
	if (!chart || !chart->o || !chart->r)
		return;

	if (!chart->header_row_query ||
		!chart->header_row_field ||
		!chart->row || !chart->bar_start || !chart->bar_end) {
		chart->valid = false;
		return;
	}

	bool unresolved = false;

	/* <Chart> expressions */
	ocrpt_expr_resolve(chart->title);
	ocrpt_expr_resolve(chart->cell_width);
	ocrpt_expr_resolve(chart->cell_height);
	ocrpt_expr_resolve(chart->cell_width_padding);
	ocrpt_expr_resolve(chart->cell_height_padding);
	ocrpt_expr_resolve(chart->label_width);
	ocrpt_expr_resolve(chart->header_row);

	/* <HeaderRow> expressions */
	ocrpt_expr_resolve_worker(chart->header_row_field, chart->header_row_query, chart->header_row_field, NULL, 0, false, &unresolved);
	ocrpt_expr_resolve_worker(chart->header_row_colspan, chart->header_row_query, chart->header_row_colspan, NULL, 0, false, &unresolved);

	/* <Row> expressions */
	ocrpt_expr_resolve_worker(chart->row, NULL, chart->row, NULL, 0, false, &unresolved);
	ocrpt_expr_resolve_worker(chart->bar_start, NULL, chart->bar_start, NULL, 0, false, &unresolved);
	ocrpt_expr_resolve_worker(chart->bar_end, NULL, chart->bar_end, NULL, 0, false, &unresolved);
	ocrpt_expr_resolve(chart->label);
	ocrpt_expr_resolve(chart->bar_label);
	ocrpt_expr_resolve(chart->label_color);
	ocrpt_expr_resolve(chart->bar_color);
	ocrpt_expr_resolve(chart->bar_label_color);

	ocrpt_expr_optimize(chart->title);
	ocrpt_expr_optimize(chart->cell_width);
	ocrpt_expr_optimize(chart->cell_height);
	ocrpt_expr_optimize(chart->cell_width_padding);
	ocrpt_expr_optimize(chart->cell_height_padding);
	ocrpt_expr_optimize(chart->label_width);
	ocrpt_expr_optimize(chart->header_row);

	ocrpt_expr_optimize(chart->header_row_field);
	ocrpt_expr_optimize(chart->header_row_colspan);

	ocrpt_expr_optimize(chart->row);
	ocrpt_expr_optimize(chart->bar_start);
	ocrpt_expr_optimize(chart->bar_end);
	ocrpt_expr_optimize(chart->label);
	ocrpt_expr_optimize(chart->bar_label);
	ocrpt_expr_optimize(chart->label_color);
	ocrpt_expr_optimize(chart->bar_color);
	ocrpt_expr_optimize(chart->bar_label_color);

	chart->valid = !unresolved;
}

DLL_EXPORT_SYM bool ocrpt_chart_is_valid(ocrpt_chart *chart) {
	if (!chart)
		return false;

	return chart->valid;
}

void ocrpt_chart_gather_header_row_data(ocrpt_chart *chart) {
	if (!chart->header_row_query || !chart->header_row_field)
		chart->valid = false;

	if (!chart->valid)
		return;

	if (chart->hdrdata)
		return;

	ocrpt_query_navigate_start(chart->header_row_query);

	int32_t cols = 0;
	while (ocrpt_query_navigate_next(chart->header_row_query)) {
		cols++;

		ocrpt_chart_hdrdata *hdrdata = ocrpt_mem_malloc(sizeof(ocrpt_chart_hdrdata));

		/* Initialize hdrdata to the point where ocrpt_result_copy() works. */
		memset(hdrdata, 0, sizeof(ocrpt_chart_hdrdata));
		hdrdata->field.o = chart->o;

		ocrpt_expr_eval(chart->header_row_field);
		ocrpt_result_copy(&hdrdata->field, EXPR_RESULT(chart->header_row_field));

		ocrpt_expr_eval(chart->header_row_colspan);
		if (EXPR_VALID_NUMERIC(chart->header_row_colspan))
			hdrdata->colspan = mpfr_get_si(EXPR_NUMERIC(chart->header_row_colspan), MPFR_RNDN);
		if (hdrdata->colspan < 1)
			hdrdata->colspan = 1;
		
		ocrpt_expr_eval(chart->cell_height);
		if (EXPR_VALID_NUMERIC(chart->cell_height)) {
		}

		chart->hdrdata = ocrpt_list_end_append(chart->hdrdata, &chart->hdrdata_end, hdrdata);
	}

	if (cols == 0 || ocrpt_list_length(chart->hdrdata) == 0)
		chart->valid = false;
}
