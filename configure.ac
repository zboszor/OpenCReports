dnl Configure template for OpenCReports

dnl Copyright (c) 2019-2025 Zoltán Böszörményi (zboszor@gmail.com)
dnl OpenCReports is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU Lesser General Public License as published by
dnl the Free Software Foundation; either version 3, or (at your option)
dnl any later version.

dnl OpenCReports is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU Lesser General Public License for more details.

AC_INIT([OpenCReports],m4_normalize(m4_include([VERSION])),[https://github.com/zboszor/OpenCReports])

AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR(include/opencreport.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE()
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE

OCRPT_LT_CURRENT=1
OCRPT_LT_AGE=0
OCRPT_LT_REVISION=0
AC_SUBST(OCRPT_LT_CURRENT)
AC_SUBST(OCRPT_LT_REVISION)
AC_SUBST(OCRPT_LT_AGE)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LEX([noyywrap])
AX_PROG_FLEX
AX_PROG_BISON
LT_INIT([disable-static win32-dll])

AC_CHECK_FUNCS([localtime_r strfmon getrandom])

dnl Detect whether -D_BSD_SOURCE works
AC_MSG_CHECKING([whether -D_BSD_SOURCE provides strcasecmp])
old_CFLAGS="$CFLAGS"
CFLAGS="-Werror -D_BSD_SOURCE $CFLAGS"
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#include <string.h>]],[[return strcasecmp("a", "a");]])],
	[_bsd_source_strcasecmp=yes],
	[_bsd_source_strcasecmp=no])
CFLAGS="$old_CFLAGS"
AC_MSG_RESULT([$_bsd_source_strcasecmp])

AC_MSG_CHECKING([whether -D_DEFAULT_SOURCE provides strcasecmp])
old_CFLAGS="$CFLAGS"
CFLAGS="-Werror -D_DEFAULT_SOURCE $CFLAGS"
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#include <string.h>]],[[return strcasecmp("a", "a");]])],
	[_default_source_strcasecmp=yes],
	[_default_source_strcasecmp=no])
CFLAGS="$old_CFLAGS"
AC_MSG_RESULT([$_default_source_strcasecmp])

AC_MSG_CHECKING([whether -D_GNU_SOURCE provides strcasecmp])
old_CFLAGS="$CFLAGS"
CFLAGS="-Werror -D_GNU_SOURCE $CFLAGS"
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#include <string.h>]],[[return strcasecmp("a", "a");]])],
	[_gnu_source_strcasecmp=yes],
	[_gnu_source_strcasecmp=no])
CFLAGS="$old_CFLAGS"
AC_MSG_RESULT([$_gnu_source_strcasecmp])

AS_IF(
	[test x$_bsd_source_strcasecmp = xyes],
	[COMPILE_SYMBOL=-D_BSD_SOURCE],
	[test x$_default_source_strcasecmp = xyes],
	[COMPILE_SYMBOL=-D_DEFAULT_SOURCE],
	[test x$_gnu_source_strcasecmp = xyes],
	[COMPILE_SYMBOL=-D_GNU_SOURCE],
	[AC_MSG_WARN([Using strcasecmp may fail to compile!])])

AC_MSG_CHECKING([whether -D_DEFAULT_SOURCE provides reallocarray])
old_CFLAGS="$CFLAGS"
CFLAGS="-Werror -D_DEFAULT_SOURCE $CFLAGS"
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdlib.h>]],
		[[void *x = reallocarray(NULL, 2, 16); return x != NULL;]])],
	[_default_source_reallocarray=yes],
	[_default_source_reallocarray=no])
CFLAGS="$old_CFLAGS"
AC_MSG_RESULT([$_default_source_reallocarray])

AC_MSG_CHECKING([whether -D_GNU_SOURCE provides reallocarray])
old_CFLAGS="$CFLAGS"
CFLAGS="-Werror -D_GNU_SOURCE $CFLAGS"
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdlib.h>]],
		[[void *x = reallocarray(NULL, 2, 16); return x != NULL;]])],
	[_gnu_source_reallocarray=yes],
	[_gnu_source_reallocarray=no])
