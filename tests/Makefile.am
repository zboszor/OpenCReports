AM_CPPFLAGS = -Wall -O2 -g -I$(top_srcdir)/include

AM_CFLAGS = -rdynamic

$(top_builddir)/libsrc/libopencreport.la:
	$(MAKE) -C $(top_builddir)/libsrc libopencreport.la

TESTS = \
	ocrpt_grammar_test ocrpt_expr_test ocrpt_environment_test ocrpt_environment2_test \
	ocrpt_array_test ocrpt_array2_test ocrpt_array_resolve_test ocrpt_array_xml_test \
	ocrpt_csv_test ocrpt_csv2_test ocrpt_csv_array_test \
	ocrpt_csv_xml_test ocrpt_csv_array_xml_test ocrpt_csv_array_xml2_test \
	ocrpt_json_test ocrpt_json2_test ocrpt_json3_test ocrpt_json4_test ocrpt_json5_test \
	ocrpt_json6_test ocrpt_json7_test ocrpt_json8_test \
	ocrpt_json_xml_test ocrpt_json_xml2_test \
	ocrpt_xml_test ocrpt_xml2_test ocrpt_xml3_test ocrpt_xml4_test ocrpt_xml5_test \
	ocrpt_xml_xml_test ocrpt_xml_xml2_test \
	ocrpt_pgsql_test ocrpt_pgsql2_test \
	ocrpt_pgsql_xml_test ocrpt_pgsql_xml2_test ocrpt_pgsql_xml3_test ocrpt_pgsql_xml4_test \
	ocrpt_pgsql_xml5_test ocrpt_pgsql_xml6_test ocrpt_pgsql_xml7_test ocrpt_pgsql_xml8_test \
	ocrpt_pgsql_xml9_test ocrpt_pgsql_xml10_test ocrpt_pgsql_xml11_test \
	ocrpt_mariadb_test ocrpt_mariadb2_test \
	ocrpt_mariadb_xml_test ocrpt_mariadb_xml2_test ocrpt_mariadb_xml3_test \
	ocrpt_mariadb_xml4_test \
	ocrpt_odbc_test ocrpt_odbc2_test ocrpt_odbc3_test ocrpt_odbc4_test \
	ocrpt_odbc5_test ocrpt_odbc6_test ocrpt_odbc7_test \
	ocrpt_odbc_xml_test ocrpt_odbc_xml2_test ocrpt_odbc_xml3_test \
	ocrpt_odbc_xml4_test \
	ocrpt_break_test

noinst_PROGRAMS = \
	$(TESTS) \
	ocrpt_random_test \
	ocrpt_paper_test

test: $(TESTS) execute_test.sh
	$(foreach TEST,$(TESTS),./execute_test.sh $(TEST) && ) true

ocrpt_grammar_test_SOURCES = ocrpt_grammar_test.c
ocrpt_grammar_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_grammar_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_expr_test_SOURCES = ocrpt_expr_test.c
ocrpt_expr_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_expr_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_random_test_SOURCES = ocrpt_random_test.c
ocrpt_random_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_random_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_environment_test_SOURCES = ocrpt_environment_test.c
ocrpt_environment_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_environment_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_environment2_test_SOURCES = ocrpt_environment2_test.c
ocrpt_environment2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_environment2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_array_test_SOURCES = ocrpt_array_test.c
ocrpt_array_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_array_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_array2_test_SOURCES = ocrpt_array2_test.c
ocrpt_array2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_array2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_array_resolve_test_SOURCES = ocrpt_array_resolve_test.c
ocrpt_array_resolve_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_array_resolve_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_array_xml_test_SOURCES = ocrpt_array_xml_test.c
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
ocrpt_csv_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_csv_array_xml_test_SOURCES = ocrpt_csv_array_xml_test.c
ocrpt_csv_array_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv_array_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_csv_array_xml2_test_SOURCES = ocrpt_csv_array_xml2_test.c
ocrpt_csv_array_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_csv_array_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json_test_SOURCES = ocrpt_json_test.c
ocrpt_json_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json2_test_SOURCES = ocrpt_json2_test.c
ocrpt_json2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json3_test_SOURCES = ocrpt_json3_test.c
ocrpt_json3_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json3_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json4_test_SOURCES = ocrpt_json4_test.c
ocrpt_json4_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json4_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json5_test_SOURCES = ocrpt_json5_test.c
ocrpt_json5_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json5_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json6_test_SOURCES = ocrpt_json6_test.c
ocrpt_json6_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json6_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json7_test_SOURCES = ocrpt_json7_test.c
ocrpt_json7_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json7_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json8_test_SOURCES = ocrpt_json8_test.c
ocrpt_json8_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json8_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json_xml_test_SOURCES = ocrpt_json_xml_test.c
ocrpt_json_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_json_xml2_test_SOURCES = ocrpt_json_xml2_test.c
ocrpt_json_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_json_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml_test_SOURCES = ocrpt_xml_test.c
ocrpt_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml2_test_SOURCES = ocrpt_xml2_test.c
ocrpt_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml3_test_SOURCES = ocrpt_xml3_test.c
ocrpt_xml3_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml3_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml4_test_SOURCES = ocrpt_xml4_test.c
ocrpt_xml4_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml4_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml5_test_SOURCES = ocrpt_xml5_test.c
ocrpt_xml5_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml5_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml_xml_test_SOURCES = ocrpt_xml_xml_test.c
ocrpt_xml_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_xml_xml2_test_SOURCES = ocrpt_xml_xml2_test.c
ocrpt_xml_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_xml_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_test_SOURCES = ocrpt_pgsql_test.c
ocrpt_pgsql_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql2_test_SOURCES = ocrpt_pgsql2_test.c
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

