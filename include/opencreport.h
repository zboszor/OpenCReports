/*
 * OpenCReports main header
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORT_H_
#define _OPENCREPORT_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <mpfr.h>

/* Main report structure type */
struct opencreport;
typedef struct opencreport opencreport;

struct ocrpt_report;
typedef struct ocrpt_report ocrpt_report;

struct ocrpt_expr;
typedef struct ocrpt_expr ocrpt_expr;

struct ocrpt_function;
typedef struct ocrpt_function ocrpt_function;

enum ocrpt_result_type {
	/*
	 * Error in parsing or evaluation
	 */
	OCRPT_RESULT_ERROR,

	/*
	 * Constants
	 */
	OCRPT_RESULT_STRING,
	OCRPT_RESULT_NUMBER,
	OCRPT_RESULT_DATETIME
};

/*
 * The first part of the definitions are the same as enum ocrpt_result_type
 */
enum ocrpt_expr_type {
	/*
	 * Error in parsing or evaluation
	 */
	OCRPT_EXPR_ERROR,

	/*
	 * Constants
	 */
	OCRPT_EXPR_STRING,
	OCRPT_EXPR_NUMBER,
	OCRPT_EXPR_DATETIME,

	/*
	 * Pre-set variable that can be evaluated early
	 */
	OCRPT_EXPR_MVAR,

	/*
	 * Internal variables, identifiers resolved from queries and
	 * general expressions. These are evaluated on a row-by-row
	 * basis from the recordset.
	 */
	OCRPT_EXPR_RVAR,
	OCRPT_EXPR_IDENT,
	OCRPT_EXPR_VVAR,
	OCRPT_EXPR,
};

enum ocrpt_varref_type {
	OCRPT_VARREF_MVAR  = (1 << 0),
	OCRPT_VARREF_RVAR  = (1 << 1),
	OCRPT_VARREF_IDENT = (1 << 2),
	OCRPT_VARREF_VVAR  = (1 << 3)
};

struct ocrpt_list {
	struct ocrpt_list *next;
	const void *data;
	size_t len;
};
typedef struct ocrpt_list ocrpt_list;

struct ocrpt_string {
	char *str;
	size_t allocated_len;
	size_t len;
};
typedef struct ocrpt_string ocrpt_string;

struct ocrpt_result {
	/* Original lexer token or (computed) string value for expression */
	ocrpt_string *string;
	/* Converted numeric constant or computed numeric value for expression */
	mpfr_t number;
	/* Datetime value */
	struct tm datetime;
	/* Group indicators together as bitfields for space saving */
	enum ocrpt_result_type type:2;
	bool number_initialized:1;
	bool string_owned:1;
	bool date_valid:1;
	bool time_valid:1;
	bool interval:1;
	bool isnull:1;
};
typedef struct ocrpt_result ocrpt_result;

#define OCRPT_FUNCTION_PARAMS opencreport *o, ocrpt_report *r, ocrpt_expr *e
#define OCRPT_FUNCTION(name) void name(OCRPT_FUNCTION_PARAMS)
#define OCRPT_STATIC_FUNCTION(name) static void name(OCRPT_FUNCTION_PARAMS)
typedef void (*ocrpt_function_call)(OCRPT_FUNCTION_PARAMS);

struct ocrpt_function {
	const char *fname;
	ocrpt_function_call func;
	int32_t n_ops;
	bool commutative:1;
	bool associative:1;
	bool left_associative:1;
	bool dont_optimize:1;
};

enum ocrpt_var_type {
	OCRPT_VARIABLE_COUNT,
	OCRPT_VARIABLE_EXPRESSION,
	OCRPT_VARIABLE_SUM,
	OCRPT_VARIABLE_AVERAGE,
	OCRPT_VARIABLE_LOWEST,
	OCRPT_VARIABLE_HIGHEST
};
typedef enum ocrpt_var_type ocrpt_var_type;

