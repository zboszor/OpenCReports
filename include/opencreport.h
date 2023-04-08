/*
 * OpenCReports main header
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
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
#include <time.h>

struct opencreport;
typedef struct opencreport opencreport;

struct ocrpt_query;
typedef struct ocrpt_query ocrpt_query;

struct ocrpt_datasource;
typedef struct ocrpt_datasource ocrpt_datasource;

struct ocrpt_query_result;
typedef struct ocrpt_query_result ocrpt_query_result;

struct ocrpt_input {
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

struct ocrpt_line;
typedef struct ocrpt_line ocrpt_line;

struct ocrpt_text;
typedef struct ocrpt_text ocrpt_text;

struct ocrpt_hline;
typedef struct ocrpt_hline ocrpt_hline;

struct ocrpt_image;
typedef struct ocrpt_image ocrpt_image;

struct ocrpt_paper {
	const char *name;
	double width;
	double height;
};
typedef struct ocrpt_paper ocrpt_paper;

struct ocrpt_break;
typedef struct ocrpt_break ocrpt_break;

enum ocrpt_var_type {
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

struct ocrpt_part_column;
typedef struct ocrpt_part_column ocrpt_part_column;

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

#define OCRPT_FUNCTION_PARAMS ocrpt_expr *e, void *user_data
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
 * Get library version
 */
const char *ocrpt_version(void);
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
bool ocrpt_parse_xml_from_buffer(opencreport *o, const char *buffer, size_t size);
/*
 * Execute the reports added up to this point.
 */
bool ocrpt_execute(opencreport *o);
/*
 * Get output data and length
 */
char *ocrpt_get_output(opencreport *o, size_t *length);
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
void ocrpt_set_numeric_precision_bits(opencreport *o, const char *expr_string);
void ocrpt_set_rounding_mode(opencreport *o, const char *expr_string);

/****************************
 * Locale related functions *
 ****************************/

/*
 * Bind text domain for translating strings
 */
void ocrpt_bindtextdomain(opencreport *o, const char *domainname, const char *dirname);
/*
 * Set locale for the report
 * It does not affect the main program.
 *
 * ocrpt_set_locale() sets the locale immediately.
 * It's used as the default setting and a higher priority
 * setting than set by the delayed function
 * ocrpt_set_locale_from_expr().
 */
void ocrpt_set_locale(opencreport *o, const char *locale);
void ocrpt_set_locale_from_expr(opencreport *o, const char *expr_string);
/*
 * Get locale for the report
 */
locale_t ocrpt_get_locale(opencreport *o);
/*
 * Print monetary value in the report's locale
 * It supports printing mpfr_t (high precision) numeric values.
 */
ssize_t ocrpt_mpfr_strfmon(opencreport *o, char * __restrict s, size_t maxsize, const char * __restrict format, ...);

/******************************************
 * Datasource and query related functions *
 ******************************************/

/*
 * Add an array datasource
 *
 * Calling this is optional, as an array datasource called
 * "array" is automatically added to an opencreport structure.
 */
ocrpt_datasource *ocrpt_datasource_add_array(opencreport *o, const char *source_name);
/*
 * Query whether the datasource is array based
 */
bool ocrpt_datasource_is_array(ocrpt_datasource *source);
/*
 * Add an array query using the datasource pointer
 *
 * The array's first row contains the header names
 * and the number of rows is the number of data rows,
 * i.e. it's one less than the actual number of rows
 * in array.
 */
ocrpt_query *ocrpt_query_add_array(ocrpt_datasource *source,
									const char *name, const char **array,
									int32_t rows, int32_t cols,
									const int32_t *types,
									int32_t types_cols);
/*
 * Add a CSV datasource
 */
ocrpt_datasource *ocrpt_datasource_add_csv(opencreport *o, const char *source_name);
/*
 * Query whether the datasource is CSV based
 */
bool ocrpt_datasource_is_csv(ocrpt_datasource *source);
/*
 * Add a CSV query
 */
ocrpt_query *ocrpt_query_add_csv(ocrpt_datasource *source,
									const char *name, const char *filename,
									const int32_t *types,
									int32_t types_cols);
/*
 * Add a JSON datasource
 */
ocrpt_datasource *ocrpt_datasource_add_json(opencreport *o, const char *source_name);
/*
 * Query whether the datasource is JSON based
 */
bool ocrpt_datasource_is_json(ocrpt_datasource *source);
/*
 * Add a JSON query
 */
