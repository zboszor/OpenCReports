AM_CPPFLAGS = \
	-Wall @WERRORFLAG@ -O2 -g \
	-D_XOPEN_SOURCE=700 \
	-I$(top_srcdir)/include

AM_LDFLAGS = @DYNAMIC_LINK_FLAG@

LIBS = $(top_builddir)/libsrc/libopencreport.la

$(top_builddir)/libsrc/libopencreport.la:
	$(MAKE) -C $(top_builddir)/libsrc libopencreport.la

compiler_cc_test_SOURCES = compiler_cc_test.cc

if ENABLE_PGSQL_TESTS

PGSQL_TESTS = \
	pgsql_test pgsql2_test \
	pgsql_xml_test pgsql_xml2_test pgsql_xml3_test pgsql_xml4_test \
	pgsql_xml5_test pgsql_xml6_test pgsql_xml7_test pgsql_xml8_test \
	pgsql_xml9_test pgsql_xml10_test pgsql_xml11_test

endif

TESTS = \
	compiler_test \
	compiler_cc_test \
	grammar_test expr_test function_test \
	expr_format_test \
	environment_test environment2_test \
	array_test array2_test array_resolve_test array_xml_test \
	csv_test csv2_test csv_array_test \
	csv_xml_test csv_array_xml_test csv_array_xml2_test \
	json_test json2_test json3_test json4_test json5_test \
	json6_test json7_test json8_test \
	json_xml_test json_xml2_test \
	xml_test xml2_test xml3_test xml4_test xml5_test \
	xml_xml_test xml_xml2_test \
	$(PGSQL_TESTS) \
	mariadb_test mariadb2_test \
	mariadb_xml_test mariadb_xml2_test mariadb_xml3_test \
	mariadb_xml4_test \
	odbc_test odbc2_test odbc3_test odbc4_test \
	odbc5_test odbc6_test odbc7_test \
	odbc_xml_test odbc_xml2_test odbc_xml3_test \
	odbc_xml4_test \
	xls_test \
	follower_circular_test \
	follower_invalidref_test \
	follower_test1_test follower_test2_test \
	follower_test3_test follower_test4_test \
	follower_test5_test follower_test6_test \
	follower_test7_test follower_test8_test \
	follower_recursive_n_1_test \
	follower_recursive_n_1_v2_test \
	follower_recursive_n_1_v3_test \
	follower_recursive_n_1_match_single_test \
	follower_recursive_n_1_match_single_v2_test \
	follower_recursive_n_1_match_single_v3_test \
	rownum_test \
	part_test part_xml_test \
	locale_test \
	variable_expression_test variable_expression_xml_test \
	variable_check_test \
	variable_ignoreexpr_test \
	variable_ignoreexpr2_test \
	variable_ignoreexpr3_test \
	variable_ignoreexpr4_test \
	break_test \
	break2_test \
	break_xml_test \
	break_xml2_test \
	break_xml3_test \
	iterative_expr_test \
	break_rownum_xml_test \
	variable_xml_test \
	variable_resetonbreak_xml_test \
	result_copy_test \
	break_multi_test \
	break_multi2_test \
	break_multi3_test \
	break_multi4_test \
	break_multi5_test \
	datetime_expr_test \
	execute_test \
	break_multi6_test \
	break_multi7_test \
	break_multi8_test \
	custom_variable_test \
	custom_variable_xml_test \
	canonicalize_path_test \
	find_file_test \
	color_test \
	quoted_ident_test \
	r_lineno_test \
	r_value_test \
	r_self_crash_test

PHP_TESTS = \
	grammar_test expr_test function_test \
	function_bcmath_test \
	expr_format_test \
	environment_test environment2_test \
	array_test array2_test array_xml_test \
	csv_test csv2_test csv_array_test \
	csv_xml_test csv_array_xml_test csv_array_xml2_test \
	json_test json2_test json3_test json4_test json5_test \
	json6_test json7_test json8_test \
	json_xml_test json_xml2_test \
	xml_test xml2_test xml3_test xml4_test xml5_test \
	xml_xml_test xml_xml2_test \
	$(PGSQL_TESTS) \
	mariadb_test mariadb2_test \
	mariadb_xml_test mariadb_xml2_test mariadb_xml3_test \
	mariadb_xml4_test \
	odbc_test odbc2_test odbc3_test odbc4_test \
	odbc5_test odbc6_test odbc7_test \
	odbc_xml_test odbc_xml2_test odbc_xml3_test \
	odbc_xml4_test \
	rownum_test \
	part_test part_xml_test \
	variable_expression_test variable_expression_xml_test \
	break_test break_xml_test break_xml2_test \
	break_xml3_test \
	iterative_expr_test \
	break_rownum_xml_test \
	variable_xml_test \
	variable_resetonbreak_xml_test \
	result_copy_test \
	break_multi_test \
	break_multi2_test \
	break_multi3_test \
	break_multi4_test \
	break_multi5_test \
	datetime_expr_test \
	execute_test \
	break_multi6_test \
	break_multi7_test \
	break_multi8_test \
	custom_variable_test \
	custom_variable_xml_test \
	canonicalize_path_test \
	find_file_test \
	color_test \
	rlib_function_test

