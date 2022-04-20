%{
/*
 * OpenCReports expression grammar
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <alloca.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include "opencreport.h"
#include "exprutil.h"
#include "scanner.h"

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

static ocrpt_expr *newblankexpr1(yyscan_t yyscanner, enum ocrpt_expr_type type, uint32_t n_ops);
static ocrpt_expr *newstring(yyscan_t yyscanner, ocrpt_string *s);
static ocrpt_expr *newnumber(yyscan_t yyscanner, ocrpt_string *n);
static ocrpt_expr *newident(yyscan_t yyscanner, int ident_type, ocrpt_string *query, ocrpt_string *name, bool dotprefixed);
static ocrpt_expr *newexpr(yyscan_t yyscanner, ocrpt_string *fname, ocrpt_list *l);

%}

%define api.pure full
%expect 0
%locations

%parse-param {yyscan_t yyscanner}
%lex-param   {yyscan_t yyscanner}

%union {
	ocrpt_string	*s;
	ocrpt_expr		*exp;
	List			*l;
}

%token <s>   IDENT SCONST DCONST L_AND L_OR INCDEC CMPEQ CMP BSHIFT
%token <s>   '+' '-' '*' '/' '%' '^' '|' '&' '~' '!' '?' IMPLMUL

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
%precedence	'!'
%precedence	'~' UMINUS
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
								base_yy_extra_type *extra = parser_yyget_extra(yyscanner);
								ocrpt_string *fname = $1->name;

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
								extra->parsed_exprs = ocrpt_list_remove(extra->parsed_exprs, $1);
								ocrpt_expr_free(extra->o, extra->r, $1);
								$$ = newexpr(yyscanner, fname, $3);
							}
	| '(' exp ')'			{
								yyset_lloc(&@1, yyscanner);
								$2->parenthesized = true;
								$$ = $2;
							}
	| exp '?' exp ':' exp	{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, $5, NULL));
							}
	| exp L_OR exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp L_AND exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| '!' exp %prec UMINUS	{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $1, ocrpt_makelist($2, NULL));
							}
	| exp '!'				{
								base_yy_extra_type *extra = parser_yyget_extra(yyscanner);
								ocrpt_string *fact;
								yyset_lloc(&@1, yyscanner);
								extra->tokens = ocrpt_list_remove(extra->tokens, $2);
								ocrpt_mem_string_free($2, true);
								fact = ocrpt_mem_string_new_with_len("factorial", 9);
								$$ = newexpr(yyscanner, fact, ocrpt_makelist($1, NULL));
							}
	| exp '|' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp '^' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp '&' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| '~' exp				{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $1, ocrpt_makelist($2, NULL));
							}
	| exp CMPEQ exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp CMP exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp BSHIFT exp		{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp '+' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp '-' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp '*' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp '/' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| exp '%' exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, $3, NULL));
							}
	| DCONST IMPLMUL ANYIDENT
							{
								ocrpt_expr *number = newnumber(yyscanner, $1);
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist(number, $3, NULL));
							}
	| DCONST IMPLMUL '(' exp ')'
							{
								ocrpt_expr *number = newnumber(yyscanner, $1);
								ocrpt_expr *paren = newexpr(yyscanner, $2, ocrpt_makelist(number, $4, NULL));
								yyset_lloc(&@1, yyscanner);
								paren->parenthesized = true;
								$$ = paren;
							}
	| ANYIDENT IMPLMUL '(' arglist ')'
							{
								base_yy_extra_type *extra = parser_yyget_extra(yyscanner);
								yyset_lloc(&@1, yyscanner);

								if (ocrpt_list_length($4) == 1)  {
									extra->parsed_exprs = ocrpt_list_remove(extra->parsed_exprs, $4->data);

									if ($1->query || $1->dotprefixed) {
										/*
										 * Only one argument is in arglist and the identifier is
										 * either domain-qualified or dot-prefixed: it can only
										 * be multiplication. Runtime will sort out if the
										 * identifier is valid.
										 */
										ocrpt_list *list = $4;
										ocrpt_expr *paren = (ocrpt_expr *)list->data;

										paren->parenthesized = true;

										list = ocrpt_list_prepend(list, $1);
										extra->parsed_arglist = list;

										$$ = newexpr(yyscanner, $2, list);
									} else {
										/*
										 * Only one argument is in arglist, we need to check
										 * if the ident is a supported function name.
										 */
										const ocrpt_function *f = ocrpt_function_get(extra->o, $1->name->str);
										if (f) {
											ocrpt_string *fname = $1->name;

											$1->name = NULL;
											extra->parsed_exprs = ocrpt_list_remove(extra->parsed_exprs, $1);
											ocrpt_expr_free(extra->o, extra->r, $1);
											$$ = newexpr(yyscanner, fname, $4);
										} else {
											ocrpt_list *list = $4;
											ocrpt_expr *paren = (ocrpt_expr *)list->data;

											paren->parenthesized = true;

											list = ocrpt_list_prepend(list, $1);
											extra->parsed_arglist = list;

											$$ = newexpr(yyscanner, $2, list);
										}
									}
								} else if ($1->query || $1->dotprefixed) {
									parser_yyerror("invalid function name");
								} else {
									ocrpt_string *fname = $1->name;

									$1->name = NULL;
									extra->parsed_exprs = ocrpt_list_remove(extra->parsed_exprs, $1);
									ocrpt_expr_free(extra->o, extra->r, $1);
									$$ = newexpr(yyscanner, fname, $4);
								}
							}
	| '(' exp ')' IMPLMUL DCONST
							{
								ocrpt_expr *number = newnumber(yyscanner, $5);
								yyset_lloc(&@1, yyscanner);
								$2->parenthesized = true;
								$$ = newexpr(yyscanner, $4, ocrpt_makelist($2, number, NULL));
							}
	| '(' exp ')' IMPLMUL ANYIDENT
							{
								yyset_lloc(&@1, yyscanner);
								$2->parenthesized = true;
								$$ = newexpr(yyscanner, $4, ocrpt_makelist($2, $5, NULL));
							}
	| '(' exp ')' IMPLMUL '(' exp ')'
							{
								yyset_lloc(&@1, yyscanner);
								$2->parenthesized = true;
								$6->parenthesized = true;
								$$ = newexpr(yyscanner, $4, ocrpt_makelist($2, $6, NULL));
							}
	| '|' exp '|'			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, ocrpt_mem_string_new_with_len("abs", 3), ocrpt_makelist($2, NULL));
							}
	| '-' exp %prec UMINUS	{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, ocrpt_mem_string_new_with_len("uminus", 6), ocrpt_makelist($2, NULL));
							}
	| '+' exp %prec UMINUS	{
								yyset_lloc(&@1, yyscanner);
								$$ = $2;
							}
	| INCDEC exp			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $1, ocrpt_makelist($2, NULL));
							}
	| exp INCDEC			{
								yyset_lloc(&@1, yyscanner);
								$$ = newexpr(yyscanner, $2, ocrpt_makelist($1, NULL));
							}
	;

