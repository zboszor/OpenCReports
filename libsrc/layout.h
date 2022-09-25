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

enum ocrpt_output_type {
	OCRPT_OUTPUT_LINE,
	OCRPT_OUTPUT_HLINE,
	OCRPT_OUTPUT_IMAGE
};
typedef enum ocrpt_output_type ocrpt_output_type;

struct ocrpt_line_element {
	ocrpt_expr *value;
	ocrpt_expr *format;
	ocrpt_expr *width;
	ocrpt_expr *align;
	ocrpt_expr *color;
	ocrpt_expr *bgcolor;
	ocrpt_expr *font_name;
	ocrpt_expr *font_size;
	ocrpt_expr *bold;
	ocrpt_expr *italic;
	ocrpt_expr *link;
	ocrpt_expr *translate;

	/* Shortcuts carried over between get_text_sizes() and draw_text() */
	const char *font;
	ocrpt_string *value_str;
	double fontsz;
	double font_width;
	double start;
	double ascent;
	double descent;
	double width_computed;

	int32_t memo_max_lines;
	int32_t lines;
	int32_t col;
	bool memo:1;
	bool memo_wrap_chars:1;
	bool bold_computed:1; /* Also a shortcut */
	bool bold_is_set:1;
	bool bold_value:1;
	bool italic_is_set:1;
	bool italic_value:1;
	bool value_allocated:1;
};
typedef struct ocrpt_line_element ocrpt_line_element;

struct ocrpt_line {
	ocrpt_output_type type;
	bool bold_is_set:1;
	bool bold_value:1;
	bool italic_is_set:1;
	bool italic_value:1;
	bool suppress_line:1;
	double ascent;
	double descent;
	double line_height;
	double fontsz;
	double font_width;
	double page_indent;
	ocrpt_expr *font_name;
	ocrpt_expr *font_size;
	ocrpt_expr *color;
	ocrpt_expr *bgcolor;
	ocrpt_expr *bold;
	ocrpt_expr *italic;
	ocrpt_expr *suppress;
	ocrpt_list *elements; /* ocrpt_line_element */
	uint32_t maxlines;
	uint32_t current_line;
};
typedef struct ocrpt_line ocrpt_line;

struct ocrpt_hline {
	ocrpt_output_type type;
	bool suppress_hline:1;
	double font_width;
	ocrpt_expr *size;
	ocrpt_expr *indent;
	ocrpt_expr *length;
	ocrpt_expr *font_size;
	ocrpt_expr *suppress;
	ocrpt_expr *color;
};
typedef struct ocrpt_hline ocrpt_hline;

struct ocrpt_image_file {
	char *name;
	void *surface;
	void *rsvg;
	void *pixbuf;
	double width;
	double height;
};
typedef struct ocrpt_image_file ocrpt_image_file;

struct ocrpt_image {
	ocrpt_output_type type;
	bool suppress_image:1;
	double image_width;
	double image_height;
	ocrpt_expr *suppress;
	ocrpt_expr *value; /* name of the image file */
	ocrpt_expr *imgtype; /* 'png', 'jpg', 'raster', or 'svg' */
	ocrpt_expr *width;
	ocrpt_expr *height;
	ocrpt_image_file *img_file;
};
typedef struct ocrpt_image ocrpt_image;

struct ocrpt_output_element {
	ocrpt_output_type type;
};
typedef struct ocrpt_output_element ocrpt_output_element;

struct ocrpt_output_functions {
	void (*draw_hline)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_row_data *, ocrpt_report *, ocrpt_hline *, double, double, double, double);
	void (*get_text_sizes)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_row_data *, ocrpt_report *, ocrpt_line *, ocrpt_line_element *, double);
	void (*draw_text)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_row_data *, ocrpt_report *, ocrpt_line *, ocrpt_line_element *, double, double, double);
	void (*draw_image)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_row_data *, ocrpt_report *, ocrpt_image *, double, double, double, double);
	void (*draw_rectangle)(opencreport *, ocrpt_part *, ocrpt_part_row *, ocrpt_part_row_data *, ocrpt_report *, ocrpt_color *, double, double, double, double, double);
	void (*finalize)(opencreport *o);
};
typedef struct ocrpt_output_functions ocrpt_output_functions;

struct ocrpt_output {
	ocrpt_list *output_list;
	ocrpt_expr *suppress;
	ocrpt_image *current_image;
	bool suppress_output:1;
	bool has_memo:1;
};

void *ocrpt_layout_new_page(opencreport *o, const ocrpt_paper *paper, bool landscape);
void ocrpt_layout_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width);
void ocrpt_layout_output_resolve(opencreport *o, ocrpt_part *p, ocrpt_report *r, ocrpt_output *output);
void ocrpt_layout_output_evaluate(opencreport *o, ocrpt_part *p, ocrpt_report *r, ocrpt_output *output);

double ocrpt_layout_top_margin(opencreport *o, ocrpt_part *p);
double ocrpt_layout_bottom_margin(opencreport *o, ocrpt_part *p);
double ocrpt_layout_left_margin(opencreport *o, ocrpt_part *p);
double ocrpt_layout_right_margin(opencreport *o, ocrpt_part *p);

void ocrpt_layout_output_internal_preamble(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, double page_width, double page_indent, double *page_position);
void ocrpt_layout_output_internal(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, double page_width, double page_indent, double *page_position);
void ocrpt_layout_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, ocrpt_output *output, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position);
void ocrpt_layout_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position);
void ocrpt_layout_output_highprio_fieldheader(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position);
void ocrpt_output_free(opencreport *o, ocrpt_output *output, bool free_subexprs);
void ocrpt_image_free(const void *ptr);

#endif
