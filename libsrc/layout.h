/*
 * Layout utilities
 *
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#ifndef _LAYOUT_H_
#define _LAYOUT_H_

#include "opencreport.h"
#include "ocrpt-private.h"
#include "listutil.h"

/* Margin defaults are in inches. 72.0 is the default DPI for PDF and others in Cairo */
#define OCRPT_DEFAULT_TOP_MARGIN (0.2)
#define OCRPT_DEFAULT_BOTTOM_MARGIN (0.2)
#define OCRPT_DEFAULT_LEFT_MARGIN (0.2)
#define OCRPT_DEFAULT_RIGHT_MARGIN (0.2)
#define OCRPT_DEFAULT_FONT_SIZE (12.0)

enum ocrpt_output_type {
	OCRPT_OUTPUT_LINE,
	OCRPT_OUTPUT_HLINE,
	OCRPT_OUTPUT_IMAGE,
	OCRPT_OUTPUT_IMAGEEND
};
typedef enum ocrpt_output_type ocrpt_output_type;

enum ocrpt_output_line_element_type {
	OCRPT_OUTPUT_LE_TEXT,
	OCRPT_OUTPUT_LE_IMAGE,
	OCRPT_OUTPUT_LE_BARCODE
};
typedef enum ocrpt_output_line_element_type ocrpt_output_line_element_type;

struct ocrpt_text {
	ocrpt_output_line_element_type le_type;
	ocrpt_output *output;
	ocrpt_expr *value;
	ocrpt_expr *delayed;
	ocrpt_expr *memo_expr;
	ocrpt_expr *wrap_chars;
	ocrpt_expr *max_lines;
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

	ocrpt_list *drawing_page;
	PangoLayout *layout;
	PangoFontDescription *font_description;
	PangoLayoutLine *pline;

	/* Shortcuts carried over between get_text_sizes() and draw_text() */
	const char *font;
	ocrpt_string *format_str;
	ocrpt_string *value_str;
	ocrpt_string *result_str;
	double fontsz;
	double font_width;
	double start;
	double ascent;
	double descent;
	double width_computed;

	PangoRectangle p_rect;
	PangoAlignment p_align;
	int32_t memo_max_lines;
	int32_t lines;
	bool bold_val:1;
	bool italic_val:1;
	bool memo:1;
	bool memo_wrap_chars:1;
	bool use_bb:1;
	bool justified:1;
};

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
	ocrpt_output *output;
	ocrpt_expr *font_name;
	ocrpt_expr *font_size;
	ocrpt_expr *color;
	ocrpt_expr *bgcolor;
	ocrpt_expr *bold;
	ocrpt_expr *italic;
	ocrpt_expr *suppress;
	ocrpt_list *elements;
	uint32_t maxlines;
	uint32_t current_line;
	uint32_t current_line_pushed;
};

struct ocrpt_hline {
	ocrpt_output_type type;
	bool suppress_hline:1;
	double font_width;
	ocrpt_output *output;
	ocrpt_expr *size;
	ocrpt_expr *indent;
	ocrpt_expr *length;
	ocrpt_expr *font_size;
	ocrpt_expr *suppress;
	ocrpt_expr *color;
};

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
	union {
		ocrpt_output_type type;
		ocrpt_output_line_element_type le_type;
	};
	bool in_line:1;
	bool suppress_image:1;
	double start;
	double image_text_width;
	double image_width;
	double image_height;
	ocrpt_color bg;
	ocrpt_output *output;
	ocrpt_expr *suppress;
	ocrpt_expr *value; /* name of the image file */
	ocrpt_expr *imgtype; /* 'png', 'jpg', 'raster', or 'svg' */
	ocrpt_expr *width;
	ocrpt_expr *height;
	ocrpt_expr *align;
	ocrpt_expr *bgcolor;
	ocrpt_expr *text_width;
	ocrpt_image_file *img_file;
};

struct ocrpt_output_element {
	ocrpt_output_type type;
};
typedef struct ocrpt_output_element ocrpt_output_element;

