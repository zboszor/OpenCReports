AM_CFLAGS = -Wall -O2 -g

include_HEADERS = opencreport.h

lib_LTLIBRARIES = libopencreport.la

noinst_PROGRAMS = \
	ocrpt_grammar_test ocrpt_expr_test \
	ocrpt_array_test ocrpt_array_resolve_test \
	ocrpt_break_test ocrpt_paper_test

ocrptgrammar.tab.c: ocrptgrammar.y
	bison -d -Wprecedence $<

ocrptgrammar.tab.h: ocrptgrammar.tab.c
	touch $@

lex.yy.c: ocrptpatterns.l ocrptgrammar.tab.h
	$(LEX) $<

libopencreport_la_CFLAGS = \
	$(CFLAG_VISIBILITY) $(MPFR_CFLAGS) $(PAPER_CFLAGS) $(UTF8PROC_CFLAGS)

libopencreport_la_SOURCES = \
	memutil.c listutil.c exprutil.c functions.c \
	lex.yy.c ocrptgrammar.tab.h ocrptgrammar.tab.c \
	api.c free.c parsexml.c datasource.c \
	array-source.c navigation.c breaks.c

libopencreport_la_LIBADD = \
	$(MPFR_LIBS) $(PAPER_LIBS) $(UTF8PROC_LIBS)

libopencreport_la_LDFLAGS = \
	-version-info $(OCRPT_LT_CURRENT):$(OCRPT_LT_REVISION):$(OCRPT_LT_AGE) \
	$(EXTRA_LDFLAGS)

ocrpt_grammar_test_SOURCES = ocrpt_grammar_test.c
ocrpt_grammar_test_LDADD = libopencreport.la

ocrpt_expr_test_SOURCES = ocrpt_expr_test.c
ocrpt_expr_test_LDADD = libopencreport.la

ocrpt_array_test_SOURCES = ocrpt_array_test.c
ocrpt_array_test_LDADD = libopencreport.la

ocrpt_array_resolve_test_SOURCES = ocrpt_array_resolve_test.c
ocrpt_array_resolve_test_CFLAGS = -rdynamic
ocrpt_array_resolve_test_LDADD = libopencreport.la

ocrpt_break_test_SOURCES = ocrpt_break_test.c
ocrpt_break_test_LDADD = libopencreport.la

ocrpt_paper_test_SOURCES = ocrpt_paper_test.c
ocrpt_paper_test_LDADD = libopencreport.la

CLEANFILES = ocrptgrammar.tab.h ocrptgrammar.tab.c ocrptgrammar.output lex.yy.c
