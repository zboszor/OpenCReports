/*
 * OpenCReports main header
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORT_H_
#define _OPENCREPORT_H_

#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <mpfr.h>

struct opencreport;
typedef struct opencreport opencreport;

struct ocrpt_query;
typedef struct ocrpt_query ocrpt_query;

struct ocrpt_datasource;
typedef struct ocrpt_datasource ocrpt_datasource;

struct ocrpt_query_result;
typedef struct ocrpt_query_result ocrpt_query_result;

enum ocrpt_input_type {
	OCRPT_INPUT_ARRAY,
	OCRPT_INPUT_CSV,
	OCRPT_INPUT_JSON,
	OCRPT_INPUT_XML,
	OCRPT_INPUT_POSTGRESQL,
	OCRPT_INPUT_MARIADB,
	OCRPT_INPUT_ODBC
};
typedef enum ocrpt_input_type ocrpt_input_type;

struct ocrpt_input {
	ocrpt_input_type type;
	void (*describe)(ocrpt_query *, ocrpt_query_result **, int32_t *);
	void (*rewind)(ocrpt_query *);
	bool (*next)(ocrpt_query *);
	bool (*populate_result)(ocrpt_query *);
	bool (*isdone)(ocrpt_query *);
	void (*free)(ocrpt_query *);
	bool (*set_encoding)(ocrpt_datasource *, const char *);
	void (*close)(const ocrpt_datasource *);
};
typedef struct ocrpt_input ocrpt_input;

struct ocrpt_output;
typedef struct ocrpt_output ocrpt_output;

struct ocrpt_paper {
	const char *name;
	double width;
	double height;
};
typedef struct ocrpt_paper ocrpt_paper;

enum ocrpt_break_attr_type {
	OCRPT_BREAK_ATTR_NEWPAGE,
	OCRPT_BREAK_ATTR_HEADERNEWPAGE,
	OCRPT_BREAK_ATTR_SUPPRESSBLANK,
	OCRPT_BREAK_ATTRS_COUNT
};
typedef enum ocrpt_break_attr_type ocrpt_break_attr_type;

struct ocrpt_break;
typedef struct ocrpt_break ocrpt_break;

enum ocrpt_var_type {
	OCRPT_VARIABLE_UNKNOWN,
	OCRPT_VARIABLE_EXPRESSION,
	OCRPT_VARIABLE_COUNT,
	OCRPT_VARIABLE_COUNTALL,
	OCRPT_VARIABLE_SUM,
	OCRPT_VARIABLE_AVERAGE,
	OCRPT_VARIABLE_AVERAGEALL,
	OCRPT_VARIABLE_LOWEST,
	OCRPT_VARIABLE_HIGHEST,
	OCRPT_VARIABLE_CUSTOM
};
typedef enum ocrpt_var_type ocrpt_var_type;

struct ocrpt_var;
typedef struct ocrpt_var ocrpt_var;

struct ocrpt_report;
typedef struct ocrpt_report ocrpt_report;

struct ocrpt_part_row_data;
typedef struct ocrpt_part_row_data ocrpt_part_row_data;

struct ocrpt_part_row;
typedef struct ocrpt_part_row ocrpt_part_row;

struct ocrpt_part;
typedef struct ocrpt_part ocrpt_part;

typedef void (*ocrpt_break_trigger_cb)(opencreport *, ocrpt_report *, ocrpt_break *, void *);
typedef void (*ocrpt_report_cb)(opencreport *, ocrpt_report *, void *data);
typedef void (*ocrpt_part_cb)(opencreport *, ocrpt_part *, void *data);
typedef void (*ocrpt_cb)(opencreport *, void *data);

struct ocrpt_string {
	char *str;
	size_t allocated_len;
	size_t len;
};
typedef struct ocrpt_string ocrpt_string;

enum ocrpt_result_type {
	OCRPT_RESULT_ERROR,
	OCRPT_RESULT_STRING,
	OCRPT_RESULT_NUMBER,
	OCRPT_RESULT_DATETIME
};

struct ocrpt_result;
typedef struct ocrpt_result ocrpt_result;

enum ocrpt_varref_type {
	OCRPT_VARREF_MVAR  = (1 << 0),
	OCRPT_VARREF_RVAR  = (1 << 1),
	OCRPT_VARREF_IDENT = (1 << 2),
	OCRPT_VARREF_VVAR  = (1 << 3)
};

#define OCRPT_EXPR_RESULTS (3)
#define OCRPT_MPFR_PRECISION_BITS	(256)

struct ocrpt_expr;
typedef struct ocrpt_expr ocrpt_expr;

struct ocrpt_function;
typedef struct ocrpt_function ocrpt_function;

struct ocrpt_list;
typedef struct ocrpt_list ocrpt_list;

#define OCRPT_UNUSED_PARAM __attribute__((unused))

#define OCRPT_FUNCTION_PARAMS opencreport *o, ocrpt_report *r OCRPT_UNUSED_PARAM, ocrpt_expr *e
#define OCRPT_FUNCTION(name) void name(OCRPT_FUNCTION_PARAMS)
#define OCRPT_STATIC_FUNCTION(name) static void name(OCRPT_FUNCTION_PARAMS)
typedef void (*ocrpt_function_call)(OCRPT_FUNCTION_PARAMS);

struct ocrpt_color {
	double r;
	double g;
	double b;
};
typedef struct ocrpt_color ocrpt_color;

/***********************************
 *                                 *
 *   H I G H   L E V E L   A P I   *
 *                                 *
 ***********************************/

