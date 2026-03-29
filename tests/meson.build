## tests/meson.build for OpenCReports
##
## Mirrors tests/Makefile.am:
##   noinst_PROGRAMS = $(TESTS) $(SLOW_TESTS) $(LAYOUT_TESTS)
##                     $(LAYOUT_CSV_TESTS) $(UNSTABLE_TESTS)
##
## AM_CPPFLAGS = -Wall @WERRORFLAG@ -O2 -g -D_XOPEN_SOURCE=700 -I$(top_srcdir)/include
## AM_LDFLAGS  = @DYNAMIC_LINK_FLAG@
## LIBS        = libopencreport.la

test_c_args   = ['-Wall', '-O2', '-g', '-D_XOPEN_SOURCE=700'] + werror_args
test_link_args = [dynamic_link_flag]
test_deps      = [libmpfr, libgmp]

# Helper to build a non-installed test executable (C source)
# All tests share the same flags and link against libopencreport.
foreach name : [
  # -----------------------------------------------------------------
  # TESTS (always built)
  # -----------------------------------------------------------------
  'compiler_test',
  'grammar_test',
  'expr_test',
  'function_test',
  'expr_format_test',
  'environment_test',
  'environment2_test',
  'array_test',
  'array2_test',
  'array_resolve_test',
  'array_xml_test',
  'csv_test',
  'csv2_test',
  'csv_array_test',
  'csv_xml_test',
  'csv_array_xml_test',
  'csv_array_xml2_test',
  'json_test',
  'json2_test',
  'json3_test',
  'json4_test',
  'json5_test',
  'json6_test',
  'json7_test',
  'json8_test',
  'json_xml_test',
  'json_xml2_test',
  'xml_test',
  'xml2_test',
  'xml3_test',
  'xml4_test',
  'xml5_test',
  'xml_xml_test',
  'xml_xml2_test',
  # MariaDB tests are always built (unconditional in Makefile.am TESTS)
  'mariadb_test',
  'mariadb2_test',
  'mariadb_xml_test',
  'mariadb_xml2_test',
  'mariadb_xml3_test',
  'mariadb_xml4_test',
  # ODBC tests are always built
  'odbc_test',
  'odbc2_test',
  'odbc3_test',
  'odbc4_test',
  'odbc5_test',
  'odbc6_test',
  'odbc7_test',
  'odbc_xml_test',
  'odbc_xml2_test',
  'odbc_xml3_test',
  'odbc_xml4_test',
  # Follower tests
  'follower_circular_test',
  'follower_invalidref_test',
  'follower_test1_test',
  'follower_test2_test',
  'follower_test3_test',
  'follower_test4_test',
  'follower_test5_test',
  'follower_test6_test',
  'follower_test7_test',
  'follower_test8_test',
  'follower_recursive_n_1_test',
  'follower_recursive_n_1_v2_test',
  'follower_recursive_n_1_v3_test',
  'follower_recursive_n_1_match_single_test',
  'follower_recursive_n_1_match_single_v2_test',
  'follower_recursive_n_1_match_single_v3_test',
  # Remaining always-built tests
  'rownum_test',
  'part_test',
  'part_xml_test',
  'locale_test',
  'variable_expression_test',
  'variable_expression_xml_test',
  'variable_check_test',
  'variable_ignoreexpr_test',
  'variable_ignoreexpr2_test',
  'variable_ignoreexpr3_test',
  'variable_ignoreexpr4_test',
  'break_test',
  'break2_test',
  'break_xml_test',
  'break_xml2_test',
  'break_xml3_test',
  'iterative_expr_test',
  'break_rownum_xml_test',
  'variable_xml_test',
  'variable_resetonbreak_xml_test',
  'result_copy_test',
  'break_multi_test',
  'break_multi2_test',
  'break_multi3_test',
  'break_multi4_test',
  'break_multi5_test',
  'datetime_expr_test',
  'execute_test',
  'break_multi6_test',
  'break_multi7_test',
  'break_multi8_test',
  'custom_variable_test',
  'custom_variable_xml_test',
  'canonicalize_path_test',
  'find_file_test',
  'color_test',
  'quoted_ident_test',
  'r_lineno_test',
  'r_value_test',
  'r_self_crash_test',
  # -----------------------------------------------------------------
  # LAYOUT_TESTS (PDF/HTML/TXT/CSV/XML/JSON output layout tests)
  # -----------------------------------------------------------------
  'layout_xml_test',
  'layout_xml2_test',
  'layout_xml3_test',
  'layout_xml4_test',
  'layout_xml4_headernewpage_test',
  'layout_xml5_test',
  'layout_xml5_headernewpage_test',
  'layout_xml6_test',
  'layout_xml7_test',
  'layout_xml8_test',
  'layout_xml9_test',
  'layout_xml9_headernewpage_test',
  'layout_xml10_test',
  'layout_xml11_test',
  'layout_xml12_test',
  'layout_xml13_test',
  'layout_xml13_include_test',
  'layout_xml14_test',
  'layout_xml15_test',
  'layout_xml16_test',
  'layout_xml17_test',
  'layout_xml18_test',
  'layout_xml19_test',
  'layout_xml20_test',
  'layout_xml21_test',
  'layout_xml22_test',
  'layout_xml23_test',
  'layout_xml24_test',
  'layout_xml25_test',
  'layout_xml26_test',
  'layout_xml27_test',
  'layout_rlib_fixed_part_test',
  'layout_ocrpt_fixed_part_test',
  'layout_xml28_test',
  'layout_xml29_test',
  'layout_xml30_test',
  'layout_xml31_test',
  'layout_xml32_test',
  'layout_xml33_test',
  'layout_xml34_test',
  'layout_xml35_test',
  'layout_xml36_test',
  'layout_xml37_test',
  'layout_xml38_test',
  'layout_xml39_test',
  'layout_xml40_test',
  'layout_xml41_test',
  'layout_code41_test',
  'layout_xml42_test',
  'layout_xml43_test',
  'layout_variable_ignoreexpr_test',
  'header_query_test',
  'header_query2_test',
  'page_header_footer_vars_test',
  'break_suppressblank_test',
  'layout_follower_test3_test',
  'layout_follower_test5_test',
  'layout_follower_test6_test',
  'layout_follower_test32_test',
  'layout_follower_test52_test',
  'layout_follower_test62_test',
  'layout_follower_test33_test',
  'layout_follower_test53_test',
  'layout_follower_test63_test',
  'layout_follower_recursive_n_1_test',
  'layout_follower_recursive_n_1_match_single_test',
  'layout_barcode1_test',
  'layout_barcode1_code_test',
  'layout_barcode2_test',
  'layout_barcode3_test',
  'layout_barcode4_test',
  'layout_xml3_ver2_test',
  'layout_xml_mods_test',
  'layout_code42_test',
  'layout_code43_test',
  'layout_xml44_test',
  'layout_xml45_test',
  'layout_code46_test',
  'layout_code47_test',
  'layout_xml46_test',
  'layout_xml47_test',
  'layout_xml48_test',
  'layout_xml49_test',
  # -----------------------------------------------------------------
  # LAYOUT_CSV_TESTS
  # -----------------------------------------------------------------
  'layout_xml17_csv1_test',
  'layout_xml17_csv2_test',
  'layout_xml17_csv3_test',
  # -----------------------------------------------------------------
  # UNSTABLE_TESTS (random output – built but results differ each run)
  # -----------------------------------------------------------------
  'random_test',
  'paper_test',
  'now_test',
]
  executable(name,
    name + '.c',
    c_args: test_c_args,
    link_args: test_link_args,
    include_directories: [build_root_inc, inc_dir],
    dependencies: test_deps,
    link_with: libopencreport,
    install: false,
  )