enum ocrpt_var_type_bit {
	OCRPT_VARIABLE_COUNT_BIT = (1 << OCRPT_VARIABLE_COUNT),
	OCRPT_VARIABLE_EXPRESSION_BIT = (1 << OCRPT_VARIABLE_EXPRESSION),
	OCRPT_VARIABLE_SUM_BIT = (1 << OCRPT_VARIABLE_SUM),
	OCRPT_VARIABLE_AVERAGE_BIT = (1 << OCRPT_VARIABLE_AVERAGE),
	OCRPT_VARIABLE_LOWEST_BIT = (1 << OCRPT_VARIABLE_LOWEST),
	OCRPT_VARIABLE_HIGHEST_BIT = (1 << OCRPT_VARIABLE_HIGHEST)
};

struct ocrpt_break;
typedef struct ocrpt_break ocrpt_break;

struct ocrpt_var {
	const char *name;
	union {
		char *br_name;
		ocrpt_break *br;
	};
	ocrpt_expr *expr;
	ocrpt_result *rowcount[2];
	ocrpt_result *intermed[2];
	ocrpt_result *result[2];
	enum ocrpt_var_type type:3;
	bool reset_on_br:1;
	bool br_resolved:1;
};
typedef struct ocrpt_var ocrpt_var;

struct ocrpt_expr {
	struct ocrpt_result *result[2];
	union {
		/*
		 * Identifiers: computed report variables,
		 * environment variables, query fields
		 */
		struct {
			ocrpt_string *query;
			ocrpt_string *name;
			ocrpt_var *var;
		};

		struct {
			const ocrpt_function *func;
			ocrpt_expr **ops;
			uint32_t n_ops;
		};
	};
	enum ocrpt_expr_type type:4;
	bool result_owned0:1;
	bool result_owned1:1;
	bool parenthesized:1;
	bool dotprefixed:1;
};

struct ocrpt_datasource;
typedef struct ocrpt_datasource ocrpt_datasource;

struct ocrpt_query;
typedef struct ocrpt_query ocrpt_query;

struct ocrpt_query_result {
	const char *name;
	bool name_allocated;
	ocrpt_result result;
};
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

struct ocrpt_break {
	const char *name;
	bool attrs[OCRPT_BREAK_ATTRS_COUNT];
	ocrpt_list *breakfields;	/* list of ocrpt_expr pointers */
	ocrpt_list *reset_vars;		/* list of ocrpt_var pointers */
	/* TODO: add details for header and footer */
};

struct ocrpt_part;
typedef struct ocrpt_part ocrpt_part;

struct ocrpt_report {
	/*
	 * TODO:
	 * ocrpt_output *nodata;
	 * ocrpt_output *pageheader;
	 * ocrpt_output *pagefooter;
	 * ocrpt_output *reportheader;
	 * orcpt_output *reportfooter;
	 * ocrpt_output *detail;
	 * ...
	 */

	/* Parent part */
	ocrpt_part *part;

	/* Toplevel query */
	ocrpt_query *query;
	/* List of ocrpt_var elements */
	ocrpt_list *variables;
	/* List of ocrpt_break elements */
	ocrpt_list *breaks;
};

struct ocrpt_part {
	/*
	 * TODO:
	 * ocrpt_output *pageheader;
	 * ocrpt_output *pagefooter;
	 * ocrpt_output *reportheader;
	 * orcpt_output *reportfooter;
	 */

	/*
	 * List of ocrpt_list, each child list is the columns for the given row,
	 * which in turn are ocrpt_report elements.
	 */
	ocrpt_list *rows;
	ocrpt_list *lastrow;
	ocrpt_list *lastcol_in_lastrow;
	const char *path;
#if 0
	const char *xmlbuf;
	bool allocated:1;
	bool parsed:1;
#endif
};

#if 0
struct report_line;
typedef struct report_line report_line;
#endif

#define OCRPT_MPFR_PRECISION_BITS	(256)

/* Main report structure type, expanded */
struct opencreport {
	/* Paper name and size */
	const ocrpt_paper *paper;
	ocrpt_paper paper0;
	int32_t paper_iterator_idx;

	ocrpt_function **functions;
	int32_t n_functions;

	/* List and array of struct ocrpt_datasource */
	ocrpt_list *datasources;

