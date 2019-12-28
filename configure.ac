dnl Configure template for OpenCReports

dnl Copyright (c) 2019 Zoltán Böszörményi (zboszor@gmail.com)
dnl OpenCReports is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU Lesser General Public License as published by
dnl the Free Software Foundation; either version 3, or (at your option)
dnl any later version.

dnl OpenCReports is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU Lesser General Public License for more details.

AC_INIT([OpenCReports],[0.1],[https://github.com/zboszor/OpenCReports])

AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR(include/opencreport.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE()
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE

OCRPT_VERSION=0.1
AC_SUBST(OCRPT_VERSION)

OCRPT_LT_CURRENT=1
OCRPT_LT_AGE=0
OCRPT_LT_REVISION=0
AC_SUBST(OCRPT_LT_CURRENT)
AC_SUBST(OCRPT_LT_REVISION)
AC_SUBST(OCRPT_LT_AGE)

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
AM_PROG_AR
AX_PROG_FLEX
AX_PROG_BISON
LT_INIT([disable-static win32-dll])

AC_CHECK_FUNCS([localtime_r strfmon])

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

PKG_CHECK_MODULES(UTF8PROC, libutf8proc)
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

AC_CONFIG_FILES([
	Makefile
	opencreports.pc
	doc/Makefile
	include/Makefile
	libsrc/Makefile
	tests/Makefile
])
AC_OUTPUT
