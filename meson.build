## meson.build for OpenCReports
##
## Copyright (c) 2019-2026 Zoltán Böszörményi (zboszor@gmail.com)
## OpenCReports is free software; you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation; either version 3, or (at your option)
## any later version.

project('OpenCReports', ['c', 'cpp'],
  version: run_command('cat', 'VERSION', check: true).stdout().strip(),
  license: 'LGPL-3.0-or-later',
  meson_version: '>= 0.60.0',
)

cc = meson.get_compiler('c')

# Library versioning matching libtool version-info 1:0:0
# soversion = CURRENT - AGE = 1 - 0 = 1
# library version string = (CURRENT-AGE).AGE.REVISION = 1.0.0
lib_soversion = 1
lib_version = '1.0.0'

conf = configuration_data()

conf.set_quoted('PACKAGE',         'opencreports')
conf.set_quoted('PACKAGE_NAME',    'OpenCReports')
conf.set_quoted('PACKAGE_TARNAME', 'opencreports')
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_STRING',  'OpenCReports ' + meson.project_version())
conf.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/zboszor/OpenCReports')
conf.set_quoted('PACKAGE_URL',     'https://github.com/zboszor/OpenCReports')
conf.set_quoted('VERSION',         meson.project_version())

# Check for standard functions (AC_CHECK_FUNCS)
foreach func : ['localtime_r', 'strfmon', 'getrandom']
  if cc.has_function(func)
    conf.set('HAVE_' + func.to_upper(), 1)
  endif
endforeach

# Check for standard headers (mirrors autoconf AC_CHECK_HEADERS)
foreach hdr : [
  'dlfcn.h', 'inttypes.h', 'stdint.h', 'stdio.h', 'stdlib.h',
  'strings.h', 'string.h', 'sys/stat.h', 'sys/types.h', 'unistd.h',
]
  if cc.has_header(hdr)
    conf.set('HAVE_' + hdr.replace('/', '_').replace('.', '_').to_upper(), 1)
  endif
endforeach
conf.set('STDC_HEADERS', 1)

# --- COMPILE_SYMBOL detection (strcasecmp) ---
# Priority: _BSD_SOURCE > _DEFAULT_SOURCE > _GNU_SOURCE
compile_symbol_args = []

_bsd_strcasecmp = cc.compiles(
  '#include <string.h>\nint main(void) { return strcasecmp("a", "a"); }',
  args: ['-Werror', '-D_BSD_SOURCE'],
  name: '-D_BSD_SOURCE provides strcasecmp',
)
_default_strcasecmp = cc.compiles(
  '#include <string.h>\nint main(void) { return strcasecmp("a", "a"); }',
  args: ['-Werror', '-D_DEFAULT_SOURCE'],
  name: '-D_DEFAULT_SOURCE provides strcasecmp',
)
_gnu_strcasecmp = cc.compiles(
  '#include <string.h>\nint main(void) { return strcasecmp("a", "a"); }',
  args: ['-Werror', '-D_GNU_SOURCE'],
  name: '-D_GNU_SOURCE provides strcasecmp',
)

if _bsd_strcasecmp
  compile_symbol_args += '-D_BSD_SOURCE'
elif _default_strcasecmp
  compile_symbol_args += '-D_DEFAULT_SOURCE'
elif _gnu_strcasecmp
  compile_symbol_args += '-D_GNU_SOURCE'
else
  warning('Using strcasecmp may fail to compile!')
endif

# --- reallocarray detection ---
_default_reallocarray = cc.compiles(
  '#include <stdlib.h>\nint main(void) { void *x = reallocarray(NULL, 2, 16); return x != NULL; }',
  args: ['-Werror', '-D_DEFAULT_SOURCE'],
  name: '-D_DEFAULT_SOURCE provides reallocarray',
)
_gnu_reallocarray = cc.compiles(
  '#include <stdlib.h>\nint main(void) { void *x = reallocarray(NULL, 2, 16); return x != NULL; }',
  args: ['-Werror', '-D_GNU_SOURCE'],
  name: '-D_GNU_SOURCE provides reallocarray',
)

if _default_reallocarray
  if '-D_DEFAULT_SOURCE' not in compile_symbol_args
    compile_symbol_args += '-D_DEFAULT_SOURCE'
  endif
  conf.set('HAVE_REALLOCARRAY', 1)
elif _gnu_reallocarray
  if '-D_GNU_SOURCE' not in compile_symbol_args
    compile_symbol_args += '-D_GNU_SOURCE'
  endif
  conf.set('HAVE_REALLOCARRAY', 1)
else
  warning('Using reallocarray may fail to compile!')
endif