CFLAGS="$old_CFLAGS"
AC_MSG_RESULT([$_gnu_source_reallocarray])

AS_IF(
	[test x$_default_source_reallocarray = xyes],
	[COMPILE_SYMBOL="$COMPILE_SYMBOL -D_DEFAULT_SOURCE"
	 AC_DEFINE(HAVE_REALLOCARRAY,[1],[Have reallocarray function])],
	[test x$_gnu_source_reallocarray = xyes],
	[COMPILE_SYMBOL="$COMPILE_SYMBOL -D_GNU_SOURCE"
	 AC_DEFINE(HAVE_REALLOCARRAY,[1],[Have reallocarray function])],
	[AC_MSG_WARN([Using reallocarray may fail to compile!])])

AC_SUBST(COMPILE_SYMBOL)

old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wno-c23-extensions"
AC_LIBTOOL_LINKER_OPTION([for linker support for -Wl,--export-dynamic],[ac_cv_Wl_dash_dash_export_dynamic],[-Wl,--export-dynamic],
	[DYNAMIC_LINK_FLAG="-Wl,--export-dynamic"],
	[AC_LIBTOOL_LINKER_OPTION([for linker support for -Wl,-export-dynamic],[ac_cv_Wl_dash_export_dynamic],[-Wl,-export-dynamic],
		[DYNAMIC_LINK_FLAG="-Wl,-export-dynamic"],
		[AC_LIBTOOL_LINKER_OPTION([for linker support for -rdynamic],[ac_cv_WL_rdynamic],[-rdynamic],
			[DYNAMIC_LINK_FLAG="-rdynamic"],
			[AC_MSG_ERROR([Working dynamic linker option not found])])])])
CFLAGS="$old_CFLAGS"
AC_SUBST(DYNAMIC_LINK_FLAG)

gl_VISIBILITY
AS_IF([test x$gl_cv_cc_visibility = xyes],
	[AC_DEFINE(DLL_EXPORT_SYM,[__attribute__((visibility("default")))],[Explicitly export global symbols])],
	[AC_DEFINE(DLL_EXPORT_SYM,[],[Explicitly export global symbols])]
)

PKG_INSTALLDIR()

dnl LibXML2 2.5.0 introduced the XmlTextReader API
PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= 2.5.0)
AC_SUBST(LIBXML_LIBS)
AC_SUBST(LIBXML_CFLAGS)

PKG_CHECK_MODULES(UTF8PROC, libutf8proc,,
	[AC_CHECK_HEADER(utf8proc.h,
		[AC_CHECK_LIB(utf8proc, utf8proc_iterate,
			[UTF8PROC_LIBS="-lutf8proc"],
			[AC_MSG_ERROR([utf8proc is not installed])])],
		[AC_MSG_ERROR([utf8proc is not installed])])])
AC_SUBST(UTF8PROC_LIBS)
AC_SUBST(UTF8PROC_CFLAGS)

dnl GNU MP/MPFR detection
AC_CHECK_HEADERS(gmp.h mpfr.h mpf2mpfr.h,,[AC_MSG_ERROR([GNU MP and MPFR headers not found!])])
AC_CHECK_LIB(mpfr, mpfr_init2,[MPFR_LIBS="-lmpfr"],[AC_MSG_ERROR([GNU MPFR library not found!])])
AC_CHECK_LIB(gmp, __gmpf_init2,[MPFR_LIBS="$MPFR_LIBS -lgmp"],[AC_MSG_ERROR([GNU MP library not found!])])
AC_SUBST(MPFR_LIBS)

dnl libpaper detection
AC_CHECK_HEADERS(paper.h,,[AC_MSG_ERROR([libpaper header is not found])])
AC_CHECK_LIB(paper, paperinit,[PAPER_LIBS="-lpaper"],[AC_MSG_ERROR([libpaper library is not found])])
AC_SUBST(PAPER_LIBS)

