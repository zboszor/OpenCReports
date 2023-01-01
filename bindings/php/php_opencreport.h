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
	ocrpt_list *assoc_objs;
	ocrpt_list *assoc_objs_last;
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
	php_opencreport_object *oo;
	ocrpt_list *assoc_objs;
	ocrpt_list *assoc_objs_last;
	zend_object zo;
} php_opencreport_ds_object;

static inline php_opencreport_ds_object *php_opencreport_ds_from_obj(zend_object *obj) {
	return (php_opencreport_ds_object *)((char *)(obj) - XtOffsetOf(php_opencreport_ds_object, zo));
}

#define Z_OPENCREPORT_DS_P(zv) php_opencreport_ds_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_query_object {
	ocrpt_query *q;
	php_opencreport_object *oo;
	zend_object zo;
} php_opencreport_query_object;

static inline php_opencreport_query_object *php_opencreport_query_from_obj(zend_object *obj) {
	return (php_opencreport_query_object *)((char *)(obj) - XtOffsetOf(php_opencreport_query_object, zo));
}

#define Z_OPENCREPORT_QUERY_P(zv) php_opencreport_query_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_expr_object {
	ocrpt_expr *e;
	php_opencreport_object *oo;
	bool has_parent;
	zend_object zo;
} php_opencreport_expr_object;

static inline php_opencreport_expr_object *php_opencreport_expr_from_obj(zend_object *obj) {
	return (php_opencreport_expr_object *)((char *)(obj) - XtOffsetOf(php_opencreport_expr_object, zo));
}

#define Z_OPENCREPORT_EXPR_P(zv) php_opencreport_expr_from_obj(Z_OBJ_P((zv)))

typedef struct _php_opencreport_result_object {
	ocrpt_result *r;
	php_opencreport_object *oo;
	bool has_parent;
	bool freed_by_lib;
	zend_object zo;
} php_opencreport_result_object;

static inline php_opencreport_result_object *php_opencreport_result_from_obj(zend_object *obj) {
	return (php_opencreport_result_object *)((char *)(obj) - XtOffsetOf(php_opencreport_result_object, zo));
}

#define Z_OPENCREPORT_RESULT_P(zv) php_opencreport_result_from_obj(Z_OBJ_P((zv)))

#endif /* PHP_OPENCREPORT_H */