ocrpt_query *ocrpt_query_add_json(ocrpt_datasource *source,
									const char *name, const char *filename,
									const int32_t *types,
									int32_t types_cols);
/*
 * Add an XML datasource
 */
ocrpt_datasource *ocrpt_datasource_add_xml(opencreport *o, const char *source_name);
/*
 * Query whether the datasource is XML based
 */
bool ocrpt_datasource_is_xml(ocrpt_datasource *source);
/*
 * Add a XML query
 */
ocrpt_query *ocrpt_query_add_xml(ocrpt_datasource *source,
									const char *name, const char *filename,
									const int32_t *types,
									int32_t types_cols);
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
 * Query whether the datasource is PostgreSQL based
 */
bool ocrpt_datasource_is_postgresql(ocrpt_datasource *source);
/*
 * Add a PostgreSQL query
 */
ocrpt_query *ocrpt_query_add_postgresql(ocrpt_datasource *source, const char *name, const char *querystr);
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
 * Query whether the datasource is MariaDB based
 */
bool ocrpt_datasource_is_mariadb(ocrpt_datasource *source);
/*
 * Add a MariaDB/MySQL query
 */
ocrpt_query *ocrpt_query_add_mariadb(ocrpt_datasource *source, const char *name, const char *querystr);
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
 * Query whether the datasource is ODBC based
 */
bool ocrpt_datasource_is_odbc(ocrpt_datasource *source);
/*
 * Add an ODBC query
 */
ocrpt_query *ocrpt_query_add_odbc(ocrpt_datasource *source, const char *name, const char *querystr);
/*
 * Find the datasource using its name
 */
ocrpt_datasource *ocrpt_datasource_get(opencreport *o, const char *source_name);
/*
 * Add a custom datasource not covered by
 * the currently implemented sources.
 */
ocrpt_datasource *ocrpt_datasource_add(opencreport *o, const char *source_name, const ocrpt_input *input);
/*
 * Set the input encoding for a datasource
 */
void ocrpt_datasource_set_encoding(ocrpt_datasource *source, const char *encoding);
/*
 * Free a datasource from the opencreport structure it was added to
 */
void ocrpt_datasource_free(ocrpt_datasource *source);
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
 * Get query result column name
 */
const char *ocrpt_query_result_column_name(ocrpt_query_result *qr, int32_t col);
/*
 * Get the query result column data
 */
ocrpt_result *ocrpt_query_result_column_result(ocrpt_query_result *qr, int32_t col);
/*
 * Add follower query (runs side-by-side with "leader")
 */
bool ocrpt_query_add_follower(ocrpt_query *leader, ocrpt_query *follower);
/*
 * Add follower query with a match function
 */
bool ocrpt_query_add_follower_n_to_1(ocrpt_query *leader, ocrpt_query *follower, ocrpt_expr *match);
/*
 * Free a query and remove it from follower references
 */
void ocrpt_query_free(ocrpt_query *q);
/*
 * Start query navigation from the beginning of the resultset
 */
void ocrpt_query_navigate_start(ocrpt_query *q);
/*
 * Move to next row in the query resultset
 */
bool ocrpt_query_navigate_next(ocrpt_query *q);
/*
 * Set the global function pointer to resolve array and type array
 */
typedef void (*ocrpt_query_discover_func)(const char *, void **, int32_t *, int32_t *, const char *, void **, int32_t *);
void ocrpt_query_set_discover_func(ocrpt_query_discover_func func);
/*
 * Default discovery function for data and type arrays
 */
extern ocrpt_query_discover_func ocrpt_query_discover_array;
/*
 * Discovery function for data and type arrays
 */
void ocrpt_query_discover_array_c(const char *arrayname, void **array, int32_t *rows, int32_t *cols, const char *typesname, void **types, int32_t *types_cols);
/*
 * Set the print debugger function
 */
typedef int (*ocrpt_printf_func)(const char *fmt, ...);

void ocrpt_set_printf_func(ocrpt_printf_func func);
void ocrpt_set_err_printf_func(ocrpt_printf_func func);

/********************************
 * Expression related functions *
 ********************************/

/*
 * Create an expression parse tree from an expression string
 */
ocrpt_expr *ocrpt_expr_parse(opencreport *o, const char *expr_string, char **err);
/*
 * Create an expression parse tree from an expression string
 * for a report
 */
