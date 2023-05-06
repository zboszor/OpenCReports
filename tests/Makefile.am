AM_CPPFLAGS = \
	-Wall -Werror -O2 -g \
	-D_XOPEN_SOURCE=700 \
	-I$(top_srcdir)/include

AM_CFLAGS = -rdynamic

LIBS = $(top_builddir)/libsrc/libopencreport.la

$(top_builddir)/libsrc/libopencreport.la:
	$(MAKE) -C $(top_builddir)/libsrc libopencreport.la

TESTS = \
	compiler_test \
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
	pgsql_test pgsql2_test \
	pgsql_xml_test pgsql_xml2_test pgsql_xml3_test pgsql_xml4_test \
	pgsql_xml5_test pgsql_xml6_test pgsql_xml7_test pgsql_xml8_test \
	pgsql_xml9_test pgsql_xml10_test pgsql_xml11_test \
	mariadb_test mariadb2_test \
	mariadb_xml_test mariadb_xml2_test mariadb_xml3_test \
	mariadb_xml4_test \
	odbc_test odbc2_test odbc3_test odbc4_test \
	odbc5_test odbc6_test odbc7_test \
	odbc_xml_test odbc_xml2_test odbc_xml3_test \
	odbc_xml4_test \
	follower_circular_test \
	follower_invalidref_test \
	rownum_test \
	part_test part_xml_test \
	locale_test \
	variable_expression_test variable_expression_xml_test \
	variable_check_test \
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
	quoted_ident_test

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
	pgsql_test pgsql2_test \
	pgsql_xml_test pgsql_xml2_test pgsql_xml3_test pgsql_xml4_test \
	pgsql_xml5_test pgsql_xml6_test pgsql_xml7_test pgsql_xml8_test \
	pgsql_xml9_test pgsql_xml10_test pgsql_xml11_test \
	mariadb_test mariadb2_test \
	mariadb_xml_test mariadb_xml2_test mariadb_xml3_test \
	mariadb_xml4_test \
	odbc_test odbc2_test odbc3_test odbc4_test \
	odbc5_test odbc6_test odbc7_test \
	odbc_xml_test odbc_xml2_test odbc_xml3_test \
	odbc_xml4_test \
	follower_circular_test \
	follower_invalidref_test \
	rownum_test \
	part_test part_xml_test \
	variable_expression_test variable_expression_xml_test \
	variable_check_test \
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

SLOW_TESTS = \
	pgsql_numeric_precision_test \
	pgsql_numeric_precision2_test

SLOW_PHP_TESTS = \
	pgsql_numeric_precision_test \
	pgsql_numeric_precision2_test

# All PDF output tests are inherently unstable because Cairo:
# * sets the /Producer PDF property with its own name and version
# * different Cairo versions support different PDF versions
# So they need to be treated differently: execute_pdf_test.sh
# converts the PDF pages to individual PNGs which are then
# compared with the ImageMagick "compare" utility. This slows down
# running these tests but makes them reliable across Cairo versions.
PDF_TESTS = \
	layout_pdf_xml_test \
	layout_pdf_xml2_test \
	layout_pdf_xml3_test \
	layout_pdf_xml4_test \
	layout_pdf_xml4_headernewpage_test \
	layout_pdf_xml5_test \
	layout_pdf_xml5_headernewpage_test \
	layout_pdf_xml6_test \
	layout_pdf_xml7_test \
	layout_pdf_xml8_test \
	layout_pdf_xml9_test \
	layout_pdf_xml9_headernewpage_test \
	layout_pdf_xml10_test \
	layout_pdf_xml11_test \
	layout_pdf_xml12_test \
	layout_pdf_xml13_test \
	layout_pdf_xml14_test \
	layout_pdf_xml15_test \
	layout_pdf_xml16_test \
	layout_pdf_xml17_test \
	layout_pdf_xml18_test \
	layout_pdf_xml19_test \
	layout_pdf_xml20_test \
	layout_pdf_xml21_test \
	layout_pdf_xml22_test \
	layout_pdf_xml23_test \
	layout_pdf_xml24_test \
	layout_pdf_xml25_test \
	layout_pdf_xml26_test \
	layout_pdf_xml27_test \
	layout_rlib_fixed_part_test \
	layout_ocrpt_fixed_part_test \
	layout_pdf_xml28_test \
	layout_pdf_xml29_test \
	layout_pdf_xml30_test \
	layout_pdf_xml31_test \
	layout_pdf_xml32_test \
	layout_pdf_xml33_test \
	layout_pdf_xml34_test \
	layout_pdf_xml35_test \
	layout_pdf_xml36_test \
	layout_pdf_xml37_test \
	layout_pdf_xml38_test \
	layout_pdf_xml39_test \
	layout_pdf_xml40_test \
	layout_pdf_xml41_test \
	layout_pdf41_test \
	layout_pdf_xml42_test \
	header_query_test \
	header_query2_test \
	page_header_footer_vars_test \
	break_suppressblank_test

