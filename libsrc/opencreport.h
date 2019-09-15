/*
 * OpenCReports main header
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OPENCREPORTS_H_
#define _OPENCREPORTS_H_

#include <mpfr.h>

/* Main report structure type */
struct opencreport;
typedef struct opencreport opencreport;

struct ocrpt_expr;
typedef struct ocrpt_expr ocrpt_expr;

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
/*
 * Create an expression parse tree from an expression string
 */
ocrpt_expr *ocrpt_expr_parse(opencreport *o, const char *str, char **err);
/*
 * Print an expression on stdout. Good for unit testing.
 */
void ocrpt_expr_print(ocrpt_expr *e);
/*
 * Count the number of expression nodes
 */
int ocrpt_expr_nodes(ocrpt_expr *e);
/*
 * Free an expression parse tree
 */
void ocrpt_free_expr(ocrpt_expr *e);
/*
 * Memory handling wrappers
 */
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

void ocrpt_mem_set_alloc_funcs(ocrpt_mem_malloc_t rmalloc, ocrpt_mem_realloc_t rrealloc, ocrpt_mem_free_t rfree, ocrpt_mem_strdup_t rstrdup, ocrpt_mem_strndup_t rstrndup);

/*
 * Paper size related functions
 */

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

#endif
