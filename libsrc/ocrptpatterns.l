%top{
/*
 * Copyright (C) 2019-2022 Zoltán Böszörményi
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <ctype.h>
#include <stdio.h>

#include "scanner.h"
#include "opencreport.h"
#include "ocrpt-private.h"
#include "ocrptgrammar.tab.h"

/* Suppress using the built-in yy_fatal_error function */
#define YY_FATAL_ERROR(msg)	ocrpt_err_printf(msg)
}

%option reentrant
%option bison-bridge
%option bison-locations
%option 8bit
%option never-interactive
 //%option nodefault
%option case-insensitive
%option noinput
%option nounput
%option noyywrap
%option noyyalloc
%option noyyrealloc
%option noyyfree
%option warn

%{

/* Suppress the defined-but-unused function warning  */
static void yynoreturn yy_fatal_error ( const char* msg , yyscan_t yyscanner ) __attribute__((unused));

#define SET_YYLLOC()  (*(yylloc) = yytext - yyextra->scanbuf)
#define ADVANCE_YYLLOC(delta)  ( *(yylloc) += (delta) )

#define startlit()  ( yyextra->literallen = 0 )
static void addlit(char *ytext, int yleng, yyscan_t yyscanner);
static void addlitchar(unsigned char ychar, yyscan_t yyscanner);
static ocrpt_string *litbufdup(yyscan_t yyscanner);

%}

digit		[0-9]
digits		{digit}+
fraction	"."{digits}
ident_start	[A-Za-z\200-\377_]
ident_cont	[A-Za-z\200-\377_0-9]
exponent	[eE][+-]{0,1}{digit}+

/*
 * Special states:
 * xd: quoble quoted strings
 * xq: single quoted strings
 * xi: implicit multiplication with juxtaposition
 */
%x xq
%x xd
%x xi

space					[ \t\n\r\f]
horiz_space				[ \t\f]
newline					[\n\r]
whitespace				{space}*

/* Extended quote
 * xqdouble implements embedded quote, ''''
 */
quote			'
xqstart			{quote}
xqstop			{quote}{whitespace}
xqcontinue		{quote}{whitespace}{quote}
xqxdcontinue	{quote}{whitespace}{dquote}
xqdouble		{quote}{quote}
xqinside		[^']*

/* Double quote
 * xddouble implements embedded quote, """"
 */
dquote			\"
xdstart			{dquote}
xdstop			{dquote}{whitespace}
xdcontinue		{dquote}{whitespace}{dquote}
xdxqcontinue	{dquote}{whitespace}{quote}
xddouble		{dquote}{dquote}
xdinside		[^"]*

%%

 /* Bitshift ops */
"<<"	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("shl", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return BSHIFT;
		}
">>"	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("shr", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return BSHIFT;
		}

 /* Comparison ops */
"==" |
"="		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("eq", 2);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return CMPEQ;
		}
"!=" |
"<>"	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("ne", 2);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return CMPEQ;
		}
">"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("gt", 2);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return CMP;
		}
">="	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("ge", 2);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return CMP;
		}
"<"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("lt", 2);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return CMP;
		}
"<="	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("le", 2);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return CMP;
		}

 /* Logic ops */
"||" |
"or"	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("lor", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return L_OR;
		}

"&&" |
"and"	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("land", 4);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return L_AND;
		}

"!"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("lnot", 4);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}

 /* Arithmetic ops that are also unary ops */
"+"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("add", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}
"-"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("sub", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}

"++"	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("inc", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return INCDEC;
		}

"--"	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("dec", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return INCDEC;
		}

"*"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("mul", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}
"/"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("div", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}
"%"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("mod", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}

 /* Binary ops */
"^"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("pow", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}
"|"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("or", 2);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}
"&"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("and", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}
"~"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("not", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}

 /* Trinary op */