/*
 * Create a new empty OpenCReports structure
 */
opencreport *ocrpt_init(void);
/*
 * Free an OpenCReports structure
 */
void ocrpt_free(opencreport *o);
/*
 * Add details to the report from the parsed XML file
 */
bool ocrpt_parse_xml(opencreport *o, const char *filename);
/*
 * Execute the reports added up to this point.
 */
bool ocrpt_execute(opencreport *o);
/*
 * Send the output to stdout
 */
void ocrpt_spool(opencreport *o);
/*
 * Set output format
 */
enum ocrpt_format_type {
	OCRPT_OUTPUT_UNSET,
	OCRPT_OUTPUT_PDF,
	OCRPT_OUTPUT_HTML,
	OCRPT_OUTPUT_TXT,
	OCRPT_OUTPUT_CSV,
	OCRPT_OUTPUT_XML
};
typedef enum ocrpt_format_type ocrpt_format_type;

void ocrpt_set_output_format(opencreport *o, ocrpt_format_type format);

/*********************************
 *                               *
 *   L O W   L E V E L   A P I   *
 *                               *
 *********************************/

/*
 * Numeric fine-tuning
 */
void ocrpt_set_numeric_precision_bits(opencreport *o, mpfr_prec_t prec);
void ocrpt_set_rounding_mode(opencreport *o, mpfr_rnd_t rndmode);

/****************************
 * Locale related functions *
 ****************************/

/*
 * Set locale for the report
 * It does not affect the main program.
 */
void ocrpt_set_locale(opencreport *o, const char *locale);
/*
 * Get locale for the report
 */
locale_t ocrpt_get_locale(opencreport *o);
/*
 * Print monetary value in the report's locale
 * It supports printing mpfr_t (high precision) numeric values.
 */
ssize_t ocrpt_mpfr_strfmon(opencreport *o, char * __restrict s, size_t maxsize, const char * __restrict format, ...);

/********************************
 * Expression related functions *
 ********************************/

/*
 * Create an expression parse tree from an expression string
 */
ocrpt_expr *ocrpt_expr_parse(opencreport *o, ocrpt_report *r, const char *str, char **err);
/*
 * Free an expression parse tree
 */
void ocrpt_expr_free(opencreport *o, ocrpt_report *r, ocrpt_expr *e);
/*
 * Resolve variable references in the expression
 */
void ocrpt_expr_resolve(opencreport *o, ocrpt_report *r, ocrpt_expr *e);
/*
 * Optimize expression after parsing
 */
void ocrpt_expr_optimize(opencreport *o, ocrpt_report *r, ocrpt_expr *e);
/*
 * Evaluate the expression, i.e. compute its ocrpt_result
 * It must be called after ocrpt_query_navigate_next(), see below.
 *
 * The returned ocrpt_result MUST NOT be freed with ocrpt_result_free().
 */
ocrpt_result *ocrpt_expr_eval(opencreport *o, ocrpt_report *r, ocrpt_expr *e);
/*
 * Get the current ocrpt_result without (re-)evaluation
 *
 * The returned ocrpt_result MUST NOT be freed with ocrpt_result_free().
 */
ocrpt_result *ocrpt_expr_get_result(opencreport *o, ocrpt_expr *e);
/*
 * Print an expression on stdout. Good for unit testing.
 */
void ocrpt_expr_print(opencreport *o, ocrpt_expr *e);
/*
 * Print the result data for every subexpression in the expression. Good for unit testing.
 */