# --- Symbol visibility (gl_VISIBILITY) ---
visibility_args = []
if cc.has_argument('-fvisibility=hidden')
  _vis_test = cc.compiles(
    '__attribute__((visibility("default"))) int foo(void) { return 0; }',
    args: '-fvisibility=hidden',
    name: 'compiler supports __attribute__((visibility))',
  )
  if _vis_test
    visibility_args = ['-fvisibility=hidden']
    conf.set('HAVE_VISIBILITY', 1)
    conf.set('DLL_EXPORT_SYM', '__attribute__((visibility("default")))')
  else
    conf.set('DLL_EXPORT_SYM', '')
  endif
else
  conf.set('DLL_EXPORT_SYM', '')
endif

# --- Dynamic link flag detection ---
dynamic_link_flag = ''
foreach candidate : ['-Wl,--export-dynamic', '-Wl,-export-dynamic', '-rdynamic']
  if dynamic_link_flag == '' and cc.has_link_argument(candidate)
    dynamic_link_flag = candidate
  endif
endforeach
if dynamic_link_flag == ''
  error('Working dynamic linker option not found')
endif

# -Werror unless maintainer_mode is enabled
if get_option('maintainer_mode')
  werror_args = []
else
  werror_args = cc.get_supported_arguments('-Werror')
endif

# Common compile flags (mirrors Makefile.am AM_CFLAGS / libopencreport_la_CFLAGS)
common_c_args = ['-Wall', '-O2', '-g', '-D_XOPEN_SOURCE=700'] \
  + compile_symbol_args \
  + visibility_args

# Include directories
inc_dir = include_directories('include')
# build_root_inc provides access to the generated config.h
build_root_inc = include_directories('.')

# --- Required dependencies ---

# libxml2 >= 2.5.0
libxml2 = dependency('libxml-2.0', version: '>= 2.5.0')

# libutf8proc (pkg-config or direct)
libutf8proc = dependency('libutf8proc', required: false)
if not libutf8proc.found()
  if not cc.has_header('utf8proc.h')
    error('utf8proc is not installed')
  endif
  libutf8proc = cc.find_library('utf8proc', required: true)
endif

# GNU MP and MPFR (AC_CHECK_HEADERS + AC_CHECK_LIB)
if not cc.has_header('gmp.h')
  error('GNU MP headers not found!')
endif
conf.set('HAVE_GMP_H', 1)

if not cc.has_header('mpfr.h')
  error('GNU MPFR headers not found!')
endif
conf.set('HAVE_MPFR_H', 1)

if cc.has_header('mpf2mpfr.h')
  conf.set('HAVE_MPF2MPFR_H', 1)
endif

libmpfr = cc.find_library('mpfr', required: true)
libgmp  = cc.find_library('gmp',  required: true)

# libpaper
if not cc.has_header('paper.h')
  error('libpaper header is not found')
endif
conf.set('HAVE_PAPER_H', 1)
libpaper = cc.find_library('paper', required: true)

# libcsv (must be >= 3.0.0, detected via CSV_APPEND_NULL symbol)
if not cc.has_header('csv.h')
  error('libcsv headers not found!')
endif
conf.set('HAVE_CSV_H', 1)
libcsv = cc.find_library('csv', required: true)
if not cc.compiles(
  '#include <csv.h>\nint i = CSV_APPEND_NULL;',
  name: 'libcsv >= 3.0.0 (CSV_APPEND_NULL)',
)
  error('libcsv 3.0.0 or newer is required!')
endif

# YAJL
libyajl = dependency('yajl')

# Cairo / PDF / Pango / RSVG / GDK-Pixbuf (PDFGEN group)
pdfgen_deps = [
  dependency('cairo',       version: '>= 1.6'),
  dependency('cairo-pdf'),
  dependency('pangocairo',  version: '>= 1.10'),
  dependency('librsvg-2.0'),
  dependency('gdk-pixbuf-2.0'),
]

# libm
libm = cc.find_library('m', required: true)

# --- Optional: PostgreSQL ---
pgsql_dep = dependency('libpq', required: false)
found_pgsql = pgsql_dep.found()
if not found_pgsql
  if cc.has_header('libpq-fe.h')
    conf.set('HAVE_LIBPQ_FE_H', 1)
    _libpq = cc.find_library('pq', required: false)
    if _libpq.found()
      pgsql_dep  = _libpq
      found_pgsql = true
      conf.set('HAVE_LIBPQ', 1)
    endif
  endif
endif

if found_pgsql
  conf.set('HAVE_POSTGRESQL', 1)

  _opt_old_pgsql = get_option('old_pgsql')
  if _opt_old_pgsql
    conf.set('USE_PQEXEC',      1)
    conf.set('USE_PQCONNECTDB', 1)
  else
    if not cc.has_function('PQdescribePortal', dependencies: pgsql_dep)
      conf.set('USE_PQEXEC', 1)
    endif
    if not cc.has_function('PQconnectdbParams', dependencies: pgsql_dep)
      conf.set('USE_PQCONNECTDB', 1)
    endif
  endif
