## libsrc/meson.build for OpenCReports
##
## Builds:
##   libopencreport_grammar  (static, internal – Bison/Flex generated parser)
##   libopencreport          (shared, installed)
##   cairotest               (executable, not installed)

bison = find_program('bison', required: true)
flex  = find_program('flex',  required: true)

# ---------------------------------------------------------------------------
# Code generation
# ---------------------------------------------------------------------------

# Bison grammar → ocrptgrammar.tab.c + ocrptgrammar.tab.h
# Matches: bison -d -Wprecedence ocrptgrammar.y
grammar_gen = custom_target('grammar_gen',
  input:   files('ocrptgrammar.y'),
  output:  ['ocrptgrammar.tab.c', 'ocrptgrammar.tab.h'],
  command: [
    bison, '-d', '-Wprecedence',
    '-o', '@OUTPUT0@',
    '--defines=@OUTPUT1@',
    '@INPUT@',
  ],
)

grammar_c = grammar_gen[0]
grammar_h = grammar_gen[1]

# Flex lexer → ocrptpatterns.yy.c
# Matches: $(LEX) -o ocrptpatterns.yy.c ocrptpatterns.l
# (depends on grammar_h to replicate Makefile.am ordering)
lexer_gen = custom_target('lexer_gen',
  input:   files('ocrptpatterns.l'),
  output:  'ocrptpatterns.yy.c',
  command: [flex, '-o', '@OUTPUT@', '@INPUT@'],
  depends: grammar_gen,
)

# ---------------------------------------------------------------------------
# Internal static library: libopencreport_grammar
#
# libopencreport_grammar_la_CFLAGS =
#   -Wall -O2 -g -I$(top_srcdir)/include
#   -D_XOPEN_SOURCE=700 @COMPILE_SYMBOL@
#   $(CFLAG_VISIBILITY) $(PDFGEN_CFLAGS)
# libopencreport_grammar_la_LDFLAGS = -static
# ---------------------------------------------------------------------------
lib_grammar = static_library('opencreport_grammar',
  sources: [grammar_c, grammar_h, lexer_gen],
  c_args: ['-Wall', '-O2', '-g', '-D_XOPEN_SOURCE=700']
    + compile_symbol_args
    + visibility_args,
  include_directories: [build_root_inc, inc_dir],
  dependencies: pdfgen_deps,
  # pic: true is required so the static lib can be linked into the shared lib
  pic: true,
  install: false,
)

# ---------------------------------------------------------------------------
# Shared library: libopencreport
#
# libopencreport_la_CFLAGS =
#   -Wall @WERRORFLAG@ -O2 -g -I$(top_srcdir)/include
#   -D_XOPEN_SOURCE=700 @COMPILE_SYMBOL@
#   $(CFLAG_VISIBILITY)
#   $(MPFR_CFLAGS) $(PAPER_CFLAGS) $(UTF8PROC_CFLAGS)
#   $(LIBXML_CFLAGS) $(YAJL_CFLAGS) $(POSTGRESQL_CFLAGS)
#   $(MYSQL_CLIENT_CFLAGS) $(ODBC_CFLAGS) $(PYTHON_CFLAGS)
#   $(PDFGEN_CFLAGS)
#
# libopencreport_la_LIBADD =
#   -lm
#   libopencreport_grammar.la
#   $(MPFR_LIBS) $(PAPER_LIBS) $(UTF8PROC_LIBS) $(LIBXML_LIBS)
#   $(CSV_LIBS) $(YAJL_LIBS) $(POSTGRESQL_LIBS) $(MYSQL_CLIENT_LIBS)
#   $(ODBC_LIBS) $(PYTHON_LIBS) $(PDFGEN_LIBS)
#
# libopencreport_la_LDFLAGS =
#   -version-info 1:0:0   →  soversion=1, version=1.0.0
# ---------------------------------------------------------------------------
libopencreport = shared_library('opencreport',
  sources: [
    'memutil.c', 'listutil.c', 'exprutil.c', 'functions.c',
    'api.c', 'free.c', 'parsexml.c', 'environment.c',
    'datasource.c', 'array-source.c', 'db-source.c', 'pandas-source.c',
    'navigation.c', 'breaks.c', 'parts.c', 'variables.c', 'strfmon.c',
    'datetime.c', 'formatting.c', 'layout.c', 'color.c', 'barcode.c',
    'common-output.c', 'pdf-output.c', 'html-output.c', 'txt-output.c',
    'xml-output.c', 'csv-output.c', 'json-output.c',
  ],
  c_args: common_c_args + werror_args,
  include_directories: [build_root_inc, inc_dir],
  dependencies: [
    libm,
    libmpfr, libgmp,
    libpaper,
    libutf8proc,
    libxml2,
    libcsv,
    libyajl,
    pdfgen_deps,
    pgsql_dep,
    mysql_dep,
    odbc_dep,
    python_dep,
  ],
  link_with: lib_grammar,
  version:   lib_version,
  soversion: lib_soversion,
  install: true,
)

# ---------------------------------------------------------------------------
# Non-installed test program: cairotest
#
# noinst_PROGRAMS = cairotest
# cairotest_CFLAGS = $(PDFGEN_CFLAGS)
# cairotest_LDADD  = $(PDFGEN_LIBS) libopencreport.la
# ---------------------------------------------------------------------------
cairotest = executable('cairotest',
  'cairotest.c',
  include_directories: [build_root_inc, inc_dir],
  dependencies: pdfgen_deps,
  link_with: libopencreport,
  install: false,
)