AC_CHECK_HEADERS(csv.h,,[AC_MSG_ERROR([libcsv headers not found!])])
AC_CHECK_LIB(csv, csv_init,[CSV_LIBS=-lcsv],[AC_MSG_ERROR([libcsv library not found!])])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
	[[#include <csv.h>]],
	[[int i = CSV_APPEND_NULL]])],,
	[AC_MSG_ERROR([libcsv 3.0.0 or newer is required!])])
AC_SUBST(CSV_LIBS)

PKG_CHECK_MODULES(YAJL, yajl)
AC_SUBST(YAJL_CFLAGS)
AC_SUBST(YAJL_LIBS)

PKG_CHECK_MODULES(PDFGEN, cairo >= 1.6 cairo-pdf pangocairo >= 1.10 librsvg-2.0 gdk-pixbuf-2.0)
AC_SUBST(PDFGEN_CFLAGS)
AC_SUBST(PDFGEN_LIBS)

found_pgsql=no
PKG_CHECK_MODULES(POSTGRESQL, libpq,
	[found_pgsql=yes
	 AC_DEFINE(HAVE_POSTGRESQL,[1],[Have PostgreSQL])],
	[AC_CHECK_HEADERS(libpq-fe.h,
		[AC_CHECK_LIB(pq, PQexec,
			[found_pgsql=yes
			 POSTGRESQL_LIBS="-lpq"
			 AC_DEFINE(HAVE_POSTGRESQL,[1],[Have PostgreSQL])])])])

AC_ARG_ENABLE([old-pgsql],
	[AS_HELP_STRING([--enable-old-pgsql],
		[Use old PostgreSQL client method @<:@default=autodetect@:>@])],
	[AS_IF([test x$enable_old_pgsql = xyes],
		[AC_DEFINE(USE_PQEXEC,[1],[Use PQexec])
		 AC_DEFINE(USE_PQCONNECTDB,[1],[Use PQconnectdb instead of PQconnectdbParams])])],
	[AS_IF([test x$found_pgsql = xyes],
		[AC_CHECK_LIB(pq, PQdescribePortal,
			[],
			[AC_DEFINE(USE_PQEXEC,[1],[Use PQexec])])
		 AC_CHECK_LIB(pq, PQconnectdbParams,
			[],
			[AC_DEFINE(USE_PQCONNECTDB,[1],[Use PQconnectdb instead of PQconnectdbParams])])
		])])

