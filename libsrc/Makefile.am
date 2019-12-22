AM_CFLAGS = -Wall -O2 -g

include_HEADERS = opencreport.h

lib_LTLIBRARIES = libopencrpt.la

noinst_PROGRAMS = \
	ocrpt_grammar_test ocrpt_expr_test \
	ocrpt_array_test \
	ocrpt_paper_test

ocrptgrammar.tab.c: ocrptgrammar.y
	bison -d -Wprecedence $<

ocrptgrammar.tab.h: ocrptgrammar.tab.c
	touch $@

lex.yy.c: ocrptpatterns.l ocrptgrammar.tab.h
	$(LEX) $<

libopencrpt_la_CFLAGS = \
	$(CFLAG_VISIBILITY) $(MPFR_CFLAGS) $(PAPER_CFLAGS) $(UTF8PROC_CFLAGS)

libopencrpt_la_SOURCES = \
	memutil.c listutil.c exprutil.c functions.c \
	lex.yy.c ocrptgrammar.tab.h ocrptgrammar.tab.c \
	api.c free.c parsexml.c datasource.c \
	array-source.c navigation.c

libopencrpt_la_LIBADD = \
	$(MPFR_LIBS) $(PAPER_LIBS) $(UTF8PROC_LIBS)

libopencrpt_la_LDFLAGS = \
	-version-info $(OCRPT_LT_CURRENT):$(OCRPT_LT_REVISION):$(OCRPT_LT_AGE) \
	$(EXTRA_LDFLAGS)

ocrpt_grammar_test_SOURCES = ocrpt_grammar_test.c
ocrpt_grammar_test_LDADD = libopencrpt.la

ocrpt_expr_test_SOURCES = ocrpt_expr_test.c
ocrpt_expr_test_LDADD = libopencrpt.la

ocrpt_array_test_SOURCES = ocrpt_array_test.c
ocrpt_array_test_LDADD = libopencrpt.la

ocrpt_paper_test_SOURCES = ocrpt_paper_test.c
ocrpt_paper_test_LDADD = libopencrpt.la

CLEANFILES = ocrptgrammar.tab.h ocrptgrammar.tab.c ocrptgrammar.output lex.yy.c
