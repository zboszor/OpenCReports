/*
 * OpenCReports PHP module header
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#ifndef PHP_OPENCREPORT_H
#define PHP_OPENCREPORT_H

#include <opencreport.h>

extern zend_module_entry ocrpt_module_entry;
#define phpext_ocrpt_ptr &ocrpt_module_entry

#define PHP_OPENCREPORT_VERSION "0.7.0"

/* Structure for main OpenCReport object. */
typedef struct _php_opencreport_object {
	opencreport *o;
	char *expr_error;
	bool free_me;
	zend_object zo;
} php_opencreport_object;

static inline php_opencreport_object *php_opencreport_from_obj(zend_object *obj) {
	return (php_opencreport_object *)((char *)(obj) - XtOffsetOf(php_opencreport_object, zo));
}

#define Z_OPENCREPORT_P(zv) php_opencreport_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_ds_object {
	ocrpt_datasource *ds;
	zend_object zo;
} php_opencreport_ds_object;

static inline php_opencreport_ds_object *php_opencreport_ds_from_obj(zend_object *obj) {
	return (php_opencreport_ds_object *)((char *)(obj) - XtOffsetOf(php_opencreport_ds_object, zo));
}

#define Z_OPENCREPORT_DS_P(zv) php_opencreport_ds_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_query_object {
	ocrpt_query *q;
	zend_object zo;
} php_opencreport_query_object;

static inline php_opencreport_query_object *php_opencreport_query_from_obj(zend_object *obj) {
	return (php_opencreport_query_object *)((char *)(obj) - XtOffsetOf(php_opencreport_query_object, zo));
}

#define Z_OPENCREPORT_QUERY_P(zv) php_opencreport_query_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_query_result_object {
	ocrpt_query_result *qr;
	zend_long cols;
	zend_object zo;
} php_opencreport_query_result_object;

static inline php_opencreport_query_result_object *php_opencreport_query_result_from_obj(zend_object *obj) {
	return (php_opencreport_query_result_object *)((char *)(obj) - XtOffsetOf(php_opencreport_query_result_object, zo));
}

#define Z_OPENCREPORT_QUERY_RESULT_P(zv) php_opencreport_query_result_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_expr_object {
	ocrpt_expr *e;
	zend_object zo;
} php_opencreport_expr_object;

static inline php_opencreport_expr_object *php_opencreport_expr_from_obj(zend_object *obj) {
	return (php_opencreport_expr_object *)((char *)(obj) - XtOffsetOf(php_opencreport_expr_object, zo));
}

#define Z_OPENCREPORT_EXPR_P(zv) php_opencreport_expr_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_result_object {
	opencreport *o;
	ocrpt_result *r;
	bool freed_by_lib;
	zend_object zo;
} php_opencreport_result_object;

static inline php_opencreport_result_object *php_opencreport_result_from_obj(zend_object *obj) {
	return (php_opencreport_result_object *)((char *)(obj) - XtOffsetOf(php_opencreport_result_object, zo));
}

#define Z_OPENCREPORT_RESULT_P(zv) php_opencreport_result_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_object {
	opencreport *o;
	ocrpt_part *p;
	ocrpt_list *iter;
	bool is_iterator;
	zend_object zo;
} php_opencreport_part_object;

static inline php_opencreport_part_object *php_opencreport_part_from_obj(zend_object *obj) {
	return (php_opencreport_part_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_object, zo));
}

#define Z_OPENCREPORT_PART_P(zv) php_opencreport_part_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_row_object {
	ocrpt_part *p;
	ocrpt_part_row *pr;
	ocrpt_list *iter;
	bool is_iterator;
	zend_object zo;
} php_opencreport_row_object;

static inline php_opencreport_row_object *php_opencreport_row_from_obj(zend_object *obj) {
	return (php_opencreport_row_object *)((char *)(obj) - XtOffsetOf(php_opencreport_row_object, zo));
}

#define Z_OPENCREPORT_ROW_P(zv) php_opencreport_row_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_col_object {
	ocrpt_part_row *pr;
	ocrpt_part_column *pc;
	ocrpt_list *iter;
	bool is_iterator;
	zend_object zo;
} php_opencreport_col_object;