endif

if get_option('pgsql_cursor')
  conf.set('USE_PGSQL_CURSOR', 1)
endif

# --- Optional: MySQL / MariaDB ---
mysql_dep   = []
found_mysql = false
mysql_config = find_program('mysql_config', required: false)
if mysql_config.found()
  _mysql_cflags = run_command(mysql_config, '--cflags',  check: false)
  _mysql_libs   = run_command(mysql_config, '--libs_r',  check: false)
  if _mysql_libs.returncode() == 0 and _mysql_libs.stdout().strip() != ''
    mysql_dep = declare_dependency(
      compile_args: _mysql_cflags.stdout().strip().split(),
      link_args:    _mysql_libs.stdout().strip().split(),
    )
    conf.set('HAVE_MYSQL', 1)
    found_mysql = true
  endif
endif

# --- Optional: ODBC (unixODBC or iODBC) ---
odbc_dep    = []
found_odbc  = false
odbcmanager = get_option('odbcmanager')

if odbcmanager == 'unixodbc' or odbcmanager == 'auto'
  _odbc_pkg = dependency('odbc', required: false)
  if _odbc_pkg.found()
    odbc_dep    = _odbc_pkg
    found_odbc  = true
    odbcmanager = 'unixodbc'
    conf.set('HAVE_ODBC', 1)
  else
    _odbc_lib = cc.find_library('odbc', required: false)
    if _odbc_lib.found() \
        and cc.has_header('sql.h') \
        and cc.has_header('sqlext.h') \
        and cc.has_header('sqltypes.h')
      odbc_dep    = _odbc_lib
      found_odbc  = true
      odbcmanager = 'unixodbc'
      conf.set('HAVE_ODBC',     1)
      conf.set('HAVE_SQL_H',    1)
      conf.set('HAVE_SQLEXT_H', 1)
      conf.set('HAVE_SQLTYPES_H', 1)
    elif get_option('odbcmanager') == 'unixodbc'
      error('unixODBC was requested but not found')
    endif
  endif
endif

if (odbcmanager == 'iodbc' or get_option('odbcmanager') == 'auto') and not found_odbc
  _iodbc_pkg = dependency('libiodbc', required: false)
  if _iodbc_pkg.found()
    odbc_dep    = _iodbc_pkg
    found_odbc  = true
    odbcmanager = 'iodbc'
    conf.set('HAVE_ODBC', 1)
  else
    _iodbc_lib = cc.find_library('iodbc', required: false)
    if _iodbc_lib.found() \
        and cc.has_header('sql.h') \
        and cc.has_header('sqlext.h') \
        and cc.has_header('sqltypes.h')
      odbc_dep    = _iodbc_lib
      found_odbc  = true
      odbcmanager = 'iodbc'
      conf.set('HAVE_ODBC',     1)
      conf.set('HAVE_SQL_H',    1)
      conf.set('HAVE_SQLEXT_H', 1)
      conf.set('HAVE_SQLTYPES_H', 1)
    elif get_option('odbcmanager') == 'iodbc'
      error('iODBC was requested but not found')
    endif
  endif
endif

# --- Optional: Python / Pandas ---
python_dep   = []
found_python = false
if get_option('pandas_source')
  _python_pkg = dependency('python3-embed', required: false)
  if _python_pkg.found()
    python_dep   = _python_pkg
    found_python = true
    conf.set('HAVE_LIBPYTHON', 1)
  endif
endif

# --- Generate config.h ---
configure_file(
  output: 'config.h',
  configuration: conf,
)

# --- Build subdirectories ---
subdir('include')
subdir('libsrc')

# --- pkg-config file (opencreports.pc) ---
# Matches opencreports.pc.in:
#   Libs:   -L${libdir} -lopencreport
#   Cflags: -I${includedir} @DYNAMIC_LINK_FLAG@
pkg = import('pkgconfig')
pkg.generate(libopencreport,
  name:        'opencreports',
  description: 'OpenCReports library',
  version:     meson.project_version(),
  filebase:    'opencreports',
  subdirs:     ['.'],
  extra_cflags: [dynamic_link_flag],
)

if get_option('enable_tests')
  subdir('tests')
endif

if get_option('enable_documentation')
  subdir('doc')
endif

# --- Summary ---
summary({
  'PostgreSQL':                 found_pgsql,
  'MySQL/MariaDB':              found_mysql,
  'ODBC':                       found_odbc ? odbcmanager : false,
  'Python / Pandas (XLS/XLSX/ODS)': found_python,
  'Memory arrays':              true,
  'CSV':                        true,
  'XML':                        true,
  'JSON':                       true,
}, section: 'Input drivers')

summary({
  'PDF':  true,
  'HTML': true,
  'TXT':  true,
  'CSV':  true,
  'XML':  true,
  'JSON': true,
}, section: 'Output formats')
