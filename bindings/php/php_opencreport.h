/*
 * OpenCReports PHP module header
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#ifndef PHP_OPENCREPORT_H
#define PHP_OPENCREPORT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <opencreport.h>

/* Structure for main OpenCReport object. */
typedef struct _php_opencreport_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	opencreport *o;
	char *expr_error;
	bool free_me;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_object;

static inline php_opencreport_object *php_opencreport_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_object *)((char *)(obj) - XtOffsetOf(php_opencreport_object, zo));
#else
	return (php_opencreport_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_P(zv) php_opencreport_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_P(zv) ((php_opencreport_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_ds_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_datasource *ds;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_ds_object;

static inline php_opencreport_ds_object *php_opencreport_ds_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_ds_object *)((char *)(obj) - XtOffsetOf(php_opencreport_ds_object, zo));
#else
	return (php_opencreport_ds_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_DS_P(zv) php_opencreport_ds_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_DS_P(zv) ((php_opencreport_ds_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_query_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_query *q;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_query_object;

static inline php_opencreport_query_object *php_opencreport_query_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_query_object *)((char *)(obj) - XtOffsetOf(php_opencreport_query_object, zo));
#else
	return (php_opencreport_query_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_QUERY_P(zv) php_opencreport_query_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_QUERY_P(zv) ((php_opencreport_query_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_query_result_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_query_result *qr;
#if PHP_VERSION_ID >= 70000
	zend_long cols;
	zend_object zo;
#else
	long cols;
#endif
} php_opencreport_query_result_object;

static inline php_opencreport_query_result_object *php_opencreport_query_result_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_query_result_object *)((char *)(obj) - XtOffsetOf(php_opencreport_query_result_object, zo));
#else
	return (php_opencreport_query_result_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_QUERY_RESULT_P(zv) php_opencreport_query_result_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_QUERY_RESULT_P(zv) ((php_opencreport_query_result_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_expr_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_expr *e;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_expr_object;

static inline php_opencreport_expr_object *php_opencreport_expr_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_expr_object *)((char *)(obj) - XtOffsetOf(php_opencreport_expr_object, zo));
#else
	return (php_opencreport_expr_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_EXPR_P(zv) php_opencreport_expr_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_EXPR_P(zv) ((php_opencreport_expr_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_result_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	opencreport *o;
	ocrpt_result *r;
	bool freed_by_lib;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_result_object;

static inline php_opencreport_result_object *php_opencreport_result_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_result_object *)((char *)(obj) - XtOffsetOf(php_opencreport_result_object, zo));
#else
	return (php_opencreport_result_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_RESULT_P(zv) php_opencreport_result_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_RESULT_P(zv) ((php_opencreport_result_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_part_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	opencreport *o;
	ocrpt_part *p;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_part_object;

static inline php_opencreport_part_object *php_opencreport_part_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_part_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_object, zo));
#else
	return (php_opencreport_part_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_PART_P(zv) php_opencreport_part_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_PART_P(zv) ((php_opencreport_part_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_row_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_part *p;
	ocrpt_part_row *pr;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_row_object;

static inline php_opencreport_row_object *php_opencreport_row_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_row_object *)((char *)(obj) - XtOffsetOf(php_opencreport_row_object, zo));
#else
	return (php_opencreport_row_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_ROW_P(zv) php_opencreport_row_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_ROW_P(zv) ((php_opencreport_row_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_col_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_part_row *pr;
	ocrpt_part_column *pc;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_col_object;

static inline php_opencreport_col_object *php_opencreport_col_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_col_object *)((char *)(obj) - XtOffsetOf(php_opencreport_col_object, zo));
#else
	return (php_opencreport_col_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_COL_P(zv) php_opencreport_col_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_COL_P(zv) ((php_opencreport_col_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_report_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_part_column *pc;
	ocrpt_report *r;
	char *expr_error;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_report_object;

static inline php_opencreport_report_object *php_opencreport_report_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_report_object *)((char *)(obj) - XtOffsetOf(php_opencreport_report_object, zo));
#else
	return (php_opencreport_report_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_REPORT_P(zv) php_opencreport_report_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_REPORT_P(zv) ((php_opencreport_report_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_variable_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_var *v;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_variable_object;

static inline php_opencreport_variable_object *php_opencreport_variable_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_variable_object *)((char *)(obj) - XtOffsetOf(php_opencreport_variable_object, zo));
#else
	return (php_opencreport_variable_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_VARIABLE_P(zv) php_opencreport_variable_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_VARIABLE_P(zv) ((php_opencreport_variable_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_break_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_report *r;
	ocrpt_break *br;
	ocrpt_list *iter;
	bool is_iterator;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_break_object;

static inline php_opencreport_break_object *php_opencreport_break_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_break_object *)((char *)(obj) - XtOffsetOf(php_opencreport_break_object, zo));
#else
	return (php_opencreport_break_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_BREAK_P(zv) php_opencreport_break_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_BREAK_P(zv) ((php_opencreport_break_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_output_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_output *output;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_output_object;

static inline php_opencreport_output_object *php_opencreport_output_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_output_object *)((char *)(obj) - XtOffsetOf(php_opencreport_output_object, zo));
#else
	return (php_opencreport_output_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_OUTPUT_P(zv) php_opencreport_output_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_OUTPUT_P(zv) ((php_opencreport_output_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_line_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_line *line;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_line_object;

static inline php_opencreport_line_object *php_opencreport_line_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_line_object *)((char *)(obj) - XtOffsetOf(php_opencreport_line_object, zo));
#else
	return (php_opencreport_line_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_LINE_P(zv) php_opencreport_line_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_LINE_P(zv) ((php_opencreport_line_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_hline_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_hline *hline;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_hline_object;

static inline php_opencreport_hline_object *php_opencreport_hline_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_hline_object *)((char *)(obj) - XtOffsetOf(php_opencreport_hline_object, zo));
#else
	return (php_opencreport_hline_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_HLINE_P(zv) php_opencreport_hline_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_HLINE_P(zv) ((php_opencreport_hline_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_image_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_image *image;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_image_object;

static inline php_opencreport_image_object *php_opencreport_image_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_image_object *)((char *)(obj) - XtOffsetOf(php_opencreport_image_object, zo));
#else
	return (php_opencreport_image_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_IMAGE_P(zv) php_opencreport_image_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_IMAGE_P(zv) ((php_opencreport_image_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_text_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_text *text;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_text_object;

static inline php_opencreport_text_object *php_opencreport_text_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_text_object *)((char *)(obj) - XtOffsetOf(php_opencreport_text_object, zo));
#else
	return (php_opencreport_text_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_TEXT_P(zv) php_opencreport_text_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_TEXT_P(zv) ((php_opencreport_text_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

typedef struct _php_opencreport_barcode_object {
#if PHP_VERSION_ID < 70000
	zend_object zo;
#endif
	ocrpt_barcode *bc;
#if PHP_VERSION_ID >= 70000
	zend_object zo;
#endif
} php_opencreport_barcode_object;

static inline php_opencreport_barcode_object *php_opencreport_barcode_from_obj(zend_object *obj) {
#if PHP_VERSION_ID >= 70000
	return (php_opencreport_barcode_object *)((char *)(obj) - XtOffsetOf(php_opencreport_barcode_object, zo));
#else
	return (php_opencreport_barcode_object *)obj;
#endif
}

#if PHP_VERSION_ID >= 70000
#define Z_OPENCREPORT_BARCODE_P(zv) php_opencreport_barcode_from_obj(Z_OBJ_P((zv)))
#else
#define Z_OPENCREPORT_BARCODE_P(zv) ((php_opencreport_barcode_object *)zend_object_store_get_object(zv TSRMLS_CC))
#endif

#endif /* PHP_OPENCREPORT_H */
