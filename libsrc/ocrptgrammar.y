%{
/*
 * OpenCReports expression grammar
 * Copyright (C) 2019 Zoltán Böszörményi
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include "memutil.h"
#include "listutil.h"
#include "scanner.h"
#include "free.h"
#include "functions.h"
#include "opencreport-private.h"

/* We can override alloc/free */
#define YYMALLOC ocrpt_mem_malloc
#define YYFREE   ocrpt_mem_free

/* To suppress "defined but not used" warnings */
static void print_token_value (FILE *, int, YYSTYPE) __attribute__((unused));

#define YYPRINT(File, Type, Value) print_token_value (File, Type, Value)

/*
 * Location tracking support --- simpler than bison's default, since we only
 * want to track the start position not the end position of each nonterminal.
 */
#define YYLLOC_DEFAULT(Current, Rhs, N) \
	do { \
		if ((N) > 0) \
			(Current) = (Rhs)[1]; \
		else \
			(Current) = (-1); \
	} while (0)

static void yyerror(YYLTYPE *yylloc, yyscan_t yyscanner, const char *msg);
#define parser_yyerror(msg)  scanner_yyerror(msg, yyscanner)

static ocrpt_expr *newblankexpr(yyscan_t yyscanner, int type, uint32_t n_ops);
static ocrpt_expr *newstring(yyscan_t yyscanner, const char *s);
static ocrpt_expr *newnumber(yyscan_t yyscanner, const char *n);
static ocrpt_expr *newident(yyscan_t yyscanner, int ident_type, const char *query, const char *name, bool dotprefixed);
static ocrpt_expr *newexpr(yyscan_t yyscanner, const char *fname, int alloc, List *l);

%}

%pure-parser
%expect 0
%locations

%parse-param {yyscan_t yyscanner}
%lex-param   {yyscan_t yyscanner}

%union {
	char		*s;
	ocrpt_expr	*exp;
	List		*l;
}

%token <s> IDENT SCONST DCONST L_AND L_OR INCDEC CMPEQ CMP BSHIFT
%token <s> '*' '/' '%' '^' '|' '&' '~' '!' '?' IMPLMUL

%type <exp> ANYIDENT
%type <exp> exp
%type <l> argelems arglist

%right		'?' ':'
%left		L_OR
%left		L_AND
%left		'|'
%left		'^'
%left		'&'
%left		CMPEQ
%left		CMP
%left		BSHIFT
%left		'+' '-'
%left		'*' '/' '%'
%precedence	'!' '~' UMINUS
%right		INCDEC

%%