void ocrpt_expr_result_deep_print(opencreport *o, ocrpt_expr *e);
/*
 * Count the number of expression nodes
 */
int32_t ocrpt_expr_nodes(ocrpt_expr *e);
/*
 * Initialize the "current" expression result to the specified type
 * Mainly used in functions
 */
bool ocrpt_expr_init_result(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type);
/*
 * Initialize expression results to the specified type
 * Mainly used in functions
 */
void ocrpt_expr_init_results(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type);
/*
 * Set or initialize the result of the expression as an error with the specified message
 */
ocrpt_result *ocrpt_expr_make_error_result(opencreport *o, ocrpt_expr *e, const char *format, ...);

///////////////////////// XXXXXXXXXXXXXXXXXXXXXX
/*
 * Set whether the start value for iterative expressions
 * is the initial value or computed
 */
void ocrpt_expr_set_iterative_start_value(ocrpt_expr *e, bool start_with_init);
/*
 * Get/set the basic type value for ocrpt_expr
 */
const char *ocrpt_expr_get_string_value(opencreport *o, ocrpt_expr *e);
void ocrpt_expr_set_string_value(opencreport *o, ocrpt_expr *e, const char *s);
void ocrpt_expr_set_nth_result_string_value(opencreport *o, ocrpt_expr *e, int which, const char *s);
long ocrpt_expr_get_long_value(opencreport *o, ocrpt_expr *e);
void ocrpt_expr_set_long_value(opencreport *o, ocrpt_expr *e, long l);
void ocrpt_expr_set_nth_result_long_value(opencreport *o, ocrpt_expr *e, int which, long l);
double ocrpt_expr_get_double_value(opencreport *o, ocrpt_expr *e);
void ocrpt_expr_set_double_value(opencreport *o, ocrpt_expr *e, double d);
void ocrpt_expr_set_nth_result_double_value(opencreport *o, ocrpt_expr *e, int which, double d);
/*
 * Get the basic data type for ocrpt_result
 */
bool ocrpt_result_isnull(ocrpt_result *result);
bool ocrpt_result_isnumber(ocrpt_result *result);
ocrpt_string *ocrpt_result_get_string(ocrpt_result *result);
mpfr_ptr ocrpt_result_get_number(ocrpt_result *result);
/*
 * Compare two subsequent row data in the expression,
 * return true if they are identical.
 * It can be used to implement report breaks.
 */
bool ocrpt_expr_cmp_results(opencreport *o, ocrpt_expr *e);
/*
 * Set delayed property of the expression
 */
void ocrpt_expr_set_delayed(opencreport *o, ocrpt_expr *e, bool delayed);
/*
 * Set the r.value reference
 */
void ocrpt_expr_set_field_expr(opencreport *o, ocrpt_expr *e, ocrpt_expr *rvalue);

/******************************
 * Variable related functions *
 ******************************/

/*
 * Create a named report variable
 */
ocrpt_var *ocrpt_variable_new(opencreport *o, ocrpt_report *r, ocrpt_var_type type, const char *name, const char *expr, const char *reset_on_break_name);
/*
 * Create a names custom variable
 */
ocrpt_var *ocrpt_variable_new_full(opencreport *o, ocrpt_report *r, enum ocrpt_result_type type, const char *name, const char *baseexpr, const char *intermedexpr, const char *intermed2expr, const char *resultexpr, const char *reset_on_break_name);
/*
 * Get various variable subexpressions 
 */
ocrpt_expr *ocrpt_variable_baseexpr(ocrpt_var *v);
ocrpt_expr *ocrpt_variable_intermedexpr(ocrpt_var *v);
ocrpt_expr *ocrpt_variable_intermed2expr(ocrpt_var *v);
ocrpt_expr *ocrpt_variable_resultexpr(ocrpt_var *v);
/*
 * Set precalculate property
 * It will imply delayed="yes" for expressions using this variable
 */
void ocrpt_variable_set_precalculate(ocrpt_var *var, bool value);
/*
 * Resolve a variable
 */
void ocrpt_variable_resolve(opencreport *o, ocrpt_report *r, ocrpt_var *v);
/*
 * Evaluate a variable
 */
void ocrpt_variable_evaluate(opencreport *o, ocrpt_report *r, ocrpt_var *v);
/*
 * Create an ocrpt_result structure
 * Must be freed with ocrpt_result_free().
 */
ocrpt_result *ocrpt_result_new(void);
/*
 * Get result type
 */
enum ocrpt_result_type ocrpt_result_get_type(ocrpt_result *result);
/*
 * Copy result value
 */
