/*
 * OpenCReports main header
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
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

struct ocrpt_expr;
typedef struct ocrpt_expr ocrpt_expr;

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

struct ocrpt_string {
	char *str;
	size_t allocated_len;
	size_t len;
};
typedef struct ocrpt_string ocrpt_string;

struct ocrpt_result {
	enum ocrpt_result_type type;
	/* Original lexer token or (computed) string value for expression */
	ocrpt_string *string;
	/* Converted numeric constant or computed numeric value for expression */
	mpfr_t number;
	bool number_initialized;
	bool string_owned;
	bool isnull;
	/* Datetime value */
	struct tm datetime;
};
typedef struct ocrpt_result ocrpt_result;

struct ocrpt_query;
typedef struct ocrpt_query ocrpt_query;

struct ocrpt_query_result {
	const char *name;
	bool name_allocated;
	ocrpt_result result;
};
typedef struct ocrpt_query_result ocrpt_query_result;

struct ocrpt_input {
	void (*describe)(ocrpt_query *, ocrpt_query_result **, int32_t *);
	void (*rewind)(ocrpt_query *);
	bool (*next)(ocrpt_query *);
	bool (*populate_result)(ocrpt_query *);
	bool (*isdone)(ocrpt_query *);
	void (*free)(ocrpt_query *);
};

typedef struct ocrpt_input ocrpt_input;

struct ocrpt_datasource;
typedef struct ocrpt_datasource ocrpt_datasource;

struct ocrpt_paper {
	const char *name;
	double width;
	double height;
};
typedef struct ocrpt_paper ocrpt_paper;

#if 0
struct report_line;
typedef struct report_line report_line;

struct ocrpt_report_part;
typedef struct ocrpt_report_part ocrpt_report_part;
#endif

#define OCRPT_MPFR_PRECISION_BITS	(256)

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

/********************************
 * Expression related functions *
 ********************************/

/*
 * Create an expression parse tree from an expression string
 */
ocrpt_expr *ocrpt_expr_parse(opencreport *o, const char *str, char **err);
/*
 * Optimize expression after parsing
 */
void ocrpt_expr_optimize(opencreport *o, ocrpt_expr *e);
/*
 * Resolve variable references in the expression
 */
void ocrpt_expr_resolve(opencreport *o, ocrpt_expr *e);
/*
 * Evaluate the expression, i.e. compute its ocrpt_result
 * It must be called after ocrpt_navigate_next(), see below.
 */
ocrpt_result *ocrpt_expr_eval(opencreport *o, ocrpt_expr *e);
/*
 * Print an expression on stdout. Good for unit testing.
 */
void ocrpt_expr_print(opencreport *o, ocrpt_expr *e);
/*
 * Print the result data. Good for unit testing.
 */
void ocrpt_expr_result_print(ocrpt_result *r);
/*
 * Count the number of expression nodes
 */
int ocrpt_expr_nodes(ocrpt_expr *e);
/*
 * Free an expression parse tree
 */
void ocrpt_free_expr(ocrpt_expr *e);

/****************************
 * Memory handling wrappers *
 ****************************/

typedef void *(*ocrpt_mem_malloc_t)(size_t);
typedef void *(*ocrpt_mem_realloc_t)(void *, size_t);
typedef void (*ocrpt_mem_free_t)(const void *);
typedef char *(*ocrpt_mem_strdup_t)(const char *);
typedef char *(*ocrpt_mem_strndup_t)(const char *, size_t);

extern ocrpt_mem_malloc_t ocrpt_mem_malloc0;
extern ocrpt_mem_realloc_t ocrpt_mem_realloc0;
extern ocrpt_mem_free_t ocrpt_mem_free0;
extern ocrpt_mem_strdup_t ocrpt_mem_strdup0;
extern ocrpt_mem_strndup_t ocrpt_mem_strndup0;

static inline void *ocrpt_mem_malloc(size_t sz) __attribute__((malloc,alloc_size(1)));
static inline void *ocrpt_mem_malloc(size_t sz) { return ocrpt_mem_malloc0(sz); }

static inline void *ocrpt_mem_realloc(void *ptr, size_t sz) __attribute__((alloc_size(2)));
static inline void *ocrpt_mem_realloc(void *ptr, size_t sz) { return ocrpt_mem_realloc0(ptr, sz); }