exp:
	DCONST					{
								yyset_lloc(&@1, yyscanner);
								$$ = newnumber(yyscanner, $1);
							}
	| SCONST				{
								yyset_lloc(&@1, yyscanner);
								$$ = newstring(yyscanner, $1);
							}
	| ANYIDENT				{	$$ = $1; }
	| ANYIDENT '(' arglist ')' {
								const char *fname = $1->name;

								yyset_lloc(&@1, yyscanner);

								/*
								 * This clause hits when there is a whitespace
								 * delimiter between ANYIDENT and '('.
								 * Only a function call is expected that way,
								 * that a domain-qualified or dot-prefixed
								 * identifier cannot satisfy. An invalid function
								 * name will be caught by newexpr().
								 */
								if ($1->query || $1->dotprefixed)
									parser_yyerror("syntax error");

								$1->name = NULL;
								parser_yyget_extra(yyscanner)->parsed_exprs = list_remove(parser_yyget_extra(yyscanner)->parsed_exprs, $1);
								ocrpt_free_expr($1);
								$$ = newexpr(yyscanner, fname, 0, $3);
							}
	| '(' exp ')'			{
								yyset_lloc(&@1, yyscanner);
								$$ = $2;
							}
	| exp '?' exp ':' exp	{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, $5, NULL));
							}
	| exp L_OR exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| exp L_AND exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| '!' exp				{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $1, 1, makelist($2, NULL));
							}
	| exp '|' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| exp '^' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| exp '&' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| '~' exp				{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $1, 1, makelist($2, NULL));
							}
	| exp CMPEQ exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| exp CMP exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| exp BSHIFT exp		{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| exp '+' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, "add", 1, makelist($1, $3, NULL));
							}
	| exp '-' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, "sub", 1, makelist($1, $3, NULL));
							}
	| exp '*' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| exp '/' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| exp '%' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, $3, NULL));
							}
	| DCONST IMPLMUL ANYIDENT
							{
								ocrpt_expr *number = newnumber(yyscanner, $1);
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist(number, $3, NULL));
							}
	| DCONST IMPLMUL '(' exp ')'
							{
								ocrpt_expr *number = newnumber(yyscanner, $1);
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist(number, $4, NULL));
							}
	| ANYIDENT IMPLMUL '(' arglist ')'
							{
								yyset_lloc(&@1, yyscanner);

								if (list_length($4) == 1)  {
									parser_yyget_extra(yyscanner)->parsed_exprs = list_remove(parser_yyget_extra(yyscanner)->parsed_exprs, $4->data);

									if ($1->query || $1->dotprefixed) {
										/*
										 * Only one argument is in arglist and the identifier is
										 * either domain-qualified or dot-prefixed: it can only
										 * be multiplication. Runtime will sort out if the
										 * identifier is valid.
										 */
										List *list = $4;

										list = list_prepend(list, $1);
										parser_yyget_extra(yyscanner)->parsed_arglist = list;

										$$ = newexpr(yyscanner, $2, 1, list);
									} else {
										/*
										 * Only one argument is in arglist, we need to check
										 * if the ident is a supported function name.
										 */
										ocrpt_function *f = ocrpt_find_function($1->name);
										if (f) {
											const char *fname = $1->name;

											$1->name = NULL;
											ocrpt_free_expr($1);
											$$ = newexpr(yyscanner, fname, 0, $4);
										} else {
											List *list = $4;

											list = list_prepend(list, $1);
											parser_yyget_extra(yyscanner)->parsed_arglist = list;

											$$ = newexpr(yyscanner, $2, 1, list);
										}
									}
								} else if ($1->query || $1->dotprefixed) {
									parser_yyerror("invalid function name");
								} else {
									const char *fname = $1->name;

									$1->name = NULL;
									parser_yyget_extra(yyscanner)->parsed_exprs = list_remove(parser_yyget_extra(yyscanner)->parsed_exprs, $1);
									ocrpt_free_expr($1);
									$$ = newexpr(yyscanner, fname, 0, $4);
								}
							}
	| '(' exp ')' IMPLMUL DCONST
							{
								ocrpt_expr *number = newnumber(yyscanner, $5);
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $4, 1, makelist($2, number, NULL));
							}
	| '(' exp ')' IMPLMUL ANYIDENT
							{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $4, 1, makelist($2, $5, NULL));
							}
	| '(' exp ')' IMPLMUL '(' exp ')'
							{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $4, 1, makelist($2, $6, NULL));
							}
	| '|' exp '|'			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, "abs", 1, makelist($2, NULL));
							}
	| '-' exp %prec UMINUS	{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, "uminus", 1, makelist($2, NULL));
							}
	| '+' exp %prec UMINUS	{
								yyset_lloc(&@1, yyscanner);
								$$ = $2;
							}
	| INCDEC exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $1, 1, makelist($2, NULL));
							}
	| exp INCDEC			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, 1, makelist($1, NULL));
							}
	;