void ocrpt_result_copy(opencreport *o, ocrpt_result *dst, ocrpt_result *src);
/*
 * Print the result data. Good for unit testing.
 */
void ocrpt_result_print(ocrpt_result *r);
/*
 * Free an ocrpt_result structure
 */
void ocrpt_result_free(ocrpt_result *r);

/******************************
 * Function related functions *
 ******************************/

/*
 * Add a new custom function to the opencreport structure
 */
bool ocrpt_function_add(opencreport *o, const char *fname, ocrpt_function_call func,
						int32_t n_ops, bool commutative, bool associative,
						bool left_associative, bool dont_optimize);
/*
 * Find a named function
 */
const ocrpt_function *ocrpt_function_get(opencreport *o, const char *fname);
/*
 * Return the number of operands of an expression
 *
 * Expressions may be function calls with operands.
 * It can be used by custom functions to inspect whether
 * the number of operands is in the expected range.
 */
int32_t ocrpt_expr_get_num_operands(ocrpt_expr *e);
/*
 * Return the expression's nth operand's current result
 *
 * It can be used by custom functions.
 */
ocrpt_result *ocrpt_expr_operand_get_result(opencreport *o, ocrpt_expr *e, int32_t opnum);

/*********************************
 * Environment related functions *
 *********************************/

typedef ocrpt_result *(*ocrpt_environment_query_func)(const char *);
extern ocrpt_environment_query_func ocrpt_environment_get;

/*
 * Set the global environment getter function
 */
void ocrpt_environment_set_query_func(ocrpt_environment_query_func func);

/*
 * Returns the value of an environment variable
 * The returned ocrpt_result pointer must be freed with ocrpt_result_free()
 */
ocrpt_result *ocrpt_environment_get_c(const char *env);

/***************************
 * Break related functions *
 ***************************/

/*
 * Add a named report break
 */
ocrpt_break *ocrpt_break_new(opencreport *o, ocrpt_report *r, const char *name);
/*
 * Free a report break
 */
void ocrpt_break_free(opencreport *o, ocrpt_report *r, ocrpt_break *br);
/*
 * Free all report breaks
 */
void ocrpt_breaks_free(opencreport *o, ocrpt_report *r);
/*
 * Validate a break pointer for o and r
 */
bool ocrpt_break_validate(opencreport *o, ocrpt_report *r, ocrpt_break *br);
/*
 * Set the break attribute from bool value
 */
bool ocrpt_break_set_attribute(ocrpt_break *br, const ocrpt_break_attr_type attr_type, bool value);
/*
 * Set the break attribute from numeric constant expression
 */
bool ocrpt_break_set_attribute_from_expr(opencreport *o, ocrpt_report *r, ocrpt_break *br, const ocrpt_break_attr_type attr_type, ocrpt_expr *expr);
/*
 * Find a report break using its name
 */
ocrpt_break *ocrpt_break_get(opencreport *o, ocrpt_report *r, const char *name);
/*
 * Get break name
 */
const char *ocrpt_break_get_name(ocrpt_break *br);
/*
 * Iterate through the report's breaks
 */
ocrpt_break *ocrpt_break_get_next(ocrpt_report *r, ocrpt_list **list);
/*
 * Add a break field to a break
 * This function takes over ownership of the breakfield expression
 */
bool ocrpt_break_add_breakfield(opencreport *o, ocrpt_report *r, ocrpt_break *br, ocrpt_expr *bf);
/*
 * Resolve and optimize break fields
 */
void ocrpt_break_resolve_fields(opencreport *o, ocrpt_report *r, ocrpt_break *br);
/*
 * Check whether the report break triggers
 */
bool ocrpt_break_check_fields(opencreport *o, ocrpt_report *r, ocrpt_break *br);
/*
 * Reset variables for the break
 */
void ocrpt_break_reset_vars(opencreport *o, ocrpt_report *r, ocrpt_break *br);
/*
 * Add break trigger callback
 */
bool ocrpt_break_add_trigger_cb(opencreport *o, ocrpt_report *r, ocrpt_break *br, ocrpt_break_trigger_cb func, void *data);

/****************************
 * Memory handling wrappers *
 ****************************/

typedef void *(*ocrpt_mem_malloc_t)(size_t);
typedef void *(*ocrpt_mem_realloc_t)(void *, size_t);
typedef void *(*ocrpt_mem_reallocarray_t)(void *, size_t, size_t);
typedef void (*ocrpt_mem_free_t)(const void *);
typedef char *(*ocrpt_mem_strdup_t)(const char *);
typedef char *(*ocrpt_mem_strndup_t)(const char *, size_t);
/* Dummy type, only used for casting ocrpt_mem_free_t for mp_set_memory_functions() */
typedef void (*ocrpt_mem_free_size_t)(void *, size_t);

