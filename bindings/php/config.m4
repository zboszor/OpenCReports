PHP_ARG_ENABLE([opencreports],[whether to enable opencreport module],
[  --enable-opencreports     Enable opencreports module],[shared],[yes])

PHP_ARG_WITH([opencreports-static-array],[whether to use static arrays],
[  --with-opencreports-static-array
                             Use PHP arrays with statically discovered data],[no],[no])
if test "$PHP_OPENCREPORTS_STATIC_ARRAY" != "no"; then
	PHP_DEFINE(opencreports_use_static_array,[1])
else
	PHP_DEFINE(opencreports_use_static_array,[0])
fi

if test "$PHP_OPENCREPORTS" != "no"; then
	AC_MSG_CHECKING(for pkg-config)
	if test ! -f "$PKG_CONFIG"; then
		PKG_CONFIG=`which pkg-config`
	fi
	if test ! -f "$PKG_CONFIG"; then
		AC_MSG_RESULT(not found)
		AC_MSG_ERROR(pkg-config is not found)
	fi
	AC_MSG_RESULT(found)

	AC_MSG_CHECKING([whether opencreports is available])

	PHP_TEST_BUILD(ocrpt_init,
		[AC_MSG_RESULT([yes])],
		[AC_MSG_RESULT([no])
		 AC_MSG_ERROR([build test failed.  Please check the config.log for details.])],
		[-lopencreport])

	AC_MSG_CHECKING(for opencreports via pkgconfig)
	if $PKG_CONFIG --exists opencreports; then
		AC_MSG_RESULT(found)
	else
		AC_MSG_RESULT(not found)
		AC_MSG_ERROR(opencreport is not found)
	fi
	OPENCREPORTS_CFLAGS="`$PKG_CONFIG --cflags opencreports`"
	OPENCREPORTS_LIBS="`$PKG_CONFIG --libs opencreports`"
	AC_DEFINE_UNQUOTED(PHP_OPENCREPORT_VERSION,["m4_normalize(m4_include([../../VERSION]))"],[OpenCReports version])

	CFLAGS="-Wall -Werror $CFLAGS $OPENCREPORTS_CFLAGS"
	LDFLAGS="$OPENCREPORTS_LIBS $LDFLAGS"

	PHP_NEW_EXTENSION(opencreports, [opencreport.c ocrptinit.c rlibcompat.c ocrpt-object.c ds-object.c query-object.c qresult-object.c expr-object.c eresult-object.c], $ext_shared)
	PHP_EVAL_INCLINE($CFLAGS)
	PHP_EVAL_LIBLINE($LDFLAGS, OPENCREPORTS_SHARED_LIBADD)
fi
