/*
 * Output structure
 *
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <cairo.h>
#include <pango/pangocairo.h>

#include <opencreport.h>
#include "listutil.h"

struct ocrpt_color;
typedef struct ocrpt_color ocrpt_color;

struct ocrpt_output_functions {
	void (*draw_hline)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_hline *, double, double, double, double);
	void (*get_text_sizes)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *, ocrpt_text *, double);
	void (*draw_text)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *, ocrpt_text *, double, double);
	void (*draw_image)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_column *, ocrpt_report *, ocrpt_output *, ocrpt_line *, ocrpt_image *, double, double, double, double, double);
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