extern ocrpt_mem_malloc_t ocrpt_mem_malloc0;
extern ocrpt_mem_realloc_t ocrpt_mem_realloc0;
extern ocrpt_mem_reallocarray_t ocrpt_mem_reallocarray0;
extern ocrpt_mem_free_t ocrpt_mem_free0;
extern ocrpt_mem_strdup_t ocrpt_mem_strdup0;
extern ocrpt_mem_strndup_t ocrpt_mem_strndup0;

static inline void *ocrpt_mem_malloc(size_t sz) __attribute__((malloc,alloc_size(1)));
static inline void *ocrpt_mem_malloc(size_t sz) { return ocrpt_mem_malloc0(sz); }

static inline void *ocrpt_mem_realloc(void *ptr, size_t sz) __attribute__((alloc_size(2)));
static inline void *ocrpt_mem_realloc(void *ptr, size_t sz) { return ocrpt_mem_realloc0(ptr, sz); }

static inline void *ocrpt_mem_reallocarray(void *ptr, size_t sz, size_t) __attribute__((alloc_size(2)));
static inline void *ocrpt_mem_reallocarray(void *ptr, size_t nmemb, size_t sz) { return ocrpt_mem_reallocarray0(ptr, nmemb, sz); }

static inline void ocrpt_mem_free(const void *ptr) { ocrpt_mem_free0(ptr); }
static inline void ocrpt_strfree(const char *s) { ocrpt_mem_free0((const void *)s); }

static inline void *ocrpt_mem_strdup(const char *ptr) { return (ptr ? ocrpt_mem_strdup0(ptr) : NULL); }

static inline void *ocrpt_mem_strndup(const char *ptr, size_t sz) { return (ptr ? ocrpt_mem_strndup0(ptr, sz) : NULL); }

void ocrpt_mem_set_alloc_funcs(ocrpt_mem_malloc_t rmalloc,
								ocrpt_mem_realloc_t rrealloc,
								ocrpt_mem_reallocarray_t rreallocarray,
								ocrpt_mem_free_t rfree,
								ocrpt_mem_strdup_t rstrdup,
								ocrpt_mem_strndup_t rstrndup);

/*
 * List related functions
 */
#define ocrpt_list_length(l) ((l) ? (l)->len : 0)
ocrpt_list *ocrpt_makelist1(const void *data);
ocrpt_list *ocrpt_makelist(const void *data1, ...);
ocrpt_list *ocrpt_list_last(const ocrpt_list *l);
ocrpt_list *ocrpt_list_nth(const ocrpt_list *l, uint32_t n);
ocrpt_list *ocrpt_list_end_append(ocrpt_list *l, ocrpt_list **e, const void *data);
ocrpt_list *ocrpt_list_append(ocrpt_list *l, const void *data);
ocrpt_list *ocrpt_list_prepend(ocrpt_list *l, const void *data);
ocrpt_list *ocrpt_list_remove(ocrpt_list *l, const void *data);
void ocrpt_list_free(ocrpt_list *l);
void ocrpt_list_free_deep(ocrpt_list *l, ocrpt_mem_free_t freefunc);

/*
 * String related functions
 */
ocrpt_string *ocrpt_mem_string_new(const char *str, bool copy);
ocrpt_string *ocrpt_mem_string_new_with_len(const char *str, size_t len);
static inline size_t ocrpt_mem_vnprintf_size_from_string(const char *format, va_list va) {
	return vsnprintf(NULL, 0, format, va);
}
ocrpt_string *ocrpt_mem_string_new_vnprintf(size_t len, const char *format, va_list va);
ocrpt_string *ocrpt_mem_string_new_printf(const char *format, ...) __attribute__ ((format(printf, 1, 2)));
ocrpt_string *ocrpt_mem_string_resize(ocrpt_string *string, size_t len);
char *ocrpt_mem_string_free(ocrpt_string *string, bool free_str);
void ocrpt_mem_string_append_len(ocrpt_string *string, const char *str, const size_t len);
void ocrpt_mem_string_append_len_binary(ocrpt_string *string, const char *str, const size_t len);
void ocrpt_mem_string_append(ocrpt_string *string, const char *str);
void ocrpt_mem_string_append_c(ocrpt_string *string, const char c);
void ocrpt_mem_string_append_printf(ocrpt_string *string, const char *format, ...);

