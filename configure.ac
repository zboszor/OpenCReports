dnl Configure template for OpenCReports

dnl Copyright (c) 2019-2024 Zoltán Böszörményi (zboszor@gmail.com)
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
AM_PROG_AR
AC_PROG_LEX([noyywrap])
AX_PROG_FLEX
AX_PROG_BISON
LT_INIT([disable-static win32-dll])

AC_CHECK_FUNCS([localtime_r strfmon])

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
	[COMPILE_SYMBOL="$COMPILE_SYMBOL -D_DEFAULT_SOURCE"],
	[test x$_gnu_source_reallocarray = xyes],
	[COMPILE_SYMBOL="$COMPILE_SYMBOL -D_GNU_SOURCE"],
	[AC_MSG_WARN([Using reallocarray may fail to compile!])])

AC_SUBST(COMPILE_SYMBOL)

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

PKG_CHECK_MODULES(PDFGEN, cairo cairo-pdf pangocairo >= 1.10 librsvg-2.0 gdk-pixbuf-2.0)
AC_SUBST(PDFGEN_CFLAGS)
AC_SUBST(PDFGEN_LIBS)

PKG_CHECK_MODULES(POSTGRESQL, libpq,,
	[AC_CHECK_HEADERS(libpq-fe.h,
		[AC_CHECK_LIB(pq, PQdescribePortal,
			[POSTGRESQL_LIBS="-lpq"],
			[AC_MSG_ERROR([PostgreSQL is not installed])])],
		[AC_MSG_ERROR([PostgreSQL is not installed])])])
AC_SUBST(POSTGRESQL_CFLAGS)
AC_SUBST(POSTGRESQL_LIBS)

MYSQL_CLIENT([5.0.3],[thread-safe])

AC_ARG_WITH([odbcmanager],
	[AS_HELP_STRING([--with-odbcmanager={unixodbc|iodbc}],
		[specify ODBC manager library @<:@default=unixodbc@:>@])],
	[odbcmanager=$with_odbcmanager],
	[odbcmanager=unixodbc])
AS_IF(
	[test x$odbcmanager = xunixodbc],
		[PKG_CHECK_MODULES(ODBC, odbc,,
			[AC_CHECK_HEADERS(sql.h sqlext.h sqltypes.h,
				[AC_CHECK_LIB(odbc, SQLSetConnectAttr,
					[ODBC_LIBS="-lodbc"],
					[AC_MSG_ERROR([ODBC library is not installed])])],
				[AC_MSG_ERROR([ODBC headers are not installed])])])],
	[test x$odbcmanager = xiodbc],[PKG_CHECK_MODULES(ODBC, libiodbc)],
	[AC_MSG_ERROR([Invalid ODBC manager type])])
AC_MSG_NOTICE([ODBC manager is $odbcmanager])
AC_SUBST(ODBC_CFLAGS)
AC_SUBST(ODBC_LIBS)

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

AC_CHECK_PROG(XMLLINT,[xmllint],[xmllint],[false])
AC_CHECK_PROG(XSLTPROC,[xsltproc],[xsltproc],[false])
AC_CHECK_PROG(FOP,[fop],[fop],[false])

AC_MSG_CHECKING([whether building HTML documentation is enabled])
AS_IF([test x$ac_cv_prog_XMLLINT = xxmllint -a x$ac_cv_prog_XSLTPROC = xxsltproc],
	[enable_docs=yes],
	[enable_docs=no])
AC_MSG_RESULT([$enable_docs])
AM_CONDITIONAL(ENABLE_DOCS, [test x$enable_docs = xyes])

AC_MSG_CHECKING([whether building PDF documentation is enabled])
AS_IF([test x$enable_docs = xyes -a x$ac_cv_prog_FOP = xfop],
	[enable_pdf_docs=yes],
	[enable_pdf_docs=no])
AC_MSG_RESULT([$enable_pdf_docs])
AM_CONDITIONAL(ENABLE_PDF_DOCS, [test x$enable_pdf_docs = xyes])

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