ANYIDENT:
	IDENT					{
								yyset_lloc(&@1, yyscanner);
								if (strcmp($1->str, "yes") == 0 || strcmp($1->str, "true") == 0) {
									$$ = newnumber(yyscanner, ocrpt_mem_string_new_with_len("1", 1));
								} else if (strcmp($1->str, "no") == 0 || strcmp($1->str, "false") == 0) {
									$$ = newnumber(yyscanner, ocrpt_mem_string_new_with_len("0", 1));
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
								if (strcmp($1->str, "m") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_MVAR, $1, $3, false);
								else if (strcmp($1->str, "r") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_RVAR, $1, $3, false);
								else if (strcmp($1->str, "v") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_VVAR, $1, $3, false);
								else
									$$ = newident(yyscanner, OCRPT_EXPR_IDENT, $1, $3, false);
							}
	| IDENT '.' SCONST	{
								yyset_lloc(&@1, yyscanner);
								if (strcmp($1->str, "m") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_MVAR, $1, $3, false);
								else if (strcmp($1->str, "r") == 0)
									$$ = newident(yyscanner, OCRPT_EXPR_RVAR, $1, $3, false);
								else if (strcmp($1->str, "v") == 0)
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
								ocrpt_list *l = ocrpt_makelist1($1);
								$$ = parser_yyget_extra(yyscanner)->parsed_arglist = l;
							}
	| argelems ',' exp		{
								ocrpt_list *l = ocrpt_list_append($1, $3);
								$$ = parser_yyget_extra(yyscanner)->parsed_arglist = l;
							}
	;

arglist:
	/* empty */				{ $$ = parser_yyget_extra(yyscanner)->parsed_arglist = NULL; }
	| argelems				{ $$ = parser_yyget_extra(yyscanner)->parsed_arglist = $1; }
	;

%%

void yyset_debug(int  _bdebug , yyscan_t yyscanner);

/* parser_init()
 * Initialize to parse one query string
 */
void parser_init(base_yy_extra_type *yyext, opencreport *o, ocrpt_report *r) {
	yyext->tokens = NULL;
	yyext->parsed_exprs = NULL;
	yyext->parsed_arglist = NULL;
	yyext->o = o;
	yyext->r = r;
	yyext->err = NULL;
}

static void ocrpt_grammar_free_token(ocrpt_string *token) {
	ocrpt_mem_string_free(token, true);
}

DLL_EXPORT_SYM ocrpt_expr *ocrpt_expr_parse(opencreport *o, ocrpt_report *r, const char *str, char **err) {
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
		parser_init(&yyextra, o, r);

		/* Parse! */
		yyresult = yyparse(yyscanner);
	}

	/* Clean up (release memory) */
	scanner_finish(yyscanner);

	if (yyresult) {
		ocrpt_list *ptr;

		ocrpt_list_free_deep(yyextra.tokens, (ocrpt_mem_free_t)ocrpt_grammar_free_token);
		ocrpt_list_free(yyextra.parsed_arglist);

		for (ptr = yyextra.parsed_exprs; ptr; ptr = ptr->next) {
			ocrpt_expr *e = (ocrpt_expr *)ptr->data;
			ocrpt_expr_free(o, r, e);
		}
		ocrpt_list_free(yyextra.parsed_exprs);

		if (err)
			*err = yyextra.err;
		else
			ocrpt_strfree(yyextra.err);
		return NULL;
	}

	ocrpt_list_free_deep(yyextra.tokens, (ocrpt_mem_free_t)ocrpt_grammar_free_token);
	ocrpt_list_free(yyextra.parsed_arglist);
	ocrpt_list_free(yyextra.parsed_exprs);

	if (r && !r->executing && !r->dont_add_exprs) {
		r->exprs = ocrpt_list_end_append(r->exprs, &r->exprs_last, yyextra.last_expr);
		yyextra.last_expr->result_index = r->num_expressions++;
		yyextra.last_expr->result_index_set = true;
	} else
		yyextra.last_expr->result_index = -1;

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
		fprintf(file, "%s", value.s->str);
		break;
	default:
		fprintf(file, "%d", type);
		break;
	}
}