ocrpt_pgsql_xml7_test_SOURCES = ocrpt_pgsql_xml7_test.c
ocrpt_pgsql_xml7_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml7_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml8_test_SOURCES = ocrpt_pgsql_xml8_test.c
ocrpt_pgsql_xml8_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml8_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml9_test_SOURCES = ocrpt_pgsql_xml9_test.c
ocrpt_pgsql_xml9_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml9_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml10_test_SOURCES = ocrpt_pgsql_xml10_test.c
ocrpt_pgsql_xml10_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml10_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_pgsql_xml11_test_SOURCES = ocrpt_pgsql_xml11_test.c
ocrpt_pgsql_xml11_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_pgsql_xml11_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_mariadb_test_SOURCES = ocrpt_mariadb_test.c
ocrpt_mariadb_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_mariadb_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_mariadb2_test_SOURCES = ocrpt_mariadb2_test.c
ocrpt_mariadb2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_mariadb2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_mariadb_xml_test_SOURCES = ocrpt_mariadb_xml_test.c
ocrpt_mariadb_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_mariadb_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_mariadb_xml2_test_SOURCES = ocrpt_mariadb_xml2_test.c
ocrpt_mariadb_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_mariadb_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_mariadb_xml3_test_SOURCES = ocrpt_mariadb_xml3_test.c
ocrpt_mariadb_xml3_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_mariadb_xml3_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_mariadb_xml4_test_SOURCES = ocrpt_mariadb_xml4_test.c
ocrpt_mariadb_xml4_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_mariadb_xml4_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc_test_SOURCES = ocrpt_odbc_test.c
ocrpt_odbc_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc2_test_SOURCES = ocrpt_odbc2_test.c
ocrpt_odbc2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc3_test_SOURCES = ocrpt_odbc3_test.c
ocrpt_odbc3_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc3_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc4_test_SOURCES = ocrpt_odbc4_test.c
ocrpt_odbc4_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc4_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc5_test_SOURCES = ocrpt_odbc5_test.c
ocrpt_odbc5_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc5_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc6_test_SOURCES = ocrpt_odbc6_test.c
ocrpt_odbc6_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc6_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc7_test_SOURCES = ocrpt_odbc7_test.c
ocrpt_odbc7_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc7_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc_xml_test_SOURCES = ocrpt_odbc_xml_test.c
ocrpt_odbc_xml_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc_xml_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc_xml2_test_SOURCES = ocrpt_odbc_xml2_test.c
ocrpt_odbc_xml2_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc_xml2_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc_xml3_test_SOURCES = ocrpt_odbc_xml3_test.c
ocrpt_odbc_xml3_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc_xml3_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_odbc_xml4_test_SOURCES = ocrpt_odbc_xml4_test.c
ocrpt_odbc_xml4_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_odbc_xml4_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_break_test_SOURCES = ocrpt_break_test.c
ocrpt_break_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_break_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la

ocrpt_paper_test_SOURCES = ocrpt_paper_test.c
ocrpt_paper_test_LDADD = $(top_builddir)/libsrc/libopencreport.la
ocrpt_paper_test_DEPENDENCIES = $(top_builddir)/libsrc/libopencreport.la