static inline php_opencreport_col_object *php_opencreport_col_from_obj(zend_object *obj) {
	return (php_opencreport_col_object *)((char *)(obj) - XtOffsetOf(php_opencreport_col_object, zo));
}

#define Z_OPENCREPORT_COL_P(zv) php_opencreport_col_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_report_object {
	ocrpt_part_column *pc;
	ocrpt_report *r;
	char *expr_error;
	ocrpt_list *iter;
	bool is_iterator;
	zend_object zo;
} php_opencreport_report_object;

static inline php_opencreport_report_object *php_opencreport_report_from_obj(zend_object *obj) {
	return (php_opencreport_report_object *)((char *)(obj) - XtOffsetOf(php_opencreport_report_object, zo));
}

#define Z_OPENCREPORT_REPORT_P(zv) php_opencreport_report_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_variable_object {
	ocrpt_var *v;
	zend_object zo;
} php_opencreport_variable_object;

static inline php_opencreport_variable_object *php_opencreport_variable_from_obj(zend_object *obj) {
	return (php_opencreport_variable_object *)((char *)(obj) - XtOffsetOf(php_opencreport_variable_object, zo));
}

#define Z_OPENCREPORT_VARIABLE_P(zv) php_opencreport_variable_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_break_object {
	ocrpt_report *r;
	ocrpt_break *br;
	ocrpt_list *iter;
	bool is_iterator;
	zend_object zo;
} php_opencreport_break_object;

static inline php_opencreport_break_object *php_opencreport_break_from_obj(zend_object *obj) {
	return (php_opencreport_break_object *)((char *)(obj) - XtOffsetOf(php_opencreport_break_object, zo));
}

#define Z_OPENCREPORT_BREAK_P(zv) php_opencreport_break_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_output_object {
	ocrpt_output *output;
	zend_object zo;
} php_opencreport_output_object;

static inline php_opencreport_output_object *php_opencreport_output_from_obj(zend_object *obj) {
	return (php_opencreport_output_object *)((char *)(obj) - XtOffsetOf(php_opencreport_output_object, zo));
}

#define Z_OPENCREPORT_OUTPUT_P(zv) php_opencreport_output_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_line_object {
	ocrpt_line *line;
	zend_object zo;
} php_opencreport_line_object;

static inline php_opencreport_line_object *php_opencreport_line_from_obj(zend_object *obj) {
	return (php_opencreport_line_object *)((char *)(obj) - XtOffsetOf(php_opencreport_line_object, zo));
}

#define Z_OPENCREPORT_LINE_P(zv) php_opencreport_line_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_hline_object {
	ocrpt_hline *hline;
	zend_object zo;
} php_opencreport_hline_object;

static inline php_opencreport_hline_object *php_opencreport_hline_from_obj(zend_object *obj) {
	return (php_opencreport_hline_object *)((char *)(obj) - XtOffsetOf(php_opencreport_hline_object, zo));
}

#define Z_OPENCREPORT_HLINE_P(zv) php_opencreport_hline_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_image_object {
	ocrpt_image *image;
	zend_object zo;
} php_opencreport_image_object;

static inline php_opencreport_image_object *php_opencreport_image_from_obj(zend_object *obj) {
	return (php_opencreport_image_object *)((char *)(obj) - XtOffsetOf(php_opencreport_image_object, zo));
}

#define Z_OPENCREPORT_IMAGE_P(zv) php_opencreport_image_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_text_object {
	ocrpt_text *text;
	zend_object zo;
} php_opencreport_text_object;

static inline php_opencreport_text_object *php_opencreport_text_from_obj(zend_object *obj) {
	return (php_opencreport_text_object *)((char *)(obj) - XtOffsetOf(php_opencreport_text_object, zo));
}

#define Z_OPENCREPORT_TEXT_P(zv) php_opencreport_text_from_obj(Z_OBJ_P((zv)))

#endif /* PHP_OPENCREPORT_H */