static inline void ocrpt_mem_free(const void *ptr) { ocrpt_mem_free0(ptr); }
static inline void ocrpt_strfree(const char *s) { ocrpt_mem_free0((const void *)s); }

static inline void *ocrpt_mem_strdup(const char *ptr) { return (ptr ? ocrpt_mem_strdup0(ptr) : NULL); }

static inline void *ocrpt_mem_strndup(const char *ptr, size_t sz) { return (ptr ? ocrpt_mem_strndup0(ptr, sz) : NULL); }

void ocrpt_mem_set_alloc_funcs(ocrpt_mem_malloc_t rmalloc,
								ocrpt_mem_realloc_t rrealloc,
								ocrpt_mem_free_t rfree,
								ocrpt_mem_strdup_t rstrdup,
								ocrpt_mem_strndup_t rstrndup);

/*
 * String related functions
 */
ocrpt_string *ocrpt_mem_string_new(const char *str, bool copy);
ocrpt_string *ocrpt_mem_string_new_with_len(const char *str, size_t len);
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
ocrpt_datasource *ocrpt_add_datasource(opencreport *o, const char *source_name, const ocrpt_input *input);
/*
 * Find the datasource using its name
 */
ocrpt_datasource *ocrpt_find_datasource(opencreport *o, const char *source_name);
/*
 * Validate the datasource pointer against the report structure
 */
ocrpt_datasource *ocrpt_validate_datasource(opencreport *o, ocrpt_datasource *source);
/*
 * Add an array datasource
 *
 * Calling this is optional, as an array datasource called
 * "array" is automatically added to an opencreport structure.
 */
ocrpt_datasource *ocrpt_add_array_datasource(opencreport *o, const char *source_name);
/*
 * Add an array query using the datasource pointer
 */
ocrpt_query *ocrpt_add_array_query(opencreport *o, ocrpt_datasource *source,
									const char *name, void *array,
									int32_t rows, int32_t cols,
									const enum ocrpt_result_type *types);
/*
 * Add an array query using the datasource name
 */
ocrpt_query *ocrpt_add_array_query_as(opencreport *o, const char *source_name,
									const char *name, void *array,
									int32_t rows, int32_t cols,
									const enum ocrpt_result_type *types);
/*
 * Return the query result array and the number of columns in it
 *
 * It must be re-run for every new data source row since
 * the pointer is invalidated after ocrpt_navigate_next()
 */
ocrpt_query_result *ocrpt_query_get_result(ocrpt_query *q, int32_t *cols);
/*
 * Add follower query with a match function
 */
bool ocrpt_add_query_follower_n_to_1(opencreport *o,
									ocrpt_query *leader,
									ocrpt_query *follower,
									ocrpt_expr *match);
/*
 * Add follower query (runs side-by-side with "leader")
 */
bool ocrpt_add_query_follower(opencreport *o,
									ocrpt_query *leader,
									ocrpt_query *follower);
/*
 * Free a query and remove it from follower references
 */
void ocrpt_free_query(opencreport *o, ocrpt_query *q);
/*
 * Start query navigation from the beginning of the resultset
 */
void ocrpt_navigate_start(opencreport *o, ocrpt_query *q);
/*
 * Move to next row in the query resultset
 */
bool ocrpt_navigate_next(opencreport *o, ocrpt_query *q);

#if 0
/*
 * Create a report part
 */
ocrpt_report_part *ocrpt_create_report_part(void);
/*
 * Add report header to report part
 */
ocrpt_report_part_set_report_header(ocrpt_report_part *part, ...);
#endif
/*
 * Add XML report description from a file
 */
int ocrpt_add_report(opencreport *o, const char *filename);
/*
 * Add XML report description using a memory buffer (zero-terminated string)
 */
int ocrpt_add_report_from_buffer(opencreport *o, const char *buffer);
/*
 * Parse the reports added up to this point.
 * Allow no deviation from the OpenCReports DTD.
 */
int ocrpt_parse(opencreport *o);
/*
 * Parse the reports added up to this point.
 * Optionally allow deviation from the DTD.
 */
int ocrpt_parse2(opencreport *o, int allow_bad_xml);
/*
 * Execute the reports added up to this point.
 * Parse any unparsed report XMLs with non-strict checks.
 */
int ocrpt_execute(opencreport *o);

#endif /* _OPENCREPORT_H_ */