if ENABLE_PGSQL_TESTS

SLOW_TESTS = \
	pgsql_numeric_precision_test \
	pgsql_numeric_precision2_test

SLOW_PHP_TESTS = \
	pgsql_numeric_precision_test \
	pgsql_numeric_precision2_test

endif

# All PDF output tests are inherently unstable because Cairo:
# * sets the /Producer PDF property with its own name and version
# * different Cairo versions support different PDF versions
# So they need to be treated differently: execute_layout_test.sh
# converts the PDF pages to individual PNGs which are then
# compared with the ImageMagick "compare" utility. This slows down
# running these tests but makes them reliable across Cairo versions.
LAYOUT_TESTS = \
	layout_xml_test \
	layout_xml2_test \
	layout_xml3_test \
	layout_xml4_test \
	layout_xml4_headernewpage_test \
	layout_xml5_test \
	layout_xml5_headernewpage_test \
	layout_xml6_test \
	layout_xml7_test \
	layout_xml8_test \
	layout_xml9_test \
	layout_xml9_headernewpage_test \
	layout_xml10_test \
	layout_xml11_test \
	layout_xml12_test \
	layout_xml13_test \
	layout_xml14_test \
	layout_xml15_test \
	layout_xml16_test \
	layout_xml17_test \
	layout_xml18_test \
	layout_xml19_test \
	layout_xml20_test \
	layout_xml21_test \
	layout_xml22_test \
	layout_xml23_test \
	layout_xml24_test \
	layout_xml25_test \
	layout_xml26_test \
	layout_xml27_test \
	layout_rlib_fixed_part_test \
	layout_ocrpt_fixed_part_test \
	layout_xml28_test \
	layout_xml29_test \
	layout_xml30_test \
	layout_xml31_test \
	layout_xml32_test \
	layout_xml33_test \
	layout_xml34_test \
	layout_xml35_test \
	layout_xml36_test \
	layout_xml37_test \
	layout_xml38_test \
	layout_xml39_test \
	layout_xml40_test \
	layout_xml41_test \
	layout_code41_test \
	layout_xml42_test \
	layout_xml43_test \
	layout_variable_ignoreexpr_test \
	header_query_test \
	header_query2_test \
	page_header_footer_vars_test \
	break_suppressblank_test \
	layout_follower_test3_test \
	layout_follower_test5_test \
	layout_follower_test6_test \
	layout_follower_test32_test \
	layout_follower_test52_test \
	layout_follower_test62_test \
	layout_follower_test33_test \
	layout_follower_test53_test \
	layout_follower_test63_test \
	layout_follower_recursive_n_1_test \
	layout_follower_recursive_n_1_match_single_test \
	layout_barcode1_test \
	layout_barcode1_code_test \
	layout_barcode2_test \
	layout_barcode3_test \
	layout_barcode4_test \
	layout_xml3_ver2_test \
	layout_xml_mods_test

LAYOUT_CSV_TESTS = \
	layout_xml17_csv1_test \
	layout_xml17_csv2_test \
	layout_xml17_csv3_test

PHP_LAYOUT_TESTS = \
	layout_xml_test \
	layout_xml2_test \
	layout_xml3_test \
	layout_xml4_test \
	layout_xml5_test \
	layout_xml6_test \
	layout_xml7_test \
	layout_xml8_test \
	layout_xml9_test \
	layout_xml10_test \
	layout_xml11_test \
	layout_xml12_test \
	layout_xml13_test \
	layout_xml14_test \
	layout_xml15_test \
	layout_xml16_test \
	layout_xml17_test \
	layout_xml18_test \
	layout_xml19_test \
	layout_xml20_test \
	layout_xml21_test \
	layout_xml22_test \
	layout_xml23_test \
	layout_xml24_test \
	layout_xml25_test \
	layout_xml26_test \
	layout_xml27_test \
	layout_rlib_fixed_part_test \
	layout_rlib_fixed_part2_test \
	layout_ocrpt_fixed_part_test \
	layout_ocrpt_fixed_part2_test \
	layout_xml28_test \
	layout_xml29_test \
	layout_xml30_test \
	layout_xml31_test \
	layout_xml32_test \
	layout_xml33_test \
	layout_xml34_test \
	layout_xml35_test \
	layout_xml36_test \
	layout_xml37_test \
	layout_xml38_test \
	layout_xml39_test \
	layout_xml40_test \
	layout_xml41_test \
	layout_code41_test \
	layout_xml42_test \
	layout_barcode1_test \
	layout_barcode1_code_test \
	layout_xml_mods_test

