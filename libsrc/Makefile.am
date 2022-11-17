noinst_PROGRAMS = cairotest

cairotest_SOURCES = cairotest.c
cairotest_CFLAGS = $(PDFGEN_CFLAGS)
cairotest_LDADD = $(PDFGEN_LIBS) libopencreport.la

lib_LTLIBRARIES = libopencreport_grammar.la libopencreport.la

ocrptgrammar.tab.c: ocrptgrammar.y
	bison -d -Wprecedence $<

ocrptgrammar.tab.h: ocrptgrammar.tab.c
	touch $@

ocrptpatterns.yy.c: ocrptpatterns.l ocrptgrammar.tab.h
	$(LEX) -o ocrptpatterns.yy.c $<

libopencreport_grammar_la_CFLAGS = \
	-Wall -O2 -g -I$(top_srcdir)/include \
	-D_XOPEN_SOURCE=700 @COMPILE_SYMBOL@ \
	$(CFLAG_VISIBILITY) $(PDFGEN_CFLAGS)

libopencreport_grammar_la_LDFLAGS = -static

libopencreport_grammar_la_SOURCES = \
	ocrptpatterns.yy.c ocrptgrammar.tab.h ocrptgrammar.tab.c

libopencreport_la_DEPENDENCIES = libopencreport_grammar.la

libopencreport_la_CFLAGS = \
	-Wall -Werror -O2 -g -I$(top_srcdir)/include \
	-D_XOPEN_SOURCE=700 @COMPILE_SYMBOL@ \
	$(CFLAG_VISIBILITY) \
	$(MPFR_CFLAGS) $(PAPER_CFLAGS) $(UTF8PROC_CFLAGS) \
	$(LIBXML_CFLAGS) $(YAJL_CFLAGS) $(POSTGRESQL_CFLAGS) \
	$(MYSQL_CLIENT_CFLAGS) $(ODBC_CFLAGS) $(PDFGEN_CFLAGS)

libopencreport_la_SOURCES = \
	memutil.c listutil.c exprutil.c functions.c \
	api.c free.c parsexml.c datasource.c environment.c \
	array-source.c db-source.c navigation.c breaks.c \
	parts.c variables.c strfmon.c datetime.c formatting.c \
	layout.c color.c pdf.c

libopencreport_la_LIBADD = \
	libopencreport_grammar.la \
	$(MPFR_LIBS) $(PAPER_LIBS) $(UTF8PROC_LIBS) $(LIBXML_LIBS) \
	$(CSV_LIBS) $(YAJL_LIBS) $(POSTGRESQL_LIBS) $(MYSQL_CLIENT_LIBS) \
	$(ODBC_LIBS) $(PDFGEN_LIBS)

libopencreport_la_LDFLAGS = \
	-version-info $(OCRPT_LT_CURRENT):$(OCRPT_LT_REVISION):$(OCRPT_LT_AGE) \
	$(EXTRA_LDFLAGS)

CLEANFILES = ocrptgrammar.tab.h ocrptgrammar.tab.c ocrptgrammar.output ocrptpatterns.yy.c

test: cairotest
	LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp $(builddir)/cairotest >$(builddir)/cairotest.pdf