	/* List and array of struct ocrpt_query */
	ocrpt_list *queries;

	/* Internal area for encoding conversion */
	ocrpt_string *converted;

	/* List of struct ocrpt_part elements */
	ocrpt_list *parts;
	ocrpt_list *last_part;

	/* Locale specific data */
	char *current_locale;
	char *currency_symbol;
	char *int_curr_symbol;
	char *decimal_point;
	char *thousands_sep;
	char *negative_sign;
	char *positive_sign;
	char *mon_grouping;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *amstr;
	char *pmstr;
	char *ampm_fmt;
	char *d_t_fmt;
	char *d_fmt;
	char *t_fmt;
	char int_n_cs_precedes;
	char int_n_sep_by_space;
	char int_n_sign_posn;
	char int_p_cs_precedes;
	char int_p_sep_by_space;
	char int_p_sign_posn;
	char n_cs_precedes;
	char n_sep_by_space;
	char n_sign_posn;
	char p_cs_precedes;
	char p_sep_by_space;
	char p_sign_posn;
	int int_frac_digits;
	int frac_digits;
	char *dayname[7];
	char *abdayname[7];
	char *monthname[12];
	char *abmonthname[12];

	/* Internal math defaults and states */
	mpfr_prec_t prec;
	mpfr_rnd_t rndmode;
	gmp_randstate_t randstate;

	/* Alternating datasource row result index  */
	bool residx:1;
};

/*
 * Create a new empty OpenCReports structure
 */
opencreport *ocrpt_init(void);
/*
 * Free an OpenCReports structure
 */
void ocrpt_free(opencreport *o);
/*
 * Set MPFR numeric precision
 */
void ocrpt_set_numeric_precision_bits(opencreport *o, mpfr_prec_t prec);
/*
 * Set MPFR rounding mode
 */
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
 * Lock and unlock the global locale mutex
 */
void ocrpt_lock_global_locale_mutex(void);
void ocrpt_unlock_global_locale_mutex(void);

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
ocrpt_expr *ocrpt_expr_parse(opencreport *o, const char *str, char **err);
/*
 * Initialize expression result to the specified type
 * Mainly used in functions
 */
bool ocrpt_expr_init_result(opencreport *o, ocrpt_expr *e, enum ocrpt_result_type type);
/*
 * Inline function to set the result owned/disowned by the expression
 */
static inline void ocrpt_expr_set_result_owned(opencreport *o, ocrpt_expr *e, bool owned) {
	if (o->residx)
		e->result_owned1 = owned;
	else
		e->result_owned0 = owned;
}
/*
 * Inline function to query the expression result ownership
 */
static inline bool ocrpt_expr_get_result_owned(opencreport *o, ocrpt_expr *e) {
	if (o->residx)
		return e->result_owned1;
	else
		return e->result_owned0;
}
/*
 * Set or initialize the result of the expression as an error with the specified message
 */
ocrpt_result *ocrpt_expr_make_error_result(opencreport *o, ocrpt_expr *e, const char *error);
/*
 * Optimize expression after parsing
 */
void ocrpt_expr_optimize(opencreport *o, ocrpt_report *r, ocrpt_expr *e);
/*
 * Resolve variable references in the expression
 */
void ocrpt_expr_resolve(opencreport *o, ocrpt_expr *e);
/*
 * Resolve variable references in the expression
 * with excluding certain variable reference types
 */
void ocrpt_expr_resolve_exclude(opencreport *o, ocrpt_expr *e, int32_t varref_exclude_mask);
/*
 * Checks whether the expression references certain variable types
 */
bool ocrpt_expr_references(opencreport *o, ocrpt_report *r, ocrpt_expr *e, int32_t varref_include_mask, uint32_t *varref_vartype_mask);
/*
 * Evaluate the expression, i.e. compute its ocrpt_result
 * It must be called after ocrpt_query_navigate_next(), see below.
 *
 * The returned ocrpt_result MUST NOT be freed with ocrpt_result_free().
 */
ocrpt_result *ocrpt_expr_eval(opencreport *o, ocrpt_report *r, ocrpt_expr *e);
/*
 * Compare two subsequent row data in the expression,
 * return true if they are identical.
 * It can be used to implement report breaks.
 */
