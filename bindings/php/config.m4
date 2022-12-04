PHP_ARG_ENABLE([opencreport],[whether to enable opencreport module],
[  --enable-opencreport      Enable opencreport module],[shared],[yes])

if test "$PHP_OPENCREPORT" != "no"; then
	AC_MSG_CHECKING([whether opencreport is available])

	PHP_TEST_BUILD(ocrpt_init,
		[AC_MSG_RESULT([yes])],
		[AC_MSG_RESULT([no])
		 AC_MSG_ERROR([build test failed.  Please check the config.log for details.])],
		[-lopencreport])

	PHP_NEW_EXTENSION(ocrpt, [opencreport.c], $ext_shared)
	PHP_EVAL_LIBLINE($LIBS, OPENCREPORT_SHARED_LIBADD)
fi