ocrpt_expr *ocrpt_report_expr_parse(ocrpt_report *r, const char *expr_string, char **err);
/*
 * Free an expression parse tree
 */
void ocrpt_expr_free(ocrpt_expr *e);
/*
 * Resolve variable references in the expression
 */
void ocrpt_expr_resolve(ocrpt_expr *e);
/*
 * Optimize expression after parsing
 */
void ocrpt_expr_optimize(ocrpt_expr *e);
/*
 * Evaluate the expression, i.e. compute its ocrpt_result
 * It must be called after ocrpt_query_navigate_next(), see below.
 *
 * The returned ocrpt_result MUST NOT be freed with ocrpt_result_free().
 */
ocrpt_result *ocrpt_expr_eval(ocrpt_expr *e);
/*
 * Get the current ocrpt_result without (re-)evaluation
 *
 * The returned ocrpt_result MUST NOT be freed with ocrpt_result_free().
 */
ocrpt_result *ocrpt_expr_get_result(ocrpt_expr *e);
/*
 * Print an expression on stdout. Good for unit testing.
 */
void ocrpt_expr_print(ocrpt_expr *e);
/*
 * Print the result data for every subexpression in the expression. Good for unit testing.
 */
void ocrpt_expr_result_deep_print(ocrpt_expr *e);
/*
 * Count the number of expression nodes
 */
int32_t ocrpt_expr_nodes(ocrpt_expr *e);
/*
 * Initialize the "current" expression result to the specified type
 * Mainly used in functions
 */
bool ocrpt_expr_init_result(ocrpt_expr *e, enum ocrpt_result_type type);
/*
 * Initialize expression results to the specified type
 * Mainly used in functions
 */
void ocrpt_expr_init_results(ocrpt_expr *e, enum ocrpt_result_type type);
/*
 * Set or initialize the result of the expression as an error with the specified message
 */
ocrpt_result *ocrpt_expr_make_error_result(ocrpt_expr *e, const char *format, ...);
/*
 * Set whether the start value for iterative expressions
 * is the initial value or computed
 */
void ocrpt_expr_set_iterative_start_value(ocrpt_expr *e, bool start_with_init);
/*
 * Get the current value for ocrpt_expr in base type
 */
const char *ocrpt_expr_get_string_value(ocrpt_expr *e);
long ocrpt_expr_get_long_value(ocrpt_expr *e);
double ocrpt_expr_get_double_value(ocrpt_expr *e);
void ocrpt_expr_set_string_value(ocrpt_expr *e, const char *s);
void ocrpt_expr_set_long_value(ocrpt_expr *e, long l);
void ocrpt_expr_set_double_value(ocrpt_expr *e, double d);
/*
 * Set the nth value for ocrpt_expr in base type
 */
void ocrpt_expr_set_nth_result_string_value(ocrpt_expr *e, int which, const char *s);
void ocrpt_expr_set_nth_result_long_value(ocrpt_expr *e, int which, long l);
void ocrpt_expr_set_nth_result_double_value(ocrpt_expr *e, int which, double d);
/*
 * Compare two subsequent row data in the expression,
 * return true if they are identical.
 * It can be used to implement report breaks.
 */
bool ocrpt_expr_cmp_results(ocrpt_expr *e);
/*
 * Set delayed property of the expression
 */
void ocrpt_expr_set_delayed(ocrpt_expr *e, bool delayed);
/*
 * Set the r.value reference
 */
void ocrpt_expr_set_field_expr(ocrpt_expr *e, ocrpt_expr *rvalue);

/**********************************************
 * Column data or expression result functions *
 **********************************************/

/*
 * Create an ocrpt_result structure
 * Must be freed with ocrpt_result_free().
 */
ocrpt_result *ocrpt_result_new(opencreport *o);
/*
 * Get result type
 */
enum ocrpt_result_type ocrpt_result_get_type(ocrpt_result *result);
/*
 * Copy result value
 */
void ocrpt_result_copy(ocrpt_result *dst, ocrpt_result *src);
/*
 * Print the result data. Good for unit testing.
 */
void ocrpt_result_print(ocrpt_result *r);
/*
 * Free an ocrpt_result structure
 */
void ocrpt_result_free(ocrpt_result *r);
/*
 * Get/set the column's NULL-ness, discover its type and
 * get its underlying value depending on its type
 * from ocrpt_result
 */
bool ocrpt_result_isnull(ocrpt_result *result);
void ocrpt_result_set_isnull(ocrpt_result *result, bool isnull);