PHP_PDF_TESTS = \
	layout_pdf_xml_test \
	layout_pdf_xml2_test \
	layout_pdf_xml3_test \
	layout_pdf_xml4_test \
	layout_pdf_xml5_test \
	layout_pdf_xml6_test \
	layout_pdf_xml7_test \
	layout_pdf_xml8_test \
	layout_pdf_xml9_test \
	layout_pdf_xml10_test \
	layout_pdf_xml11_test \
	layout_pdf_xml12_test \
	layout_pdf_xml13_test \
	layout_pdf_xml14_test \
	layout_pdf_xml15_test \
	layout_pdf_xml16_test \
	layout_pdf_xml17_test \
	layout_pdf_xml18_test \
	layout_pdf_xml19_test \
	layout_pdf_xml20_test \
	layout_pdf_xml21_test \
	layout_pdf_xml22_test \
	layout_pdf_xml23_test \
	layout_pdf_xml24_test \
	layout_pdf_xml25_test \
	layout_pdf_xml26_test \
	layout_pdf_xml27_test \
	layout_rlib_fixed_part_test \
	layout_rlib_fixed_part2_test \
	layout_ocrpt_fixed_part_test \
	layout_ocrpt_fixed_part2_test \
	layout_pdf_xml28_test \
	layout_pdf_xml29_test \
	layout_pdf_xml30_test \
	layout_pdf_xml31_test \
	layout_pdf_xml32_test \
	layout_pdf_xml33_test \
	layout_pdf_xml34_test \
	layout_pdf_xml35_test \
	layout_pdf_xml36_test \
	layout_pdf_xml37_test \
	layout_pdf_xml38_test \
	layout_pdf_xml39_test \
	layout_pdf_xml40_test \
	layout_pdf41_test \
	layout_pdf_xml41_test \
	layout_pdf_xml42_test

if ENABLE_XLATE_TESTS

XLATE_DEPS = \
	locale/hu_HU/LC_MESSAGES/translate_test.mo

.po.mo:
	$(MSGFMT) -o $@ $(srcdir)/$<

PDF_TESTS += \
	layout_pdf_translate1_test \
	layout_pdf_translate_xml1_test

PHP_PDF_TESTS += \
	layout_pdf_translate1_test \
	layout_pdf_translate_xml1_test

endif

# Unstable tests by nature:
# random_test produces a random number which is different every time
# paper_test prints paper sizes and defaults which depends on language settings
UNSTABLE_TESTS = \
	random_test \
	paper_test \
	now_test

noinst_PROGRAMS = $(TESTS) $(SLOW_TESTS) $(PDF_TESTS) $(UNSTABLE_TESTS)

$(noinst_PROGRAMS): $(top_builddir)/libsrc/libopencreport.la

all: $(noinst_PROGRAMS)

basic-test: $(TESTS) execute_test.sh
	$(foreach TEST,$(TESTS),ASAN_OPTIONS="log_path=results/$(TEST).asanout,fast_unwind_on_malloc=0" UBSAN_OPTIONS=print_stacktrace=true LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_test.sh $(TEST) &&) true

php-basic-test: execute_php_test.sh
	$(foreach TEST,$(PHP_TESTS),ASAN_OPTIONS="log_path=results/$(TEST).asanout,fast_unwind_on_malloc=0" UBSAN_OPTIONS=print_stacktrace=true LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_php_test.sh $(TEST) &&) true

if ENABLE_PDF_TESTS

pdf-test: $(PDF_TESTS) $(XLATE_DEPS) execute_pdf_test.sh
	$(foreach TEST,$(PDF_TESTS),ASAN_OPTIONS="log_path=results/$(TEST).asanout,fast_unwind_on_malloc=0" UBSAN_OPTIONS=print_stacktrace=true LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_pdf_test.sh $(TEST) &&) true

php-pdf-test: $(XLATE_DEPS) execute_php_pdf_test.sh
	$(foreach TEST,$(PHP_PDF_TESTS),ASAN_OPTIONS="log_path=results/$(TEST).asanout,fast_unwind_on_malloc=0" UBSAN_OPTIONS=print_stacktrace=true LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_php_pdf_test.sh $(TEST) &&) true

else

.PHONY: pdf-test php-pdf-test

endif

test: basic-test pdf-test

php-test: php-basic-test php-pdf-test

slow-test: $(SLOW_TESTS) execute_test.sh
	$(foreach TEST,$(SLOW_TESTS),ASAN_OPTIONS="log_path=results/$(TEST).asanout,fast_unwind_on_malloc=0" UBSAN_OPTIONS=print_stacktrace=true LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_test.sh $(TEST) &&) true

php-slow-test: execute_php_test.sh
	$(foreach TEST,$(SLOW_PHP_TESTS),ASAN_OPTIONS="log_path=results/$(TEST).asanout,fast_unwind_on_malloc=0" UBSAN_OPTIONS=print_stacktrace=true LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_php_test.sh $(TEST) &&) true

unstable-test: $(UNSTABLE_TESTS) execute_test.sh
	$(foreach TEST,$(UNSTABLE_TESTS),ASAN_OPTIONS="log_path=results/$(TEST).asanout,fast_unwind_on_malloc=0" UBSAN_OPTIONS=print_stacktrace=true LSAN_OPTIONS=suppressions=$(top_srcdir)/fontconfig.supp abs_builddir=$(abs_builddir) abs_srcdir=$(abs_srcdir) ./execute_test.sh $(TEST) &&) true

all-test: test slow-test unstable-test

CLEANFILES = results/* locale/*/*/*.mo
