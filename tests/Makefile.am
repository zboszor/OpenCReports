AM_CPPFLAGS = -Wall -Werror -O2 -g -I$(top_srcdir)/include

AM_CFLAGS = -rdynamic

LIBS = $(top_builddir)/libsrc/libopencreport.la

$(top_builddir)/libsrc/libopencreport.la:
	$(MAKE) -C $(top_builddir)/libsrc libopencreport.la

TESTS = \
	ocrpt_compiler_test \
	ocrpt_grammar_test ocrpt_expr_test ocrpt_function_test \
	ocrpt_expr_rlib_compat_test \
	ocrpt_expr_format_test \
	ocrpt_environment_test ocrpt_environment2_test \
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
	ocrpt_follower_circular_test \
	ocrpt_follower_invalidref_test \
	ocrpt_rownum_test \
	ocrpt_part_test ocrpt_part_xml_test \
	ocrpt_locale_test \
	ocrpt_variable_expression_test ocrpt_variable_expression_xml_test \
	ocrpt_variable_check_test \
	ocrpt_break_test ocrpt_break_xml_test ocrpt_break_xml2_test \
	ocrpt_break_xml3_test \
	ocrpt_iterative_expr_test \
	ocrpt_break_rownum_xml_test \
	ocrpt_variable_xml_test \
	ocrpt_variable_resetonbreak_xml_test \
	ocrpt_result_copy_test \
	ocrpt_break_multi_test \
	ocrpt_break_multi2_test \
	ocrpt_break_multi3_test \
	ocrpt_break_multi4_test \
	ocrpt_break_multi5_test \
	ocrpt_datetime_expr_test \
	ocrpt_execute_test \
	ocrpt_break_multi6_test \
	ocrpt_break_multi7_test \
	ocrpt_break_multi8_test \
	ocrpt_custom_variable_test \
	ocrpt_custom_variable_xml_test \
	ocrpt_canonicalize_path_test \
	ocrpt_find_file_test \
	ocrpt_color_test

SLOW_TESTS = \
	ocrpt_pgsql_numeric_precision_test \
	ocrpt_pgsql_numeric_precision2_test

# Unstable tests by nature:
# ocrpt_random_test produces a random number which is different every time
# ocrpt_paper_test prints paper sizes and defaults which depends on language settings
# All PDF output tests are unstable because Cairo:
# * sets the /Producer PDF property with its own name and version
# * different Cairo versions support different PDF versions
UNSTABLE_TESTS = \
	ocrpt_random_test \
	ocrpt_paper_test \
	ocrpt_now_test \
	ocrpt_layout_pdf_xml_test \
	ocrpt_layout_pdf_xml2_test

noinst_PROGRAMS = $(TESTS) $(SLOW_TESTS) $(UNSTABLE_TESTS)

$(noinst_PROGRAMS): $(top_builddir)/libsrc/libopencreport.la

all: $(noinst_PROGRAMS)

test: $(TESTS) execute_test.sh
	$(foreach TEST,$(TESTS),LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_test.sh $(TEST) &&) true

slow-test: $(SLOW_TESTS) execute_test.sh
	$(foreach TEST,$(SLOW_TESTS),LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_test.sh $(TEST) &&) true

unstable-test: $(UNSTABLE_TESTS) execute_test.sh
	$(foreach TEST,$(UNSTABLE_TESTS),LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_test.sh $(TEST) &&) true

all-test: test slow-test unstable-test