static ocrpt_expr *newblankexpr1(yyscan_t yyscanner, enum ocrpt_expr_type type, uint32_t n_ops) {
	base_yy_extra_type *extra = parser_yyget_extra(yyscanner);
	ocrpt_expr *e = newblankexpr(extra->o, extra->r, type, n_ops);

	if (!e)
		parser_yyerror("out of memory");

	extra->last_expr = e;
	extra->parsed_exprs = ocrpt_list_prepend(extra->parsed_exprs, e);

	return e;
}

static ocrpt_expr *newstring(yyscan_t yyscanner, ocrpt_string *s) {
	ocrpt_expr *e = newblankexpr1(yyscanner, OCRPT_EXPR_STRING, 0);
	base_yy_extra_type *extra = parser_yyget_extra(yyscanner);
	opencreport *o = extra->o;

	extra->tokens = ocrpt_list_remove(extra->tokens, s);
	ocrpt_expr_init_result(o, e, OCRPT_RESULT_STRING);
	e->result[o->residx]->string = s;
	e->result[o->residx]->string_owned = true;
	e->result[ocrpt_expr_next_residx(o->residx)] = e->result[o->residx];
	e->result[ocrpt_expr_next_residx(ocrpt_expr_next_residx(o->residx))] = e->result[o->residx];

	return e;
}