bool ocrpt_result_isnumber(ocrpt_result *result);
mpfr_ptr ocrpt_result_get_number(ocrpt_result *result);
void ocrpt_result_set_long(ocrpt_result *result, long value);
void ocrpt_result_set_double(ocrpt_result *result, double value);

bool ocrpt_result_isstring(ocrpt_result *result);
ocrpt_string *ocrpt_result_get_string(ocrpt_result *result);
void ocrpt_result_set_string(ocrpt_result *result, const char *value);

bool ocrpt_result_isdatetime(ocrpt_result *result);
const struct tm *ocrpt_result_get_datetime(ocrpt_result *result);
bool ocrpt_result_datetime_is_interval(ocrpt_result *result);
bool ocrpt_result_datetime_is_date_valid(ocrpt_result *result);
bool ocrpt_result_datetime_is_time_valid(ocrpt_result *result);

/******************************
 * Variable related functions *
 ******************************/

/*
 * Create a named report variable
 */
ocrpt_var *ocrpt_variable_new(ocrpt_report *r,
			ocrpt_var_type type, const char *name,
			const char *expr,
			const char *reset_on_break_name);
/*
 * Create a names custom variable
 */
ocrpt_var *ocrpt_variable_new_full(ocrpt_report *r,
			enum ocrpt_result_type type, const char *name,
			const char *baseexpr, const char *intermedexpr,
			const char *intermed2expr, const char *resultexpr,
			const char *reset_on_break_name);
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
void ocrpt_variable_resolve(ocrpt_var *v);
/*
 * Evaluate a variable
 */
void ocrpt_variable_evaluate(ocrpt_var *v);

/***************************
 * Break related functions *
 ***************************/

/*
 * Add a named report break
 */
ocrpt_break *ocrpt_break_new(ocrpt_report *r, const char *name);
/*
 * Set the headernewpage break attribute from expression string
 */
void ocrpt_break_set_headernewpage(ocrpt_break *br, const char *headernewpage);
/*
 * Set the suppressblank break attribute from expression string
 */
void ocrpt_break_set_suppressblank(ocrpt_break *br, const char *suppressblank);
/*
 * Find a report break using its name
 */
ocrpt_break *ocrpt_break_get(ocrpt_report *r, const char *name);
/*
 * Get break name
 */
const char *ocrpt_break_get_name(ocrpt_break *br);
/*
 * Add a break field to a break
 * This function takes over ownership of the breakfield expression
 */
bool ocrpt_break_add_breakfield(ocrpt_break *br, ocrpt_expr *bf);
/*
 * Iterate through the report's breaks
 */
ocrpt_break *ocrpt_break_get_next(ocrpt_report *r, ocrpt_list **list);
/*
 * Resolve and optimize break fields
 */
void ocrpt_break_resolve_fields(ocrpt_break *br);
/*
 * Check whether the report break triggers
 */
bool ocrpt_break_check_fields(ocrpt_break *br);
/*
 * Check whether any of the report break fields are blank
 * (empty string or NULL)
 */
bool ocrpt_break_check_blank(ocrpt_break *br, bool evaluate);
/*
 * Reset variables for the break
 */
void ocrpt_break_reset_vars(ocrpt_break *br);

/******************************
 * Function related functions *
 ******************************/

/*
 * Add a new custom function to the opencreport structure
 */