/********************************
 * Paper size related functions *
 ********************************/

/*
 * System default paper name and size
 */
const ocrpt_paper *ocrpt_get_system_paper(void);
/*
 * Get paper size of the specified paper name
 */
const ocrpt_paper *ocrpt_get_paper_by_name(const char *paper);
/*
 * Set paper for the report using an ocrpt_paper structure
 * The contents of the structure is copied.
 */
void ocrpt_set_paper(opencreport *o, const ocrpt_paper *paper);
/*
 * Set paper for the report using a paper name
 * If the paper name is unknown, the system default paper is set.
 */
void ocrpt_set_paper_by_name(opencreport *o, const char *paper);
/*
 * Get the report's currently set paper
 */
const ocrpt_paper *ocrpt_get_paper(opencreport *o);
/*
 * Iterator for supported paper names and sizes
 * The returned paper structures are library-global
 * but the iterator is per report.
 */
const ocrpt_paper *ocrpt_paper_first(opencreport *o);
const ocrpt_paper *ocrpt_paper_next(opencreport *o);

/******************************************
 * Datasource and query related functions *
 ******************************************/

/*
 * Add a custom datasource not covered by
 * the currently implemented sources.
 */
ocrpt_datasource *ocrpt_datasource_add(opencreport *o, const char *source_name, const ocrpt_input *input);
/*
 * Free a datasource from the opencreport structure it was added to
 */
void ocrpt_datasource_free(opencreport *o, ocrpt_datasource *source);
/*
 * Find the datasource using its name
 */
ocrpt_datasource *ocrpt_datasource_get(opencreport *o, const char *source_name);
/*
 * Validate the datasource pointer against the report structure
 */
ocrpt_datasource *ocrpt_datasource_validate(opencreport *o, ocrpt_datasource *source);
/*
 * Set the input encoding for a datasource
 */
void ocrpt_datasource_set_encoding(opencreport *o, ocrpt_datasource *source, const char *encoding);
/*
 * Add an array datasource
 *
 * Calling this is optional, as an array datasource called
 * "array" is automatically added to an opencreport structure.
 */
ocrpt_datasource *ocrpt_datasource_add_array(opencreport *o, const char *source_name);
/*
 * Set the global function pointer to resolve array and type array
 */
typedef void (*ocrpt_query_discover_func)(const char *, void **, const char *, void **);
void ocrpt_query_set_discover_func(ocrpt_query_discover_func func);
/*
 * Default discovery function for data and type arrays
 */
extern ocrpt_query_discover_func ocrpt_query_discover_array;
/*
 * Discovery function for data and type arrays
 */
void ocrpt_query_discover_array_c(const char *arrayname, void **array, const char *typesname, void **types);
/*
 * Add an array query using the datasource pointer
 *
 * The array's first row contains the header names
 * and the number of rows is the number of data rows,
 * i.e. it's one less than the actual number of rows
 * in array.
 */
ocrpt_query *ocrpt_query_add_array(opencreport *o, ocrpt_datasource *source,
									const char *name, const char **array,
									int32_t rows, int32_t cols,
									const enum ocrpt_result_type *types);
/*
 * Add a CSV datasource
 */
ocrpt_datasource *ocrpt_datasource_add_csv(opencreport *o, const char *source_name);
/*
 * Add a CSV query
 */
ocrpt_query *ocrpt_query_add_csv(opencreport *o, ocrpt_datasource *source,
									const char *name, const char *filename,
									const enum ocrpt_result_type *types);
/*
 * Add a JSON datasource
 */
ocrpt_datasource *ocrpt_datasource_add_json(opencreport *o, const char *source_name);
/*
 * Add a JSON query
 */
ocrpt_query *ocrpt_query_add_json(opencreport *o, ocrpt_datasource *source,
									const char *name, const char *filename,
									const enum ocrpt_result_type *types);
/*
 * Add an XML datasource
 */
ocrpt_datasource *ocrpt_datasource_add_xml(opencreport *o, const char *source_name);
/*
 * Add a XML query
 */
ocrpt_query *ocrpt_query_add_xml(opencreport *o, ocrpt_datasource *source,
									const char *name, const char *filename,
									const enum ocrpt_result_type *types);
/*
 * Add a PostgreSQL datasource using separate connection parameters
 */
ocrpt_datasource *ocrpt_datasource_add_postgresql(opencreport *o, const char *source_name,
												const char *host, const char *port, const char *dbname,
												const char *user, const char *password);