if ENABLE_XLATE_TESTS

XLATE_DEPS = \
	locale/hu_HU/LC_MESSAGES/translate_test.mo

.po.mo:
	$(MSGFMT) -o $@ $(srcdir)/$<

LAYOUT_TESTS += \
	layout_translate1_test \
	layout_translate_xml1_test

PHP_LAYOUT_TESTS += \
	layout_translate1_test \
	layout_translate_xml1_test

endif

# Unstable tests by nature:
# random_test produces a random number which is different every time
# paper_test prints paper sizes and defaults which depends on language settings
UNSTABLE_TESTS = \
	random_test \
	paper_test \
	now_test

noinst_PROGRAMS = $(TESTS) $(SLOW_TESTS) $(LAYOUT_TESTS) $(LAYOUT_CSV_TESTS) $(UNSTABLE_TESTS)

$(noinst_PROGRAMS): $(top_builddir)/libsrc/libopencreport.la

all: $(noinst_PROGRAMS)

basic-test: $(TESTS) execute_test.sh
	$(foreach TEST,$(TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_test.sh $(TEST) &&) true

php-basic-test: execute_php_test.sh
	$(foreach TEST,$(PHP_TESTS), abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_php_test.sh $(TEST) &&) true

if ENABLE_PDF_TESTS

pdf-test: $(LAYOUT_TESTS) $(XLATE_DEPS) execute_layout_test.sh
	$(foreach TEST,$(LAYOUT_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_layout_test.sh $(TEST) 1 &&) true

php-pdf-test: $(XLATE_DEPS) execute_php_layout_test.sh
	$(foreach TEST,$(PHP_LAYOUT_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_php_layout_test.sh $(TEST) &&) true

else

.PHONY: pdf-test php-pdf-test

endif

html-test: $(LAYOUT_TESTS) $(XLATE_DEPS) execute_layout_test.sh
	$(foreach TEST,$(LAYOUT_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_layout_test.sh $(TEST) 2 &&) true

txt-test: $(LAYOUT_TESTS) $(XLATE_DEPS) execute_layout_test.sh
	$(foreach TEST,$(LAYOUT_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_layout_test.sh $(TEST) 3 &&) true

csv-test: $(LAYOUT_TESTS) $(LAYOUT_CSV_TESTS) $(XLATE_DEPS) execute_layout_test.sh
	$(foreach TEST,$(LAYOUT_TESTS) $(LAYOUT_CSV_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_layout_test.sh $(TEST) 4 &&) true

xml-test: $(LAYOUT_TESTS) $(XLATE_DEPS) execute_layout_test.sh
	$(foreach TEST,$(LAYOUT_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_layout_test.sh $(TEST) 5 &&) true

json-test: $(LAYOUT_TESTS) $(XLATE_DEPS) execute_layout_test.sh
	$(foreach TEST,$(LAYOUT_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_layout_test.sh $(TEST) 6 &&) true

test: basic-test pdf-test html-test txt-test csv-test xml-test json-test

php-test: php-basic-test php-pdf-test

if ENABLE_PGSQL_TESTS

PGSQL_SLOW_TEST = slow-test

slow-test: $(SLOW_TESTS) execute_test.sh
	$(foreach TEST,$(SLOW_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_test.sh $(TEST) &&) true

php-slow-test: execute_php_test.sh
	$(foreach TEST,$(SLOW_PHP_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_php_test.sh $(TEST) &&) true

endif

unstable-test: $(UNSTABLE_TESTS) execute_test.sh
	$(foreach TEST,$(UNSTABLE_TESTS),abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) top_srcdir=$(top_srcdir) ./execute_test.sh $(TEST) &&) true

all-test: test $(PGSQL_SLOW_TEST) unstable-test

CLEANFILES = results/* locale/*/*/*.mo