bool ocrpt_function_add(opencreport *o, const char *fname,
						ocrpt_function_call func, void *user_data,
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
ocrpt_result *ocrpt_expr_operand_get_result(ocrpt_expr *e, int32_t opnum);

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
ocrpt_part_row *ocrpt_part_new_row(ocrpt_part *p);
/*
 * Create a new column element in a row
 */
ocrpt_part_column *ocrpt_part_row_new_column(ocrpt_part_row *pr);
/*
 * Create a new ocrpt_report structure in the column
 */
ocrpt_report *ocrpt_part_column_new_report(ocrpt_part_column *pd);
/*
 * Layout (part/part row/part row column/report) related iterators
 */
ocrpt_part *ocrpt_part_get_next(opencreport *o, ocrpt_list **list);
ocrpt_part_row *ocrpt_part_row_get_next(ocrpt_part *p, ocrpt_list **list);
ocrpt_part_column *ocrpt_part_column_get_next(ocrpt_part_row *pr, ocrpt_list **list);
ocrpt_report *ocrpt_report_get_next(ocrpt_part_column *pd, ocrpt_list **list);

/*
 * Set the main query of an ocrpt_report
 */
void ocrpt_report_set_main_query(ocrpt_report *r, const ocrpt_query *query);

void ocrpt_report_set_main_query_by_name(ocrpt_report *r, const char *query);
/*
 * Get the current row number of the main query
 */
long ocrpt_report_get_query_rownum(ocrpt_report *r);
/*
 * Resolve report variables' base expressions
 */
void ocrpt_report_resolve_variables(ocrpt_report *r);
/*
 * Evaluate report variables
 */
void ocrpt_report_evaluate_variables(ocrpt_report *r);
/*
 * Resolve report breaks' break fields
 */
void ocrpt_report_resolve_breaks(ocrpt_report *r);
/*
 * Resolve report expressions
 */
void ocrpt_report_resolve_expressions(ocrpt_report *r);
/*
 * Evaluate report expressions
 */
void ocrpt_report_evaluate_expressions(ocrpt_report *r);

/******************************
 * Callback related functions *
 ******************************/

/*
 * Add a "part added" callback
 */
bool ocrpt_add_part_added_cb(opencreport *o, ocrpt_part_cb func, void *data);
/*
 * Add a "report added" callback
 */
bool ocrpt_add_report_added_cb(opencreport *o, ocrpt_report_cb func, void *data);
/*
 * Add an "all precalculations done" callback (one callback after all reports are precalculated)
 */
bool ocrpt_add_precalculation_done_cb(opencreport *o, ocrpt_cb func, void *data);
/*
 * Add a "part iteration" callback
 */
bool ocrpt_part_add_iteration_cb(ocrpt_part *p, ocrpt_part_cb func, void *data);
/*
 * Add a "report started" callback
 */
bool ocrpt_report_add_start_cb(ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add "report done" callback
 */
bool ocrpt_report_add_done_cb(ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add a "new row" callback
 */
bool ocrpt_report_add_new_row_cb(ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add a "report iteration" callback
 */
bool ocrpt_report_add_iteration_cb(ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add a "report precalculation done" callback
 */
bool ocrpt_report_add_precalculation_done_cb(ocrpt_report *r, ocrpt_report_cb func, void *data);
/*
 * Add break trigger callback
 */
bool ocrpt_break_add_trigger_cb(ocrpt_break *br, ocrpt_break_trigger_cb func, void *data);

/***********************************
 * Output/layout related functions *
 ***********************************/

void ocrpt_set_size_unit(opencreport *o, const char *expr_string);
void ocrpt_set_noquery_show_nodata(opencreport *o, const char *expr_string);
void ocrpt_set_report_height_after_last(opencreport *o, const char *expr_string);

void ocrpt_part_set_iterations(ocrpt_part *p, int32_t iterations);
void ocrpt_part_set_font_name(ocrpt_part *p, const char *font_name);
void ocrpt_part_set_font_size(ocrpt_part *p, const char *font_size);
void ocrpt_part_set_paper_by_name(ocrpt_part *p, const char *paper_name);
void ocrpt_part_set_landscape(ocrpt_part *p, bool landscape);
void ocrpt_part_set_top_margin(ocrpt_part *p, const char * margin);
void ocrpt_part_set_bottom_margin(ocrpt_part *p, const char *margin);
void ocrpt_part_set_left_margin(ocrpt_part *p, const char *margin);
void ocrpt_part_set_right_margin(ocrpt_part *p, const char *margin);
void ocrpt_part_set_suppress(ocrpt_part *p, bool suppress);
void ocrpt_part_set_suppress_pageheader_firstpage(ocrpt_part *p, bool suppress);

void ocrpt_part_row_set_suppress(ocrpt_part_row *pr, bool suppress);
void ocrpt_part_row_set_newpage(ocrpt_part_row *pr, bool newpage);
void ocrpt_part_row_set_layout_fixed(ocrpt_part_row *pr, bool fixed);

void ocrpt_part_column_set_suppress(ocrpt_part_column *pd, bool suppress);
void ocrpt_part_column_set_width(ocrpt_part_column *pd, double width);
void ocrpt_part_column_set_height(ocrpt_part_column *pd, double height);
void ocrpt_part_column_set_border_width(ocrpt_part_column *pd, double border_width);
void ocrpt_part_column_set_border_color(ocrpt_part_column *pd, const char *color);
void ocrpt_part_column_set_detail_columns(ocrpt_part_column *pd, int32_t detail_columns);
void ocrpt_part_column_set_column_padding(ocrpt_part_column *pd, double padding);

void ocrpt_report_set_suppress(ocrpt_report *r, bool suppress);
void ocrpt_report_set_iterations(ocrpt_report *r, int32_t iterations);
void ocrpt_report_set_font_name(ocrpt_report *r, const char *font_name);
void ocrpt_report_set_font_size(ocrpt_report *r, double font_size);
void ocrpt_report_set_height(ocrpt_report *r, double height);
void ocrpt_report_set_fieldheader_high_priority(ocrpt_report *r, bool high_priority);

void ocrpt_layout_part_page_header_set_report(ocrpt_part *p, ocrpt_report *r);
ocrpt_output *ocrpt_layout_part_page_header(ocrpt_part *p);

void ocrpt_layout_part_page_footer_set_report(ocrpt_part *p, ocrpt_report *r);
ocrpt_output *ocrpt_layout_part_page_footer(ocrpt_part *p);

ocrpt_output *ocrpt_layout_report_nodata(ocrpt_report *r);
ocrpt_output *ocrpt_layout_report_header(ocrpt_report *r);
ocrpt_output *ocrpt_layout_report_footer(ocrpt_report *r);
ocrpt_output *ocrpt_layout_report_field_header(ocrpt_report *r);
ocrpt_output *ocrpt_layout_report_field_details(ocrpt_report *r);

ocrpt_output *ocrpt_break_get_header(ocrpt_break *br);
ocrpt_output *ocrpt_break_get_footer(ocrpt_break *br);

void ocrpt_output_set_suppress(ocrpt_output *output, const char *expr_string);

ocrpt_line *ocrpt_output_add_line(ocrpt_output *output);

void ocrpt_line_set_font_name(ocrpt_line *line, const char *expr_string);
void ocrpt_line_set_font_size(ocrpt_line *line, const char *expr_string);
void ocrpt_line_set_bold(ocrpt_line *line, const char *expr_string);
void ocrpt_line_set_italic(ocrpt_line *line, const char *expr_string);
void ocrpt_line_set_suppress(ocrpt_line *line, const char *expr_string);
void ocrpt_line_set_color(ocrpt_line *line, const char *expr_string);
void ocrpt_line_set_bgcolor(ocrpt_line *line, const char *expr_string);

ocrpt_text *ocrpt_line_add_text(ocrpt_line *line);

void ocrpt_text_set_value_string(ocrpt_text *text, const char *string);
void ocrpt_text_set_value_expr(ocrpt_text *text, const char *expr_string, bool delayed);
void ocrpt_text_set_format(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_translate(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_width(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_alignment(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_color(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_bgcolor(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_font_name(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_font_size(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_bold(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_italic(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_link(ocrpt_text *text, const char *expr_string);
void ocrpt_text_set_memo(ocrpt_text *text, bool memo, bool wrap_chars, int32_t max_lines);

ocrpt_hline *ocrpt_output_add_hline(ocrpt_output *output);

void ocrpt_hline_set_size(ocrpt_hline *hline, const char *expr_string);
void ocrpt_hline_set_indent(ocrpt_hline *hline, const char *expr_string);
void ocrpt_hline_set_length(ocrpt_hline *hline, const char *expr_string);
void ocrpt_hline_set_font_size(ocrpt_hline *hline, const char *expr_string);
void ocrpt_hline_set_suppress(ocrpt_hline *hline, const char *expr_string);
void ocrpt_hline_set_color(ocrpt_hline *hline, const char *expr_string);

ocrpt_image *ocrpt_output_add_image(ocrpt_output *output);
ocrpt_image *ocrpt_line_add_image(ocrpt_line *line);

void ocrpt_image_set_value(ocrpt_image *image, const char *expr_string);
void ocrpt_image_set_suppress(ocrpt_image *image, const char *expr_string);
void ocrpt_image_set_type(ocrpt_image *image, const char *expr_string);
void ocrpt_image_set_width(ocrpt_image *image, const char *expr_string);
void ocrpt_image_set_height(ocrpt_image *image, const char *expr_string);
void ocrpt_image_set_alignment(ocrpt_image *image, const char *expr_string);
void ocrpt_image_set_bgcolor(ocrpt_image *image, const char *expr_string);
void ocrpt_image_set_text_width(ocrpt_image *image, const char *expr_string);

void ocrpt_output_add_image_end(ocrpt_output *output);

/*********************************
 * Environment related functions *
 *********************************/

typedef ocrpt_result *(*ocrpt_env_query_func)(opencreport *, const char *);
extern ocrpt_env_query_func ocrpt_env_get;

/*
 * Set the global environment getter function
 */
void ocrpt_env_set_query_func(ocrpt_env_query_func func);

/*
 * Returns the value of an environment variable
 * The returned ocrpt_result pointer must be freed with ocrpt_result_free()
 */
ocrpt_result *ocrpt_env_get_c(opencreport *o, const char *env);

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
void ocrpt_get_color(const char *cname, ocrpt_color *color, bool bgcolor);

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
 * The returned paper structures are library-global.
 * The iterator is caller specified.
 */
const ocrpt_paper *ocrpt_paper_next(opencreport *o, void **iter);

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

static inline void *ocrpt_mem_reallocarray(void *ptr, size_t nmemb, size_t sz) __attribute__((alloc_size(2)));
static inline void *ocrpt_mem_reallocarray(void *ptr, size_t nmemb, size_t sz) { return ocrpt_mem_reallocarray0(ptr, nmemb, sz); }

static inline void ocrpt_mem_free(const void *ptr) { ocrpt_mem_free0(ptr); }

static inline void *ocrpt_mem_strdup(const char *ptr) { return (ptr ? ocrpt_mem_strdup0(ptr) : NULL); }
static inline void *ocrpt_mem_strndup(const char *ptr, size_t sz) { return (ptr ? ocrpt_mem_strndup0(ptr, sz) : NULL); }
static inline void ocrpt_strfree(const char *s) { ocrpt_mem_free0((const void *)s); }

void ocrpt_mem_set_alloc_funcs(ocrpt_mem_malloc_t rmalloc,
								ocrpt_mem_realloc_t rrealloc,
								ocrpt_mem_reallocarray_t rreallocarray,
								ocrpt_mem_free_t rfree,
								ocrpt_mem_strdup_t rstrdup,
								ocrpt_mem_strndup_t rstrndup);

/*
 * List related functions
 */
size_t ocrpt_list_length(ocrpt_list *l);
ocrpt_list *ocrpt_makelist1(const void *data);
ocrpt_list *ocrpt_makelist(const void *data1, ...);
ocrpt_list *ocrpt_list_last(const ocrpt_list *l);
ocrpt_list *ocrpt_list_nth(const ocrpt_list *l, uint32_t n);
ocrpt_list *ocrpt_list_end_append(ocrpt_list *l, ocrpt_list **e, const void *data);
ocrpt_list *ocrpt_list_append(ocrpt_list *l, const void *data);
ocrpt_list *ocrpt_list_prepend(ocrpt_list *l, const void *data);
ocrpt_list *ocrpt_list_end_remove(ocrpt_list *l, ocrpt_list **endptr, const void *data);
ocrpt_list *ocrpt_list_remove(ocrpt_list *l, const void *data);
ocrpt_list *ocrpt_list_next(ocrpt_list *l);
void *ocrpt_list_get_data(ocrpt_list *l);
void ocrpt_list_free(ocrpt_list *l);
void ocrpt_list_free_deep(ocrpt_list *l, ocrpt_mem_free_t freefunc);

/*
 * String related functions
 */
ocrpt_string *ocrpt_mem_string_new(const char *str, bool copy);
ocrpt_string *ocrpt_mem_string_new_with_len(const char *str, size_t len);
ocrpt_string *ocrpt_mem_string_new_vnprintf(size_t len, const char *format, va_list va);
ocrpt_string *ocrpt_mem_string_new_printf(const char *format, ...) __attribute__ ((format(printf, 1, 2)));
ocrpt_string *ocrpt_mem_string_resize(ocrpt_string *string, size_t len);
char *ocrpt_mem_string_free(ocrpt_string *string, bool free_str);
void ocrpt_mem_string_append_len(ocrpt_string *string, const char *str, const size_t len);
void ocrpt_mem_string_append_len_binary(ocrpt_string *string, const char *str, const size_t len);
void ocrpt_mem_string_append(ocrpt_string *string, const char *str);
void ocrpt_mem_string_append_c(ocrpt_string *string, const char c);
void ocrpt_mem_string_append_printf(ocrpt_string *string, const char *format, ...);

#endif /* _OPENCREPORT_H_ */
