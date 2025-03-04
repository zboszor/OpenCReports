/*
 * Output structure
 *
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <stdbool.h>
#include <cairo.h>
#include <pango/pangocairo.h>

#include <opencreport.h>
#include "listutil.h"

struct ocrpt_output_functions {
	void (*reset_state)(opencreport *o);
	void *(*get_new_page)(opencreport *o, ocrpt_part *p);
	void (*add_new_page)(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position);
	void (*add_new_page_epilogue)(opencreport *o);
	void *(*get_current_page)(opencreport *o);
	void (*set_current_page)(opencreport *o, void *page);
	bool (*is_current_page_first)(opencreport *o);
	void (*start_part)(opencreport *, ocrpt_part *);
	void (*end_part)(opencreport *, ocrpt_part *);
	void (*start_part_row)(opencreport *, ocrpt_part *, ocrpt_part_row *);
	void (*end_part_row)(opencreport *, ocrpt_part *, ocrpt_part_row *);
	void (*start_part_column)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *);
	void (*end_part_column)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *);
	void (*start_report)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *);
	void (*end_report)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *);
	void (*start_output)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *);
	void (*end_output)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *);
	void (*start_data_row)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *, ocrpt_line *, double, double);
	void (*end_data_row)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *, ocrpt_line *);
	void (*draw_hline)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *, ocrpt_hline *, double, double, double, double);
	void (*prepare_set_font_sizes)(opencreport *);
	void (*set_font_sizes)(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width);
	void (*prepare_get_text_sizes)(opencreport *);
	void (*get_text_sizes)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *, ocrpt_text *, bool, double);
	void (*draw_text)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *, ocrpt_line *, ocrpt_text *, bool, double, double, double);
	void (*draw_image)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *, ocrpt_line *, ocrpt_image *, bool, double, double, double, double, double, double);
	void (*draw_barcode)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *, ocrpt_line *, ocrpt_barcode *, bool, double, double, double, double, double, double);
	void (*draw_imageend)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_break *, ocrpt_output *);
	void (*draw_rectangle)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_color *, double, double, double, double, double);
	void (*finalize)(opencreport *o);
	bool supports_page_break:1;
	bool supports_column_break:1;
	bool supports_pd_height:1;
	bool supports_report_height:1;
	bool reopen_tags_across_pages:1;
	bool support_bbox:1;
	bool support_any_font:1;
	bool support_fontdesc:1;
	bool line_element_font:1;
};
typedef struct ocrpt_output_functions ocrpt_output_functions;

struct ocrpt_output {
	double old_page_position;
	opencreport *o;
	ocrpt_report *r;
	ocrpt_list *output_list;
	ocrpt_list *iter;
	ocrpt_expr *suppress;
	ocrpt_image *current_image;
	ocrpt_barcode *current_barcode;
	bool suppress_output:1;
	bool has_memo:1;
	bool height_exceeded:1;
	bool pd_height_exceeded:1;
	bool r_height_exceeded:1;
};

#endif /* _OUTPUT_H_ */
