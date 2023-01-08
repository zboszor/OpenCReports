/*
 * OpenCReports PHP module header
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#ifndef PHP_OPENCREPORT_H
#define PHP_OPENCREPORT_H

#include <opencreport.h>

extern zend_module_entry ocrpt_module_entry;
#define phpext_ocrpt_ptr &ocrpt_module_entry

#define PHP_OPENCREPORT_VERSION "0.0.1"

/* Structure for main OpenCReport object. */
typedef struct _php_opencreport_object {
	opencreport *o;
	char *expr_error;
	ocrpt_list *funcnames;
	ocrpt_list *funcnames_last;
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
	int32_t cols;
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
	ocrpt_result *r;
	bool freed_by_lib;
	zend_object zo;
} php_opencreport_result_object;

static inline php_opencreport_result_object *php_opencreport_result_from_obj(zend_object *obj) {
	return (php_opencreport_result_object *)((char *)(obj) - XtOffsetOf(php_opencreport_result_object, zo));
}

#define Z_OPENCREPORT_RESULT_P(zv) php_opencreport_result_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_object {
	ocrpt_part *p;
	zend_object zo;
} php_opencreport_part_object;

static inline php_opencreport_part_object *php_opencreport_part_from_obj(zend_object *obj) {
	return (php_opencreport_part_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_object, zo));
}

#define Z_OPENCREPORT_PART_P(zv) php_opencreport_part_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_iter_object {
	opencreport *o;
	ocrpt_part *p;
	ocrpt_list *iter;
	zend_object zo;
} php_opencreport_part_iter_object;

static inline php_opencreport_part_iter_object *php_opencreport_part_iter_from_obj(zend_object *obj) {
	return (php_opencreport_part_iter_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_iter_object, zo));
}

#define Z_OPENCREPORT_PART_ITER_P(zv) php_opencreport_part_iter_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_row_object {
	ocrpt_part_row *pr;
	zend_object zo;
} php_opencreport_part_row_object;

static inline php_opencreport_part_row_object *php_opencreport_part_row_from_obj(zend_object *obj) {
	return (php_opencreport_part_row_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_row_object, zo));
}

#define Z_OPENCREPORT_PART_ROW_P(zv) php_opencreport_part_row_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_row_iter_object {
	ocrpt_part *p;
	ocrpt_part_row *pr;
	ocrpt_list *iter;
	zend_object zo;
} php_opencreport_part_row_iter_object;

static inline php_opencreport_part_row_iter_object *php_opencreport_part_row_iter_from_obj(zend_object *obj) {
	return (php_opencreport_part_row_iter_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_row_iter_object, zo));
}

#define Z_OPENCREPORT_PART_ROW_ITER_P(zv) php_opencreport_part_row_iter_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_col_object {
	ocrpt_part_column *pc;
	zend_object zo;
} php_opencreport_part_col_object;

static inline php_opencreport_part_col_object *php_opencreport_part_col_from_obj(zend_object *obj) {
	return (php_opencreport_part_col_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_col_object, zo));
}

#define Z_OPENCREPORT_PART_COL_P(zv) php_opencreport_part_col_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_col_iter_object {
	ocrpt_part_row *pr;
	ocrpt_part_column *pc;
	ocrpt_list *iter;
	zend_object zo;
} php_opencreport_part_col_iter_object;

static inline php_opencreport_part_col_iter_object *php_opencreport_part_col_iter_from_obj(zend_object *obj) {
	return (php_opencreport_part_col_iter_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_col_iter_object, zo));
}

#define Z_OPENCREPORT_PART_COL_ITER_P(zv) php_opencreport_part_col_iter_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_report_object {
	ocrpt_report *r;
	zend_object zo;
} php_opencreport_part_report_object;

static inline php_opencreport_part_report_object *php_opencreport_part_report_from_obj(zend_object *obj) {
	return (php_opencreport_part_report_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_report_object, zo));
}

#define Z_OPENCREPORT_PART_REPORT_P(zv) php_opencreport_part_report_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_part_report_iter_object {
	ocrpt_part_column *pc;
	ocrpt_report *r;
	ocrpt_list *iter;
	zend_object zo;
} php_opencreport_part_report_iter_object;

static inline php_opencreport_part_report_iter_object *php_opencreport_part_report_iter_from_obj(zend_object *obj) {
	return (php_opencreport_part_report_iter_object *)((char *)(obj) - XtOffsetOf(php_opencreport_part_report_iter_object, zo));
}

#define Z_OPENCREPORT_PART_REPORT_ITER_P(zv) php_opencreport_part_report_iter_from_obj(Z_OBJ_P((zv)))

#endif /* PHP_OPENCREPORT_H */