ANYIDENT:
	IDENT					{
								yyset_lloc(&@1, yyscanner);
								if (strcmp($1, "yes") == 0 || strcmp($1, "true") == 0) {
									$$ = newnumber(yyscanner, ocrpt_mem_strdup("1"));
								} else if (strcmp($1, "no") == 0 || strcmp($1, "false") == 0) {
									$$ = newnumber(yyscanner, ocrpt_mem_strdup("0"));
								} else
									$$ = newident(yyscanner, OCRPT_EXPR_IDENT, NULL, $1, false); }
	| '.' IDENT				{
								yyset_lloc(&@1, yyscanner);
								$$ = newident(yyscanner, OCRPT_EXPR_IDENT, NULL, $2, true);
							}
	| '.' SCONST			{
								yyset_lloc(&@1, yyscanner);
								$$ = newident(yyscanner, OCRPT_EXPR_IDENT, NULL, $2, true);
							}
	| IDENT '.' IDENT		{
								yyset_lloc(&@1, yyscanner);
								if (strcmp($1, "m") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_MVAR, $1, $3, false);
								else if (strcmp($1, "r") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_RVAR, $1, $3, false);
								else if (strcmp($1, "v") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_VVAR, $1, $3, false);
								else
									$$ = newident(yyscanner, OCRPT_EXPR_IDENT, $1, $3, false);
							}
	| IDENT '.' SCONST	{
								yyset_lloc(&@1, yyscanner);
								if (strcmp($1, "m") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_MVAR, $1, $3, false);
								else if (strcmp($1, "r") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_RVAR, $1, $3, false);
								else if (strcmp($1, "v") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_VVAR, $1, $3, false);
								else
									$$ = newident(yyscanner, OCRPT_EXPR_IDENT, $1, $3, false);
							}
	| SCONST '.' IDENT		{
								yyset_lloc(&@1, yyscanner);
								$$ = newident(yyscanner, OCRPT_EXPR_IDENT, $1, $3, false);
							}
	| SCONST '.' SCONST		{
								yyset_lloc(&@1, yyscanner);
								$$ = newident(yyscanner, OCRPT_EXPR_IDENT, $1, $3, false);
							}
	;

argelems:
	exp						{
								List *l = makelist1($1);
								$$ = parser_yyget_extra(yyscanner)->parsed_arglist = l;
							}
	| argelems ',' exp		{
								List *l = list_append($1, $3);
								$$ = parser_yyget_extra(yyscanner)->parsed_arglist = l;
							}
	;

arglist:
	/* empty */				{ $$ = parser_yyget_extra(yyscanner)->parsed_arglist = NULL; }
	| argelems				{ $$ = parser_yyget_extra(yyscanner)->parsed_arglist = $1; }
	;

%%

/* To suppress "defined but not used" warnings */
static const yytype_uint16 yytoknum[] __attribute__((unused));

void yyset_debug(int  _bdebug , yyscan_t yyscanner);

/* parser_init()
 * Initialize to parse one query string
 */
void parser_init(base_yy_extra_type *yyext, opencreport *o) {
	yyext->tokens = NULL;
	yyext->parsed_exprs = NULL;
	yyext->parsed_arglist = NULL;
	yyext->o = o;
	yyext->err = NULL;
}

ocrpt_expr *ocrpt_expr_parse(opencreport *o, const char *str, char **err) {
	yyscan_t yyscanner;
	base_yy_extra_type yyextra;
	int yyresult = 1;

	if (setjmp(yyextra.env) == 0) {
#if YYDEBUG
		/*
		 * Gammar debugging - needs bison -t to be effective.
		 */
		 yydebug = 1;
#endif

		/* initialize the flex scanner */
		yyscanner = scanner_init(str, &yyextra.core_yy_extra);

#if YYDEBUG
		/*
		 * Lexer debugging - needs flex -d to be effective
		 */
		yyset_debug(1, yyscanner);
#endif

		/* initialize the bison parser */
		parser_init(&yyextra, o);

		/* Parse! */
		yyresult = yyparse(yyscanner);
	}

	/* Clean up (release memory) */
	scanner_finish(yyscanner);

	if (yyresult) {
		list_free_deep(yyextra.tokens, (ocrpt_mem_free_t)ocrpt_mem_free);
		list_free(yyextra.parsed_arglist);
		list_free_deep(yyextra.parsed_exprs, (ocrpt_mem_free_t)ocrpt_free_expr);

		if (err)
			*err = yyextra.err;
		else
			ocrpt_strfree(yyextra.err);
		return NULL;
	}

	list_free_deep(yyextra.tokens, (ocrpt_mem_free_t)ocrpt_mem_free);
	list_free(yyextra.parsed_arglist);
	list_free(yyextra.parsed_exprs);
	return yyextra.last_expr;
}

