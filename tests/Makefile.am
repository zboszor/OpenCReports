AM_CPPFLAGS = -Wall -O2 -g -I$(top_srcdir)/include

$(top_builddir)/libsrc/libopencreport.la:
	$(MAKE) -C $(top_builddir)/libsrc libopencreport.la

noinst_PROGRAMS = \
	ocrpt_grammar_test ocrpt_expr_test \
	ocrpt_array_test ocrpt_array_resolve_test ocrpt_array_xml_test \
	ocrpt_csv_test ocrpt_csv_array_test \
	ocrpt_csv_xml_test ocrpt_csv_array_xml_test ocrpt_csv_array_xml2_test \
	ocrpt_break_test ocrpt_paper_test

ocrpt_grammar_test_SOURCES = ocrpt_grammar_test.c
ocrpt_grammar_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_grammar_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_expr_test_SOURCES = ocrpt_expr_test.c
ocrpt_expr_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_expr_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_array_test_SOURCES = ocrpt_array_test.c
ocrpt_array_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_array_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_array_resolve_test_SOURCES = ocrpt_array_resolve_test.c
ocrpt_array_resolve_test_CFLAGS = -rdynamic
ocrpt_array_resolve_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_array_resolve_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_array_xml_test_SOURCES = ocrpt_array_xml_test.c
ocrpt_array_xml_test_CFLAGS = -rdynamic
ocrpt_array_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_array_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_csv_test_SOURCES = ocrpt_csv_test.c
ocrpt_csv_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_csv_array_test_SOURCES = ocrpt_csv_array_test.c
ocrpt_csv_array_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv_array_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_csv_xml_test_SOURCES = ocrpt_csv_xml_test.c
ocrpt_csv_xml_test_CFLAGS = -rdynamic
ocrpt_csv_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_csv_array_xml_test_SOURCES = ocrpt_csv_array_xml_test.c
ocrpt_csv_array_xml_test_CFLAGS = -rdynamic
ocrpt_csv_array_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv_array_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_csv_array_xml2_test_SOURCES = ocrpt_csv_array_xml2_test.c
ocrpt_csv_array_xml2_test_CFLAGS = -rdynamic
ocrpt_csv_array_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv_array_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_break_test_SOURCES = ocrpt_break_test.c
ocrpt_break_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_break_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_paper_test_SOURCES = ocrpt_paper_test.c
ocrpt_paper_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_paper_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la