AC_ARG_ENABLE([pgsql-cursor],
	[AS_HELP_STRING([--disable-pgsql-cursor],[Don't use cursor for PostgreSQL queries by default])],
	[AS_IF([test x$enable_pgsql_cursor = xyes],
		[AC_DEFINE(USE_PGSQL_CURSOR,[1],[Use PostgreSQL cursor for queries by default])])],
	[AC_DEFINE(USE_PGSQL_CURSOR,[1],[Use PostgreSQL cursor for queries by default])])

AC_SUBST(POSTGRESQL_CFLAGS)
AC_SUBST(POSTGRESQL_LIBS)
AM_CONDITIONAL(ENABLE_PGSQL_TESTS, [test x$found_pgsql = xyes])

AC_CHECK_PROG(MYSQL_CONFIG,[mysql_config],[mysql_config],[false])
AS_IF([test "$MYSQL_CONFIG" = "mysql_config"],
	[MYSQL_CLIENT([5.0.3],[thread-safe])])
AS_IF([test -n "$MYSQL_CLIENT_LIBS"],
	[AC_DEFINE(HAVE_MYSQL,[1],[Have MySQL or MariaDB])
	 found_mysql=yes])

AC_ARG_WITH([odbcmanager],
	[AS_HELP_STRING([--with-odbcmanager={unixodbc|iodbc}],
		[specify ODBC manager library @<:@default=autodetect@:>@])],
	[odbcmanager=$with_odbcmanager],
	[odbcmanager=auto])

AS_IF([test "x$odbcmanager" = "xyes"],[odbcmanager=auto],
	[test "x$odbcmanager" = "xno"],[odbcmanager=])

AC_ARG_WITH([unixodbc-headers],
	[AS_HELP_STRING([--with-unixodbc-headers=@<:@path@:>@],
		[specify location of unixODBC headers])],
	[AS_IF([test "x$with_unixodbc_headers" = "xyes" -o "x$with_unixodbc_headers" = "xno"],
		[with_unixodbc_headers=])])

AC_ARG_WITH([iodbc-headers],
	[AS_HELP_STRING([--with-iodbc-headers=@<:@path@:>@],
		[specify location of iODBC headers])],
	[AS_IF([test "x$with_iodbc_headers" = "xyes" -o "x$with_iodbc_headers" = "xno"],
		[with_iodbc_headers=])])

AS_IF([test "x$odbcmanager" = "xunixodbc" -o "x$odbcmanager" = "xauto"],
	[PKG_CHECK_MODULES(ODBC, odbc,
		[AC_DEFINE(HAVE_ODBC,[1],[Have ODBC manager])
		 odbcmanager=unixodbc],
		[AC_CHECK_LIB(odbc, SQLSetConnectAttr,
			[old_CFLAGS="$CFLAGS"
			 AS_IF([test -n "$with_unixodbc_headers"],[CFLAGS="$CFLAGS -I$with_unixodbc_headers"])
			 AC_CHECK_HEADERS(sql.h sqlext.h sqltypes.h,
				[ODBC_LIBS="-lodbc"
				 AC_DEFINE(HAVE_ODBC,[1],[Have ODBC manager])
				 odbcmanager=unixodbc],
				[AS_IF([test "x$odbcmanager" = "xunixodbc"],
					[AC_MSG_ERROR([unixODBC was requested but not found])])])
			 CFLAGS="$old_CFLAGS"],
			[AS_IF([test "x$odbcmanager" = "xunixodbc"],
				[AC_MSG_ERROR([unixODBC was requested but not found])])])])])

AS_IF([test "x$odbcmanager" = "xiodbc" -o "x$odbcmanager" = "xauto"],
	[PKG_CHECK_MODULES(ODBC, libiodbc,
		[AC_DEFINE(HAVE_ODBC,[1],[Have ODBC manager])
		 odbcmanager=iodbc],
		[AC_CHECK_LIB(iodbc, SQLSetConnectAttr,
			[old_CFLAGS="$CFLAGS"
			 AS_IF([test -n "$with_iodbc_headers"],[CFLAGS="$CFLAGS -I$with_iodbc_headers"])
			 AC_CHECK_HEADERS(sql.h sqlext.h sqltypes.h,
				[AC_DEFINE(HAVE_ODBC,[1],[Have ODBC manager])
				 ODBC_CFLAGS="-I$with_iodbc_headers"
				 ODBC_LIBS="-liodbc"
				 odbcmanager=iodbc],
				[AS_IF([test "x$odbcmanager" = "xiodbc"],
					[AC_MSG_ERROR([iODBC was requested but not found])])])
			 CFLAGS="$old_CFLAGS"],
			[AS_IF([test "x$odbcmanager" = "xiodbc"],
				[AC_MSG_ERROR([iODBC was requested but not found])])])])])

AS_IF([test "x$odbcmanager" = "xauto"],[odbcmanager=])

AC_SUBST(ODBC_CFLAGS)
AC_SUBST(ODBC_LIBS)

found_python=no
AC_ARG_WITH([pandas-source],
	[AS_HELP_STRING([--without-pandas-source],
		[Disable use of Python / Pandas for spreadsheet datasource @<:@default=autodetect@:>@])])

AS_IF([test x$enable_pandas_source != xno],
	[PKG_CHECK_MODULES(PYTHON, python3-embed,
	 	[AC_DEFINE(HAVE_LIBPYTHON,[1],[Have embedded python])
	 	 found_python=yes])])
AM_CONDITIONAL(ENABLE_PYTHON_TESTS, [test x$found_python = xyes])

AC_SUBST(PYTHON_CFLAGS)
AC_SUBST(PYTHON_LIBS)

AC_MSG_CHECKING([whether tests are enabled])
AC_ARG_ENABLE([tests],
	[AS_HELP_STRING([--enable-tests],[enable unit tests])],
	[],[enable_tests=no])
AC_MSG_RESULT([$enable_tests])
AM_CONDITIONAL(ENABLE_TESTS, [test x$enable_tests = xyes])

AC_CHECK_PROG(MSGFMT,[msgfmt],[msgfmt],[false])
AM_CONDITIONAL(ENABLE_XLATE_TESTS, [test x$ac_cv_prog_MSGFMT = xmsgfmt])

AC_CHECK_PROG(GHOSTSCRIPT,[ghostscript],[ghostscript],[false])
AC_CHECK_PROG(IMCOMPARE,[compare],[compare],[false])
AM_CONDITIONAL(ENABLE_PDF_TESTS, [test x$ac_cv_prog_GHOSTSCRIPT = xghostscript -a x$ac_cv_prog_IMCOMPARE = xcompare])

AC_MSG_CHECKING([whether building documentation is enabled])
AC_ARG_ENABLE([documentation],
    [AS_HELP_STRING([--enable-documentation],[enable building documentation])],
    [],[enable_documentation=yes])
AC_MSG_RESULT([$enable_documentation])

AC_CHECK_PROG(XMLLINT,[xmllint],[xmllint],[false])
AC_CHECK_PROG(XSLTPROC,[xsltproc],[xsltproc],[false])
AC_CHECK_PROG(FOP,[fop],[fop],[false])

AC_MSG_CHECKING([whether building HTML documentation is possible])
AS_IF([test x$ac_cv_prog_XMLLINT = xxmllint -a x$ac_cv_prog_XSLTPROC = xxsltproc],
	[enable_docs=yes],
	[enable_docs=no])
AC_MSG_RESULT([$enable_docs])
AS_IF([test x$enable_documentation = xno],[enable_docs=no])
AM_CONDITIONAL(ENABLE_DOCS, [test x$enable_docs = xyes])

AC_MSG_CHECKING([whether building PDF documentation is possible])
AS_IF([test x$enable_docs = xyes -a x$ac_cv_prog_FOP = xfop],
	[enable_pdf_docs=yes],
	[enable_pdf_docs=no])
AC_MSG_RESULT([$enable_pdf_docs])
AS_IF([test x$enable_documentation = xno],[enable_pdf_docs=no])
AM_CONDITIONAL(ENABLE_PDF_DOCS, [test x$enable_pdf_docs = xyes])

AS_IF([test x$enable_maintainer_mode = xyes],[WERRORFLAG=],[WERRORFLAG="-Werror"])
AC_SUBST(WERRORFLAG)

AC_CONFIG_FILES([
	Makefile
	opencreports.pc
	doc/Makefile
	doc/OpenCReports.sgm
	include/Makefile
	libsrc/Makefile
	tests/Makefile
])
AC_OUTPUT

AC_DEFUN([SUMMARY],
	 [pretty="$2"
	  if test "$pretty" = "unixodbc"; then
		pretty="yes, unixODBC"
	  elif test "$pretty" = "iodbc"; then
		pretty="yes, iODBC"
	  elif test "$pretty" = ""; then
		pretty=no
	  fi
	  echo "AS_HELP_STRING([$1:],[$pretty])"])

echo
echo "Summary"
echo
echo "Input drivers:"
echo
SUMMARY([PostgreSQL],[$found_pgsql])
SUMMARY([MySQL/MariaDB],[$found_mysql])
SUMMARY([ODBC],[$odbcmanager])
SUMMARY([Python / Pandas (XLS, XLSX, ODS)],[$found_python])
SUMMARY([Memory arrays],[always])
SUMMARY([CSV],[always])
SUMMARY([XML],[always])
SUMMARY([JSON],[always])
echo
echo "Output formats:"
echo
SUMMARY([PDF],[always])
SUMMARY([HTML],[always])
SUMMARY([TXT],[always])
SUMMARY([CSV],[always])
SUMMARY([XML],[always])
SUMMARY([JSON],[always])
echo
