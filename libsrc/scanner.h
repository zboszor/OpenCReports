/*
 * Common scanner header for lexer and parser
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _SCANNER_H_
#define _SCANNER_H_

#include <stddef.h> /* for definition of size_t */
#include <stdint.h>
#include <setjmp.h>
#include <mpfr.h>

#include "opencreport.h"

#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;

typedef union core_YYSTYPE {
	ocrpt_string	*s;
	ocrpt_expr		*exp;
	ocrpt_list		*l;
} core_YYSTYPE;

#define YYSTYPE core_YYSTYPE
#define YYSTYPE_IS_DECLARED 1

#define YYLTYPE int
#define YY_EXTRA_TYPE core_yy_extra_type *

typedef struct core_yy_extra_type {
	/*
	 * The string the scanner is physically scanning.  We keep this mainly so
	 * that we can cheaply compute the offset of the current token (yytext).
	 */
	char *scanbuf;
	size_t scanbuflen;

	/*
	 * literalbuf is used to accumulate literal values when multiple rules are
	 * needed to parse a single literal.  Call startlit() to reset buffer to
	 * empty, addlit() to add text.  NOTE: the string in literalbuf is NOT
	 * necessarily null-terminated, but there always IS room to add a trailing
	 * null at offset literallen.  We store a null only when we need it.
	 */
	char *literalbuf;
	int32_t literallen;
	int32_t literalalloc;
} core_yy_extra_type;

typedef struct base_yy_extra_type {
	/*
	 * Fields used by the core scanner.
	 */
	core_yy_extra_type core_yy_extra;

	jmp_buf env;
	opencreport *o;
	ocrpt_report *r;
	ocrpt_expr *last_expr;
	ocrpt_list *tokens;
	ocrpt_list *parsed_exprs;
	ocrpt_list *parsed_arglist;
	ocrpt_list *parsed_arglist_stack;
	char *err;
} base_yy_extra_type;

#define parser_yyget_extra(yyscanner) (*((base_yy_extra_type **) (yyscanner)))

/* Entry points in ocrptpatterns.l */
extern yyscan_t scanner_init(const char *str, core_yy_extra_type *yyext);
extern void scanner_finish(yyscan_t *yyscanner);
extern int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param, yyscan_t yyscanner);
extern void yyset_lloc(YYLTYPE *  yylloc_param , yyscan_t yyscanner);
YYSTYPE *yyget_lval(yyscan_t yyscanner);

extern void scanner_yyerror(const char *message, yyscan_t yyscanner) __attribute__((noreturn));

int  yyparse(yyscan_t yyscanner);

#endif /* _SCANNER_H_ */