void *ocrpt_layout_new_page(opencreport *o, const ocrpt_paper *paper, bool landscape);
void ocrpt_layout_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width);
void ocrpt_layout_output_evaluate_expr_params(ocrpt_output *output);
void ocrpt_layout_output_resolve(ocrpt_output *output);
void ocrpt_layout_output_evaluate(ocrpt_output *output);

double ocrpt_layout_top_margin(opencreport *o, ocrpt_part *p);
double ocrpt_layout_bottom_margin(opencreport *o, ocrpt_part *p);
double ocrpt_layout_left_margin(opencreport *o, ocrpt_part *p);
double ocrpt_layout_right_margin(opencreport *o, ocrpt_part *p);

static inline void ocrpt_cairo_create(opencreport *o) {
	if (o->current_page) {
		if (o->cr) {
			if (o->drawing_page != o->current_page) {
				cairo_destroy(o->cr);
				o->cr = cairo_create((cairo_surface_t *)o->current_page->data);
				o->drawing_page = o->current_page;
			}
		} else
			o->cr = cairo_create((cairo_surface_t *)o->current_page->data);
	} else {
		if (!o->nullpage_cs)
			o->nullpage_cs = ocrpt_layout_new_page(o, o->paper, false);
		if (o->cr) {
			if (o->drawing_page != o->current_page) {
				cairo_destroy(o->cr);
				o->cr = cairo_create(o->nullpage_cs);
				o->drawing_page = o->current_page;
			}
		} else
			o->cr = cairo_create(o->nullpage_cs);
	}
}

static inline void ocrpt_layout_output_init(ocrpt_output *output) __attribute__((nonnull(1)));
static inline void ocrpt_layout_output_init(ocrpt_output *output) {
	output->has_memo = false;
	output->iter = output->output_list;
	output->current_image = NULL;

	for (ocrpt_list *ol = output->output_list; ol; ol = ol->next) {
		ocrpt_output_element *oe = (ocrpt_output_element *)ol->data;

		switch (oe->type) {
		case OCRPT_OUTPUT_LINE: {
			ocrpt_line *l = (ocrpt_line *)oe;
			l->current_line = 0;
			break;
		}
		default:
			break;
		}
	}
}

static inline void ocrpt_layout_output_position_push(ocrpt_output *output) {
	if (output->iter) {
		ocrpt_output_element *oe = (ocrpt_output_element *)output->iter->data;
		if (oe->type == OCRPT_OUTPUT_LINE) {
			ocrpt_line *l = (ocrpt_line *)oe;
			l->current_line_pushed = l->current_line;
		}
	}
}

static inline void ocrpt_layout_output_position_pop(ocrpt_output *output) {
	if (output->iter) {
		ocrpt_output_element *oe = (ocrpt_output_element *)output->iter->data;
		if (oe->type == OCRPT_OUTPUT_LINE) {
			ocrpt_line *l = (ocrpt_line *)oe;
			l->current_line = l->current_line_pushed;
		}
	}
}

static inline uint32_t ocrpt_layout_get_page_nr(opencreport *o, cairo_surface_t *cs) {
	uint32_t p = 1;
	for (ocrpt_list *pl = o->pages; pl; pl = pl->next, p++)
		if (pl->data == cs)
			return p;
	return 0;
}

ocrpt_expr *ocrpt_layout_expr_parse(opencreport *o, ocrpt_report *r, const char *expr, bool report, bool create_string);
ocrpt_expr *ocrpt_layout_const_expr_parse(opencreport *o, const char *expr, bool fake_vars_expected, bool report);

void ocrpt_layout_output_internal_preamble(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, double page_width, double page_indent, double *page_position);
bool ocrpt_layout_output_internal(bool draw, opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, double page_width, double page_indent, double *page_position);
void ocrpt_layout_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_output *output, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position);
void ocrpt_layout_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position);
void ocrpt_layout_output_highprio_fieldheader(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position);
void ocrpt_output_free(opencreport *o, ocrpt_output *output, bool free_subexprs);
void ocrpt_image_free(const void *ptr);

#endif
