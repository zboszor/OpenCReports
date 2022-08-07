/*
 * Formatting utilities
 *
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _LAYOUT_H_
#define _LAYOUT_H_

/* Margin defaults are in inches. 72.0 is the default DPI for PDF and others in Cairo */
#define OCRPT_DEFAULT_TOP_MARGIN (0.2 * 72.0)
#define OCRPT_DEFAULT_BOTTOM_MARGIN (0.2 * 72.0)
#define OCRPT_DEFAULT_LEFT_MARGIN (0.2 * 72.0)
#define OCRPT_DEFAULT_FONT_SIZE (12.0)

void *ocrpt_layout_new_page(opencreport *o, const ocrpt_paper *paper, bool landscape);
void ocrpt_layout_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width);

double ocrpt_layout_top_margin(opencreport *o, ocrpt_part *p);
double ocrpt_layout_bottom_margin(opencreport *o, ocrpt_part *p);
double ocrpt_layout_left_margin(opencreport *o, ocrpt_part *p, ocrpt_report *r);
double ocrpt_layout_right_margin(opencreport *o, ocrpt_part *p, ocrpt_report *r);

void ocrpt_layout_output_internal(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_list *output_list, double page_width, double page_indent, double *page_position);
void ocrpt_layout_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_list *output_list, unsigned int rows, double page_width, double page_height, double page_indent, double *page_position);
void ocrpt_layout_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, unsigned int rows, double page_width, double page_height, double page_indent, double *page_position);
void ocrpt_output_free(opencreport *o, ocrpt_report *r, ocrpt_list *output_list);
void ocrpt_image_free(const void *ptr);

#endif