/*
 * Add a PostgreSQL datasource using a connection info string
 */
ocrpt_datasource *ocrpt_datasource_add_postgresql2(opencreport *o, const char *source_name, const char *conninfo);
/*
 * Add a PostgreSQL query
 */
ocrpt_query *ocrpt_query_add_postgresql(opencreport *o, ocrpt_datasource *source, const char *name, const char *querystr);
/*
 * Add a MariaDB/MySQL datasource using separate connection parameters
 */
ocrpt_datasource *ocrpt_datasource_add_mariadb(opencreport *o, const char *source_name,
												const char *host, const char *port, const char *dbname,
												const char *user, const char *password, const char *unix_socket);
/*
 * Add a MariaDB/MySQL datasource using an option file and group settings
 * If the option file is NULL, the default option files are used.
 */
ocrpt_datasource *ocrpt_datasource_add_mariadb2(opencreport *o, const char *source_name, const char *optionfile, const char *group);
/*
 * Add a MariaDB/MySQL query
 */
ocrpt_query *ocrpt_query_add_mariadb(opencreport *o, ocrpt_datasource *source, const char *name, const char *querystr);
/*
 * Add an ODBC datasource using separate connection parameters
 */
ocrpt_datasource *ocrpt_datasource_add_odbc(opencreport *o, const char *source_name,
											const char *dbname, const char *user, const char *password);
/*
 * Add an ODBC datasource using a connection info string
 */
ocrpt_datasource *ocrpt_datasource_add_odbc2(opencreport *o, const char *source_name, const char *conninfo);
/*
 * Add an ODBC query
 */
ocrpt_query *ocrpt_query_add_odbc(opencreport *o, ocrpt_datasource *source, const char *name, const char *querystr);
/*
 * Find a query using its name
 */
ocrpt_query *ocrpt_query_get(opencreport *o, const char *name);
/*
 * Return the query result array and the number of columns in it
 *
 * It must be re-run for every new data source row since
 * the pointer is invalidated after ocrpt_query_navigate_next()
 */
ocrpt_query_result *ocrpt_query_get_result(ocrpt_query *q, int32_t *cols);
/*
 * Get query result name, data and isnull
 */
const char *ocrpt_query_result_column_name(ocrpt_query_result *qr, int32_t col);
ocrpt_result *ocrpt_query_result_column_result(ocrpt_query_result *qr, int32_t col);

/*
 * Add follower query with a match function
 */
bool ocrpt_query_add_follower_n_to_1(opencreport *o,
									ocrpt_query *leader,
									ocrpt_query *follower,
									ocrpt_expr *match);
/*
 * Add follower query (runs side-by-side with "leader")
 */
bool ocrpt_query_add_follower(opencreport *o,
									ocrpt_query *leader,
									ocrpt_query *follower);
/*
 * Free a query and remove it from follower references
 */
void ocrpt_query_free(opencreport *o, ocrpt_query *q);
/*
 * Start query navigation from the beginning of the resultset
 */
void ocrpt_query_navigate_start(opencreport *o, ocrpt_query *q);
/*
 * Move to next row in the query resultset
 */
bool ocrpt_query_navigate_next(opencreport *o, ocrpt_query *q);

/********************************************************
 * Functions related to report and report part handling *
 ********************************************************/

/*
 * Create new part structure in the main opencreport structure
 */
ocrpt_part *ocrpt_part_new(opencreport *o);
/*
 * Create a new row in ocrpt_part
 */
ocrpt_part_row *ocrpt_part_new_row(opencreport *o, ocrpt_part *p);
/*
 * Create a new column element in a row
 */
ocrpt_part_row_data *ocrpt_part_row_new_data(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr);
/*
 * Append an ocrpt_report to the last column data of the last row in ocrpt_part
 *
 * "p" may be NULL, in which case a new part, a new row and
 * a new column are allocated and the passed-in "r" report
 * will be made part of it.
 *
 * The newly allocated (or valid passed-in) "p" ocrpt_part pointer
 * will be returned.
 */
ocrpt_part *ocrpt_part_append_report(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_row_data *pd, ocrpt_report *r);
/*
 * Free a report part and remove it from the parts list
 */
void ocrpt_part_free(opencreport *o, struct ocrpt_part *p);
/*
 * Free all report parts and remove them from the parts list
 */
void ocrpt_parts_free(opencreport *o);
/*
 * Layout (part/part row/part row column/report) related iterators
 */
