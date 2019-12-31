AM_CPPFLAGS = -Wall -O2 -g -I$(top_srcdir)/include

$(top_builddir)/libsrc/libopencreport.la:
	$(MAKE) -C $(top_builddir)/libsrc libopencreport.la

noinst_PROGRAMS = \
	ocrpt_grammar_test ocrpt_expr_test ocrpt_environment_test ocrpt_environment2_test \
	ocrpt_array_test ocrpt_array_resolve_test ocrpt_array_xml_test \
	ocrpt_csv_test ocrpt_csv2_test ocrpt_csv_array_test \
	ocrpt_csv_xml_test ocrpt_csv_array_xml_test ocrpt_csv_array_xml2_test \
	ocrpt_json_test ocrpt_json2_test ocrpt_json3_test ocrpt_json4_test ocrpt_json5_test \
	ocrpt_json6_test ocrpt_json7_test ocrpt_json8_test \
	ocrpt_json_xml_test ocrpt_json_xml2_test \
	ocrpt_xml_test ocrpt_xml2_test ocrpt_xml3_test ocrpt_xml4_test ocrpt_xml5_test \
	ocrpt_xml_xml_test ocrpt_xml_xml2_test \
	ocrpt_pgsql_test ocrpt_pgsql2_test \
	ocrpt_pgsql_xml_test ocrpt_pgsql_xml2_test ocrpt_pgsql_xml3_test ocrpt_pgsql_xml4_test \
	ocrpt_pgsql_xml5_test ocrpt_pgsql_xml6_test \
	ocrpt_break_test ocrpt_paper_test

ocrpt_grammar_test_SOURCES = ocrpt_grammar_test.c
ocrpt_grammar_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_grammar_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_expr_test_SOURCES = ocrpt_expr_test.c
ocrpt_expr_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_expr_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_environment_test_SOURCES = ocrpt_environment_test.c
ocrpt_environment_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_environment_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_environment2_test_SOURCES = ocrpt_environment2_test.c
ocrpt_environment2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_environment2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

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

ocrpt_csv2_test_SOURCES = ocrpt_csv2_test.c
ocrpt_csv2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

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

ocrpt_json_test_SOURCES = ocrpt_json_test.c
ocrpt_json_test_CFLAGS = -rdynamic
ocrpt_json_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json2_test_SOURCES = ocrpt_json2_test.c
ocrpt_json2_test_CFLAGS = -rdynamic
ocrpt_json2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json3_test_SOURCES = ocrpt_json3_test.c
ocrpt_json3_test_CFLAGS = -rdynamic
ocrpt_json3_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json3_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json4_test_SOURCES = ocrpt_json4_test.c
ocrpt_json4_test_CFLAGS = -rdynamic
ocrpt_json4_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json4_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json5_test_SOURCES = ocrpt_json5_test.c
ocrpt_json5_test_CFLAGS = -rdynamic
ocrpt_json5_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json5_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json6_test_SOURCES = ocrpt_json6_test.c
ocrpt_json6_test_CFLAGS = -rdynamic
ocrpt_json6_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json6_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json7_test_SOURCES = ocrpt_json7_test.c
ocrpt_json7_test_CFLAGS = -rdynamic
ocrpt_json7_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json7_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json8_test_SOURCES = ocrpt_json8_test.c
ocrpt_json8_test_CFLAGS = -rdynamic
ocrpt_json8_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json8_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json_xml_test_SOURCES = ocrpt_json_xml_test.c
ocrpt_json_xml_test_CFLAGS = -rdynamic
ocrpt_json_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json_xml2_test_SOURCES = ocrpt_json_xml2_test.c
ocrpt_json_xml2_test_CFLAGS = -rdynamic
ocrpt_json_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml_test_SOURCES = ocrpt_xml_test.c
ocrpt_xml_test_CFLAGS = -rdynamic
ocrpt_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml2_test_SOURCES = ocrpt_xml2_test.c
ocrpt_xml2_test_CFLAGS = -rdynamic
ocrpt_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml3_test_SOURCES = ocrpt_xml3_test.c
ocrpt_xml3_test_CFLAGS = -rdynamic
ocrpt_xml3_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml3_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml4_test_SOURCES = ocrpt_xml4_test.c
ocrpt_xml4_test_CFLAGS = -rdynamic
ocrpt_xml4_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml4_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml5_test_SOURCES = ocrpt_xml5_test.c
ocrpt_xml5_test_CFLAGS = -rdynamic
ocrpt_xml5_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml5_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml_xml_test_SOURCES = ocrpt_xml_xml_test.c
ocrpt_xml_xml_test_CFLAGS = -rdynamic
ocrpt_xml_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml_xml2_test_SOURCES = ocrpt_xml_xml2_test.c
ocrpt_xml_xml2_test_CFLAGS = -rdynamic
ocrpt_xml_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_test_SOURCES = ocrpt_pgsql_test.c
ocrpt_pgsql_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql2_test_SOURCES = ocrpt_pgsql_test.c
ocrpt_pgsql2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml_test_SOURCES = ocrpt_pgsql_xml_test.c
ocrpt_pgsql_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml2_test_SOURCES = ocrpt_pgsql_xml2_test.c
ocrpt_pgsql_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml3_test_SOURCES = ocrpt_pgsql_xml3_test.c
ocrpt_pgsql_xml3_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml3_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml4_test_SOURCES = ocrpt_pgsql_xml4_test.c
ocrpt_pgsql_xml4_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml4_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml5_test_SOURCES = ocrpt_pgsql_xml5_test.c
ocrpt_pgsql_xml5_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml5_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml6_test_SOURCES = ocrpt_pgsql_xml6_test.c
ocrpt_pgsql_xml6_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml6_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_break_test_SOURCES = ocrpt_break_test.c
ocrpt_break_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_break_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_paper_test_SOURCES = ocrpt_paper_test.c
ocrpt_paper_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_paper_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la