/*
 * The signature of this function is required by bison.  However, we
 * ignore the passed yylloc and instead use the last token position
 * available from the scanner.
 */
static void yyerror(YYLTYPE *yylloc, yyscan_t yyscanner, const char *msg) {
	parser_yyerror(msg);
}

static void print_token_value (FILE *file, int type, YYSTYPE value) {
	switch (type) {
	case SCONST:
	case DCONST:
	case IDENT:
		fprintf(file, "%s", value.s);
		break;
	default:
		fprintf(file, "%d", type);
		break;
	}
}

static ocrpt_expr *newblankexpr(yyscan_t yyscanner, int type, uint32_t n_ops) {
	ocrpt_expr *e;

	e = ocrpt_mem_malloc(sizeof(ocrpt_expr));
	if (!e)
		parser_yyerror("out of memory");

	parser_yyget_extra(yyscanner)->last_expr = e;
	parser_yyget_extra(yyscanner)->parsed_exprs = list_prepend(parser_yyget_extra(yyscanner)->parsed_exprs, e);

	e->type = type;
	e->n_ops = n_ops;
	e->uops = (n_ops > 0 ? ocrpt_mem_malloc(n_ops * sizeof(ocrpt_united_expr)) : NULL);

	return e;
}

static ocrpt_expr *newstring(yyscan_t yyscanner, const char *s) {
	ocrpt_expr *e = newblankexpr(yyscanner, OCRPT_EXPR_STRING, 0);

	e->string = s;
	parser_yyget_extra(yyscanner)->tokens = list_remove(parser_yyget_extra(yyscanner)->tokens, s);

	return e;
}

static ocrpt_expr *newnumber(yyscan_t yyscanner, const char *n) {
	ocrpt_expr *e = newblankexpr(yyscanner, OCRPT_EXPR_NUMBER, 0);

	e->string = n;
	parser_yyget_extra(yyscanner)->tokens = list_remove(parser_yyget_extra(yyscanner)->tokens, n);

	return e;
}

static ocrpt_expr *newident(yyscan_t yyscanner, int ident_type, const char *query, const char *name, bool dotprefixed) {
	ocrpt_expr *e;

	if ((query && query[0] == '\0') || (name && name[0] == '\0'))
		parser_yyerror("syntax error: quoted identifier cannot be empty");

	e = newblankexpr(yyscanner, ident_type, 0);
	if (query)
		parser_yyget_extra(yyscanner)->tokens = list_remove(parser_yyget_extra(yyscanner)->tokens, query);
	e->query = query;
	if (name)
		parser_yyget_extra(yyscanner)->tokens = list_remove(parser_yyget_extra(yyscanner)->tokens, name);
	e->name = name;
	e->dotprefixed = dotprefixed;

	return e;
}

static ocrpt_expr *newexpr(yyscan_t yyscanner, const char *fname, int alloc, List *l) {
	ocrpt_expr *e;
	List *ptr;
	int idx;
	ocrpt_function *f;

	f = ocrpt_find_function(fname);
	if (!f) {
		if (!alloc)
			ocrpt_strfree(fname);
		parser_yyerror("invalid function name");
	}

	e = newblankexpr(yyscanner, OCRPT_EXPR, list_length(l));
	parser_yyget_extra(yyscanner)->tokens = list_remove(parser_yyget_extra(yyscanner)->tokens, fname);
	e->fname = (alloc ? ocrpt_mem_strdup(fname) : fname);

	for (ptr = l, idx = 0; ptr; ptr = ptr->next, idx++) {
		parser_yyget_extra(yyscanner)->parsed_exprs = list_remove(parser_yyget_extra(yyscanner)->parsed_exprs, ptr->data);
		e->uops[idx].type = OCRPT_EXPR;
		e->uops[idx].op = (ocrpt_expr *)ptr->data;
	}

	parser_yyget_extra(yyscanner)->parsed_arglist = NULL;
	list_free(l);

	return e;
}