"?"		{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len("iif", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return yytext[0];
		}

 /* Misc single character tokens */
":" |
"." |
"," |
"[" |
"]" |
"(" |
")"		{ return yytext[0]; }

 /* Implied multiplication state return to normal */
<xi>.	{
			yyless(0);
			BEGIN(INITIAL);
			yylval->s = ocrpt_mem_string_new_with_len("mul", 3);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return IMPLMUL;
		}

 /* ")" juxtaposed with an ident */
")"{ident_start} {
			yyless(1);
			BEGIN(xi);
			SET_YYLLOC();
			return yytext[0];
		}
 /* ")" juxtaposed with a number */
")"{digit}	{
			yyless(1);
			BEGIN(xi);
			SET_YYLLOC();
			return yytext[0];
		}
 /* ")" juxtaposed with an ident or a fractional number */
")""."	{
			yyless(1);
			BEGIN(xi);
			SET_YYLLOC();
			return yytext[0];
		}
 /* ")" juxtaposed with "(" */
")""("	{
			yyless(1);
			BEGIN(xi);
			SET_YYLLOC();
			return yytext[0];
		}

 /* Identifier juxtaposed with "(" */
{ident_start}{ident_cont}*"("	{
			yyless(yyleng - 1);
			BEGIN(xi);
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len(yytext, yyleng);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return IDENT;
		}
 /* Identifier as is */
{ident_start}{ident_cont}*	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len(yytext, yyleng);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return IDENT;
		}
 /* Number juxtaposed with "(" */
{digits}{exponent}{0,1}"("	{
			yyless(yyleng - 1);
			BEGIN(xi);
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len(yytext, yyleng);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return DCONST;
		}
 /* Number juxtaposed with an ident */
{digits}{exponent}{0,1}{ident_start} {
			yyless(yyleng - 1);
			BEGIN(xi);
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len(yytext, yyleng);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return DCONST;
		}
 /* Number as is */
{digits}{exponent}{0,1}	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len(yytext, yyleng);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return DCONST;
		}
 /* Fractional number juxtaposed with "(" */
{digits}{0,1}{fraction}{exponent}{0,1}"("	{
			yyless(yyleng - 1);
			BEGIN(xi);
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len(yytext, yyleng);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return DCONST;
		}
 /* Fractional number juxtaposed with an ident */
{digits}{0,1}{fraction}{exponent}{0,1}{ident_start}	{
			yyless(yyleng - 1);
			BEGIN(xi);
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len(yytext, yyleng);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return DCONST;
		}
 /* Fractional number as is */
{digits}{0,1}{fraction}{exponent}{0,1}	{
			SET_YYLLOC();
			yylval->s = ocrpt_mem_string_new_with_len(yytext, yyleng);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return DCONST;
		}

{xqstart}	{
			SET_YYLLOC();
			BEGIN(xq);
			startlit();
		}
<xq>{xqdouble}		{ yyless(2); addlitchar('\'', yyscanner); }
<xq>{xqinside}		{ addlit(yytext, yyleng, yyscanner); }
<xq>{xqcontinue}	{ /* ignore */ }
<xq>{xqxdcontinue}	{ BEGIN(xd); }
<xq>{xqstop}	{
			yyless(1);
			BEGIN(INITIAL);
			yylval->s = litbufdup(yyscanner);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return SCONST;
		}

{xdstart}			{ SET_YYLLOC(); BEGIN(xd); startlit(); }
<xd>{xddouble}		{ yyless(2); addlitchar('\"', yyscanner); }
<xd>{xdinside}		{ addlit(yytext, yyleng, yyscanner); }
<xd>{xdcontinue}	{ /* ignore */ }
<xd>{xdxqcontinue}	{ BEGIN(xq); }
<xd>{xdstop}	{
			yyless(1);
			BEGIN(INITIAL);
			yylval->s = litbufdup(yyscanner);
			parser_yyget_extra(yyscanner)->tokens = ocrpt_list_prepend(parser_yyget_extra(yyscanner)->tokens, yylval->s);
			return SCONST;
		}

{whitespace}		{ /* ignore */ }

.					{ SET_YYLLOC(); scanner_yyerror("syntax error", yyscanner); }

%%

static void addlit(char *ytext, int yleng, yyscan_t yyscanner) {
	struct yyguts_t *yyg = (struct yyguts_t *)yyscanner;

	/* enlarge buffer if needed */
	if ((yyextra->literallen + yleng) >= yyextra->literalalloc) {
		do {
			yyextra->literalalloc *= 2;
		} while ((yyextra->literallen + yleng) >= yyextra->literalalloc);
		yyextra->literalbuf = (char *) ocrpt_mem_realloc(yyextra->literalbuf, yyextra->literalalloc);
	}
	/* append new data */
	memcpy(yyextra->literalbuf + yyextra->literallen, ytext, yleng);
	yyextra->literallen += yleng;
}