ocrpt_part *ocrpt_part_get_next(opencreport *o, ocrpt_list **list);
ocrpt_part_row *ocrpt_part_row_get_next(ocrpt_part *p, ocrpt_list **list);
ocrpt_part_row_data *ocrpt_part_row_data_get_next(ocrpt_part_row *pr, ocrpt_list **list);
ocrpt_report *ocrpt_report_get_next(ocrpt_part_row_data *pd, ocrpt_list **list);

/*
 * Create a new ocrpt_report structure
 * It will have to be appended with ocrpt_part_append_report()
 * or to be used standalone for unit tests
 */
ocrpt_report *ocrpt_report_new(opencreport *o);
/*
 * Free an ocrpt_report structure
 * and optionally remove it from the parts list
 */
void ocrpt_report_free(opencreport *o, ocrpt_report *r);
/*
 * Validate a report against "o"
 */
bool ocrpt_report_validate(opencreport *o, ocrpt_report *r);
/*
 * Set the main query of an ocrpt_report
 */
void ocrpt_report_set_main_query(ocrpt_report *r, const char *query);
/*
 * Get the current row number of the main query
 */
long ocrpt_report_get_query_rownum(opencreport *o, ocrpt_report *r);
/*
 * Resolve report variables' base expressions
 */
void ocrpt_report_resolve_variables(opencreport *o, ocrpt_report *r);
/*
 * Evaluate report variables
 */
void ocrpt_report_evaluate_variables(opencreport *o, ocrpt_report *r);
/*
 * Resolve report breaks' break fields
 */
void ocrpt_report_resolve_breaks(opencreport *o, ocrpt_report *r);
/*
 * Resolve report expressions
 */
void ocrpt_report_resolve_expressions(opencreport *o, ocrpt_report *r);
/*
 * Evaluate report expressions
 */
void ocrpt_report_evaluate_expressions(opencreport *o, ocrpt_report *r);
/*
 * Add "report started" callback
 */
bool ocrpt_report_add_start_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add "report done" callback
 */
bool ocrpt_report_add_done_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add "new row" callback
 */
bool ocrpt_report_add_new_row_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add "report iteration" callback
 */
bool ocrpt_report_add_iteration_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add "precalculation done" callback
 */
bool ocrpt_report_add_precalculation_done_cb(opencreport *o, ocrpt_report *r, ocrpt_report_cb func, void *data);

/******************************
 * Callback related functions *
 ******************************/

/*
 * Add a "part added" callback
 */
void ocrpt_add_part_added_cb(opencreport *o, ocrpt_part_cb func, void *data);
/*
 * Add a "report added" callback
 */
void ocrpt_add_report_added_cb(opencreport *o, ocrpt_report_cb func, void *data);
/*
 * All "part iteration" callback
 */
bool ocrpt_add_part_iteration_cb(opencreport *o, ocrpt_part_cb func, void *data);
/*
 * All "all precalculations done" callback (one callback after all reports are precalculated)
 */
bool ocrpt_add_precalculation_done_cb(opencreport *o, ocrpt_cb func, void *data);

/**************************************
 * Functions related to file handling *
 *************************************/

/*
 * Returns a sanitized ("canonicalized") path
 *
 * The returned path contains only single directory separators
 * and doesn't contains symlinks.
 */
char *ocrpt_canonicalize_path(const char *path);

/*
 * Add search path (toplevel directories)
 *
 * Useful to find files relative to the search paths.
 */
void ocrpt_add_search_path(opencreport *o, const char *path);

/*
 * Find a file and return the canonicalized path to it
 *
 * This function takes the search paths into account.
 */
char *ocrpt_find_file(opencreport *o, const char *filename);

/**********************************************************
 * Functions related to finding or converting color names *
 **********************************************************/

/*
 * Find a color by its name
 *
 * The function fills in the ocrpt_color structure with RGB values
 * in Cairo values (0.0 ... 1.0)
 *
 * If the color name starts with "#" or "0x" or "0X" then it must be
 * in HTML notation.
 *
 * Otherwise, the color name is looked up in the color name database
 * case-insensitively. If found, the passed-in ocrpt_color structure is
 * filled with the RGB color value of that name.
 *
 * If not found or the passed-in color name is NULL, depending on the
 * the expected usage (foreground or background color), the ocrpt_color
 * structure is filled with either white or black.
 */
void ocrpt_get_color(opencreport *o, const char *cname, ocrpt_color *color, bool bgcolor) __attribute__((nonnull(1,3)));

#endif /* _OPENCREPORT_H_ */