static ocrpt_expr *newnumber(yyscan_t yyscanner, ocrpt_string *n) {
	ocrpt_expr *e = newblankexpr1(yyscanner, OCRPT_EXPR_NUMBER, 0);
	base_yy_extra_type *extra = parser_yyget_extra(yyscanner);
	opencreport *o = extra->o;

	extra->tokens = ocrpt_list_remove(extra->tokens, n);
	ocrpt_expr_init_result(o, e, OCRPT_RESULT_NUMBER);
	mpfr_set_str(e->result[o->residx]->number, n->str, 10, o->rndmode);
	e->result[ocrpt_expr_next_residx(o->residx)] = e->result[o->residx];
	e->result[ocrpt_expr_next_residx(ocrpt_expr_next_residx(o->residx))] = e->result[o->residx];
	ocrpt_mem_string_free(n, true);

	return e;
}

static ocrpt_expr *newident(yyscan_t yyscanner, int ident_type, ocrpt_string *query, ocrpt_string *name, bool dotprefixed) {
	base_yy_extra_type *extra = parser_yyget_extra(yyscanner);
	ocrpt_expr *e;

	if ((query && query->str[0] == '\0') || (name && name->str[0] == '\0'))
		parser_yyerror("syntax error: quoted identifier cannot be empty");

	e = newblankexpr1(yyscanner, ident_type, 0);
	if (query)
		extra->tokens = ocrpt_list_remove(extra->tokens, query);
	e->query = query;
	if (name)
		extra->tokens = ocrpt_list_remove(extra->tokens, name);
	e->name = name;
	e->dotprefixed = dotprefixed;

	return e;
}

static ocrpt_expr *newexpr(yyscan_t yyscanner, ocrpt_string *fname, ocrpt_list *l) {
	base_yy_extra_type *extra = parser_yyget_extra(yyscanner);
	ocrpt_expr *e;
	ocrpt_list *ptr;
	int idx;
	const ocrpt_function *f;

	f = ocrpt_function_get(extra->o, fname->str);
	if (!f) {
		ocrpt_mem_string_free(fname, true);
		parser_yyerror("invalid function name");
	}

	if (f->n_ops > 0 && f->n_ops != ocrpt_list_length(l)) {
		char *msg;
		int len;

		len = fname->len + sizeof("invalid number of operands for %s");
		msg = alloca(len + 1);
		sprintf(msg, "invalid number of operands for %s", fname->str);

		ocrpt_mem_string_free(fname, true);

		parser_yyerror(msg);
	}

	extra->tokens = ocrpt_list_remove(extra->tokens, fname);

	e = newblankexpr1(yyscanner, OCRPT_EXPR, ocrpt_list_length(l));
	ocrpt_mem_string_free(fname, true);
	e->func = f;

	for (ptr = l, idx = 0; ptr; ptr = ptr->next, idx++) {
		extra->parsed_exprs = ocrpt_list_remove(extra->parsed_exprs, ptr->data);
		e->ops[idx] = (ocrpt_expr *)ptr->data;
	}

	extra->parsed_arglist = NULL;
	ocrpt_list_free(l);

	return e;
}