static void addlitchar(unsigned char ychar, yyscan_t yyscanner) {
	struct yyguts_t *yyg = (struct yyguts_t *)yyscanner;

	/* enlarge buffer if needed */
	if ((yyextra->literallen + 1) >= yyextra->literalalloc) {
		yyextra->literalalloc *= 2;
		yyextra->literalbuf = (char *) ocrpt_mem_realloc(yyextra->literalbuf, yyextra->literalalloc);
	}
	/* append new data */
	yyextra->literalbuf[yyextra->literallen] = ychar;
	yyextra->literallen += 1;
}

/*
 * Create an ocrpt_mem_malloc'd copy of literalbuf, adding a trailing null.
 */
static ocrpt_string *litbufdup(yyscan_t yyscanner) {
	struct yyguts_t *yyg = (struct yyguts_t *)yyscanner;
	int llen = yyextra->literallen;
	ocrpt_string *new = ocrpt_mem_string_new_with_len(yyextra->literalbuf, llen);
	return new;
}

/*
 * scanner_yyerror - Report a lexer or grammar error.
 *
 * The message's cursor position is whatever YYLLOC was last set to,
 * ie, the start of the current token if called within yylex(), or the
 * most recently lexed token if called from the grammar.
 * This is OK for syntax error messages from the Bison parser, because Bison
 * parsers report error as soon as the first unparsable token is reached.
 * Beware of using yyerror for other purposes, as the cursor position might
 * be misleading!
 */
void scanner_yyerror(const char *message, yyscan_t yyscanner) {
	struct yyguts_t *yyg = (struct yyguts_t *)yyscanner;
	const char *loc = yyextra->scanbuf + *yylloc;
	char *err = NULL;

	if (*loc == YY_END_OF_BUFFER_CHAR) {
		err = ocrpt_mem_malloc(strlen(message) + 18);
		if (err)
			sprintf(err, "%s at end of input", message);
	} else {
		err = ocrpt_mem_malloc(strlen(message) + strlen(loc) + 16);
		if (err)
			sprintf(err, "%s at or near \"%s\"", message, loc);
	}

	parser_yyget_extra(yyscanner)->err = err;

	/* satisfy __attribute__((noreturn)) */
	longjmp(parser_yyget_extra(yyscanner)->env, -1);
}

/*
 * Called before any actual parsing is done
 */
yyscan_t scanner_init(const char *str, core_yy_extra_type *yyext) {
	size_t slen = str ? strlen(str) : 0;
	yyscan_t scanner;
	YYSTYPE ocrpt_lval;

	if (yylex_init(&scanner) != 0) {
		ocrpt_err_printf("yylex_init() failed: %m");
		return NULL;
	}

	memset(&ocrpt_lval, 0, sizeof(ocrpt_lval));
	yyset_lval(&ocrpt_lval, scanner);

	yyset_extra(yyext, scanner);

	/*
	 * Make a scan buffer with special termination needed by flex.
	 */
	yyext->scanbuf = (char *) ocrpt_mem_malloc(slen + 2);
	if (yyext->scanbuf) {
		yyext->scanbuflen = slen;
		if (str)
			memcpy(yyext->scanbuf, str, slen);
		yyext->scanbuf[slen] = yyext->scanbuf[slen + 1] = YY_END_OF_BUFFER_CHAR;
		yy_scan_buffer(yyext->scanbuf, slen + 2, scanner);
	}

	/* initialize literal buffer to a reasonable but expansible size */
	yyext->literalalloc = 1024;
	yyext->literalbuf = (char *) ocrpt_mem_malloc(yyext->literalalloc);
	yyext->literallen = 0;

	return scanner;
}

/*
 * Called after parsing is done to clean up after scanner_init()
 */
void scanner_finish(yyscan_t yyscanner) {
	struct yyguts_t *yyg = (struct yyguts_t *)yyscanner;

	ocrpt_mem_free(yyextra->scanbuf);
	ocrpt_mem_free(yyextra->literalbuf);
	yylex_destroy(yyscanner);
}

void *yyalloc(yy_size_t size, yyscan_t yyscanner) {
	struct yyguts_t *yyg = (struct yyguts_t *)yyscanner;

	(void)yyg;

	return ocrpt_mem_malloc(size);
}

void *yyrealloc(void *ptr, yy_size_t size, yyscan_t yyscanner) {
	struct yyguts_t *yyg = (struct yyguts_t *)yyscanner;

	(void)yyg;

	return ocrpt_mem_realloc(ptr, size);
}

void yyfree(void *ptr, yyscan_t yyscanner) {
	struct yyguts_t *yyg = (struct yyguts_t *)yyscanner;

	(void)yyg;

	ocrpt_mem_free(ptr);
}
