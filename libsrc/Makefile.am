lib_LTLIBRARIES = libopencreport.la

ocrptgrammar.tab.c: ocrptgrammar.y
	bison -d -Wprecedence $<

ocrptgrammar.tab.h: ocrptgrammar.tab.c
	touch $@

lex.yy.c: ocrptpatterns.l ocrptgrammar.tab.h
	$(LEX) $<

libopencreport_la_CFLAGS = \
	-Wall -O2 -g -I$(top_srcdir)/include $(CFLAG_VISIBILITY) \
	$(MPFR_CFLAGS) $(PAPER_CFLAGS) $(UTF8PROC_CFLAGS) \
	$(LIBXML_CFLAGS) $(YAJL_CFLAGS) $(POSTGRESQL_CFLAGS) \
	$(MYSQL_CLIENT_CFLAGS) $(ODBC_CFLAGS)

libopencreport_la_SOURCES = \
	memutil.c listutil.c exprutil.c functions.c \
	lex.yy.c ocrptgrammar.tab.h ocrptgrammar.tab.c \
	api.c free.c parsexml.c datasource.c environment.c \
	array-source.c db-source.c navigation.c breaks.c

libopencreport_la_LIBADD = \
	$(MPFR_LIBS) $(PAPER_LIBS) $(UTF8PROC_LIBS) $(LIBXML_LIBS) \
	$(CSV_LIBS) $(YAJL_LIBS) $(POSTGRESQL_LIBS) $(MYSQL_CLIENT_LIBS) \
	$(ODBC_LIBS)

libopencreport_la_LDFLAGS = \
	-version-info $(OCRPT_LT_CURRENT):$(OCRPT_LT_REVISION):$(OCRPT_LT_AGE) \
	$(EXTRA_LDFLAGS)

CLEANFILES = ocrptgrammar.tab.h ocrptgrammar.tab.c ocrptgrammar.output lex.yy.c
