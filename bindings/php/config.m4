PHP_ARG_ENABLE([opencreport],[whether to enable opencreport module],
[  --enable-opencreport      Enable opencreport module],[shared],[yes])

if test "$PHP_OPENCREPORT" != "no"; then
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
	OPENCREPORT_CFLAGS="`$PKG_CONFIG --cflags opencreports`"
	OPENCREPORT_LIBS="`$PKG_CONFIG --libs opencreports`"

	CFLAGS="-Wall -Werror $CFLAGS"
	LDFLAGS="$OPENCREPORT_LIBS $LDFLAGS"

	PHP_NEW_EXTENSION(opencreport, [opencreport.c], $ext_shared)
	PHP_EVAL_INCLINE($CFLAGS)
	PHP_EVAL_LIBLINE($LDFLAGS, OPENCREPORT_SHARED_LIBADD)
fi
