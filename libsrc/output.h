/*
 * Output structure
 *
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <stdbool.h>
#include <cairo.h>
#include <pango/pangocairo.h>

#include <opencreport.h>
#include "listutil.h"

struct ocrpt_color;
typedef struct ocrpt_color ocrpt_color;

struct ocrpt_output_functions {
	void (*add_new_page)(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position);
	void *(*get_current_page)(opencreport *o);
	void (*set_current_page)(opencreport *o, void *page);
	bool (*is_current_page_first)(opencreport *o);
	void (*start_part)(opencreport *, ocrpt_part *);
	void (*end_part)(opencreport *, ocrpt_part *);
	void (*start_part_row)(opencreport *, ocrpt_part *, ocrpt_part_row *);
	void (*end_part_row)(opencreport *, ocrpt_part *, ocrpt_part_row *);
	void (*start_part_column)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *);
	void (*end_part_column)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *);
	void (*start_output)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *);
	void (*end_output)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *);
	void (*start_data_row)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *, double, double);
	void (*end_data_row)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *);
	void (*draw_hline)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_hline *, double, double, double, double);
	void (*set_font_sizes)(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width);
	void (*get_text_sizes)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *, ocrpt_text *, double);
	void (*draw_text)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *, ocrpt_text *, double, double);
	void (*draw_image)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *, ocrpt_image *, double, double, double, double, double);
	void (*draw_imageend)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *);
	void (*draw_rectangle)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_color *, double, double, double, double, double);
	void (*finalize)(opencreport *o);
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
	bool suppress_output:1;
	bool has_memo:1;
	bool height_exceeded:1;
	bool pd_height_exceeded:1;
	bool r_height_exceeded:1;
};

#endif /* _OUTPUT_H_ */