endforeach

# -----------------------------------------------------------------
# compiler_cc_test  (C++ source – compiler_cc_test_SOURCES = compiler_cc_test.cc)
# -----------------------------------------------------------------
executable('compiler_cc_test',
  'compiler_cc_test.cc',
  link_args: test_link_args,
  include_directories: [build_root_inc, inc_dir],
  dependencies: test_deps,
  link_with: libopencreport,
  install: false,
)

# -----------------------------------------------------------------
# PGSQL_TESTS (conditional on found_pgsql)
# -----------------------------------------------------------------
if found_pgsql
  foreach name : [
    'pgsql_test',
    'pgsql2_test',
    'pgsql_xml_test',
    'pgsql_xml2_test',
    'pgsql_xml3_test',
    'pgsql_xml4_test',
    'pgsql_xml5_test',
    'pgsql_xml6_test',
    'pgsql_xml7_test',
    'pgsql_xml8_test',
    'pgsql_xml9_test',
    'pgsql_xml10_test',
    'pgsql_xml11_test',
  ]
    executable(name,
      name + '.c',
      c_args: test_c_args,
      link_args: test_link_args,
      include_directories: [build_root_inc, inc_dir],
      dependencies: test_deps,
      link_with: libopencreport,
      install: false,
    )
  endforeach

  # SLOW_TESTS (pgsql only)
  foreach name : [
    'pgsql_numeric_precision_test',
    'pgsql_numeric_precision2_test',
  ]
    executable(name,
      name + '.c',
      c_args: test_c_args,
      link_args: test_link_args,
      include_directories: [build_root_inc, inc_dir],
      dependencies: test_deps,
      link_with: libopencreport,
      install: false,
    )
  endforeach
endif

# -----------------------------------------------------------------
# PYTHON_TESTS and LAYOUT_PYTHON_TESTS (conditional on found_python)
# -----------------------------------------------------------------
if found_python
  foreach name : [
    'xls_test',
    'xls2_test',
    'xls3_test',
    'xls4_test',
    'layout_xls_test',
    'layout_xlsx_test',
    'layout_xlsx_mixed_test',
    'layout_ods_test',
  ]
    executable(name,
      name + '.c',
      c_args: test_c_args,
      link_args: test_link_args,
      include_directories: [build_root_inc, inc_dir],
      dependencies: test_deps,
      link_with: libopencreport,
      install: false,
    )
  endforeach
endif

# -----------------------------------------------------------------
# XLATE layout tests (conditional on msgfmt)
# -----------------------------------------------------------------
msgfmt = find_program('msgfmt', required: false)
if msgfmt.found()
  foreach name : [
    'layout_translate1_test',
    'layout_translate_xml1_test',
  ]
    executable(name,
      name + '.c',
      c_args: test_c_args,
      link_args: test_link_args,
      include_directories: [build_root_inc, inc_dir],
      dependencies: test_deps,
      link_with: libopencreport,
      install: false,
    )
  endforeach

  # Generate .mo files from .po sources (mirrors .po.mo rule)
  locale_mo = custom_target('locale_hu_HU_mo',
    input:   'locale/hu_HU/LC_MESSAGES/translate_test.po',
    output:  'translate_test.mo',
    command: [msgfmt, '-o', '@OUTPUT@', '@INPUT@'],
    install: false,
  )
endif
