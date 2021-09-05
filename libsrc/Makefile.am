lib_LTLIBRARIES = libopencreport_grammar.la libopencreport.la

ocrptgrammar.tab.c: ocrptgrammar.y
	bison -d -Wprecedence $<

ocrptgrammar.tab.h: ocrptgrammar.tab.c
	touch $@

lex.yy.c: ocrptpatterns.l ocrptgrammar.tab.h
	$(LEX) $<

libopencreport_grammar_la_CFLAGS = \
	-Wall -O2 -g -I$(top_srcdir)/include \
	$(CFLAG_VISIBILITY) $(PTHREAD_CFLAGS)

libopencreport_grammar_la_LDFLAGS = -static

libopencreport_grammar_la_SOURCES = \
	lex.yy.c ocrptgrammar.tab.h ocrptgrammar.tab.c

libopencreport_la_DEPENDENCIES = libopencreport_grammar.la

libopencreport_la_CFLAGS = \
	-Wall -Werror -O2 -g -I$(top_srcdir)/include \
	$(CFLAG_VISIBILITY) $(PTHREAD_CFLAGS) \
	$(MPFR_CFLAGS) $(PAPER_CFLAGS) $(UTF8PROC_CFLAGS) \
	$(LIBXML_CFLAGS) $(YAJL_CFLAGS) $(POSTGRESQL_CFLAGS) \
	$(MYSQL_CLIENT_CFLAGS) $(ODBC_CFLAGS)

libopencreport_la_SOURCES = \
	memutil.c listutil.c exprutil.c functions.c \
	api.c free.c parsexml.c datasource.c environment.c \
	array-source.c db-source.c navigation.c breaks.c \
	parts.c variables.c strfmon.c

libopencreport_la_LIBADD = \
	libopencreport_grammar.la \
	$(MPFR_LIBS) $(PAPER_LIBS) $(UTF8PROC_LIBS) $(LIBXML_LIBS) \
	$(CSV_LIBS) $(YAJL_LIBS) $(POSTGRESQL_LIBS) $(MYSQL_CLIENT_LIBS) \
	$(ODBC_LIBS) $(PTHREAD_LIBS)

libopencreport_la_LDFLAGS = \
	-version-info $(OCRPT_LT_CURRENT):$(OCRPT_LT_REVISION):$(OCRPT_LT_AGE) \
	$(EXTRA_LDFLAGS)

CLEANFILES = ocrptgrammar.tab.h ocrptgrammar.tab.c ocrptgrammar.output lex.yy.c