bool ocrpt_expr_cmp_results(opencreport *o, ocrpt_expr *e);
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
 * Free an expression parse tree
 */
void ocrpt_expr_free(ocrpt_expr *e);
/*
 * Create a named report variable
 */
ocrpt_var *ocrpt_variable_new(opencreport *o, ocrpt_report *r, ocrpt_var_type type, const char *name, ocrpt_expr *e);
/*
 * Free a report variable
 */
void ocrpt_variable_free(opencreport *o, ocrpt_report *r, ocrpt_var *var);
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
ocrpt_function const * const ocrpt_function_find(opencreport *o, const char *fname);

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
 * Set the break attribute from bool value
 */
bool ocrpt_break_set_attribute(ocrpt_break *brk, const ocrpt_break_attr_type attr_type, bool value);
/*
 * Set the break attribute from numeric constant expression
 */
bool ocrpt_break_set_attribute_from_expr(opencreport *o, ocrpt_report *r, ocrpt_break *brk, const ocrpt_break_attr_type attr_type, ocrpt_expr *expr);
/*
 * Free a report break
 */
void ocrpt_break_free(opencreport *o, ocrpt_report *r, ocrpt_break *br, bool remove_from_list);
/*
 * Find a report break using its name
 */
ocrpt_break *ocrpt_break_get(opencreport *o, ocrpt_report *r, const char *name);
/*
 * Add a break field to a break
 */
bool ocrpt_break_add_breakfield(opencreport *o, ocrpt_report *r, ocrpt_break *b, ocrpt_expr *bf);
/*
 * Check whether the report break triggers
 */
bool ocrpt_break_check_fields(opencreport *o, ocrpt_break *br);

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
ocrpt_string *ocrpt_mem_string_resize(ocrpt_string *string, size_t len);
char *ocrpt_mem_string_free(ocrpt_string *string, bool free_str);
void ocrpt_mem_string_append_len(ocrpt_string *string, const char *str, const size_t len);
void ocrpt_mem_string_append(ocrpt_string *string, const char *str);
void ocrpt_mem_string_append_c(ocrpt_string *string, const char c);

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
ocrpt_datasource *ocrpt_datasource_find(opencreport *o, const char *source_name);
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
ocrpt_query *ocrpt_query_find(opencreport *o, const char *name);
/*
 * Return the query result array and the number of columns in it
 *
 * It must be re-run for every new data source row since
 * the pointer is invalidated after ocrpt_query_navigate_next()
 */
ocrpt_query_result *ocrpt_query_get_result(ocrpt_query *q, int32_t *cols);
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
ocrpt_list *ocrpt_part_new_row(opencreport *o, ocrpt_part *p);
/*
 * Append an ocrpt_report to the column list of the last row in ocrpt_part
 */
void ocrpt_part_append_report(opencreport *o, ocrpt_part *p, ocrpt_report *r);
/*
 * Free a report part and remove it from the parts list
 */
void ocrpt_part_free(opencreport *o, struct ocrpt_part *p);
/*
 * Free all report parts and remove them from the parts list
 */
void ocrpt_parts_free(opencreport *o);
/*
 * Create a new ocrpt_report structure
 * It will have to be appended with ocrpt_part_append_report()
 * or to be used standalone for unit tests
 */
ocrpt_report *ocrpt_report_new(void);
/*
 * Free an ocrpt_report structure
 * and optionally remove it from the parts list
 */
void ocrpt_report_free(opencreport *o, ocrpt_report *r, bool remove_from_list);
/*
 * Set the main query of an ocrpt_report
 */
void ocrpt_report_set_main_query(ocrpt_report *r, const char *query);

/********************************************
 * Functions related to report XML handling *
 ********************************************/

/*
 * Add details to the report from the parsed XML file
 */
bool ocrpt_parse_xml(opencreport *o, const char *filename);

/*
 * Execute the reports added up to this point.
 */
bool ocrpt_execute(opencreport *o);

#endif /* _OPENCREPORT_H_ */
