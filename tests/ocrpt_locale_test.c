/*
 * OpenCReports test
 * Copyright (C) 2019-2021 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

/*
 * This test demonstrates that the main application can
 * have different language setting than individual reports.
 *
 * This test requires installed language packs (locale data) for:
 * - en_GB
 * - fr_FR
 * - hu_HU
 */

#include <errno.h>
#include <langinfo.h>
#include <monetary.h>
#include <stdio.h>

#include <opencreport.h>

int main(void) {
	opencreport *o1 = ocrpt_init();
	opencreport *o2 = ocrpt_init();
	double dval = 1000000.0;
	mpfr_t mval;
	char sval[64];
	int32_t i, ret;

	setlocale(LC_ALL, "en_GB.UTF-8");

	ocrpt_set_locale(o1, "hu_HU.UTF-8");
	ocrpt_set_locale(o2, "fr_FR.UTF-8");

	printf("Hungarian month names:\n");
	for (i = 0; i < 12; i++)
		printf("%s %s\n", nl_langinfo_l(MON_1 + i, o1->locale), nl_langinfo_l(ABMON_1 + i, o1->locale));
	printf("\n");

	printf("Hungarian weekday names:\n");
	for (i = 0; i < 7; i++)
		printf("%s %s\n", nl_langinfo_l(DAY_1 + i, o1->locale), nl_langinfo_l(ABDAY_1 + i, o1->locale));
	printf("\n");

	printf("French month names:\n");
	for (i = 0; i < 12; i++)
		printf("%s %s\n", nl_langinfo_l(MON_1 + i, o2->locale), nl_langinfo_l(ABMON_1 + i, o2->locale));
	printf("\n");

	printf("French weekday names:\n");
	for (i = 0; i < 7; i++)
		printf("%s %s\n", nl_langinfo_l(DAY_1 + i, o2->locale), nl_langinfo_l(ABDAY_1 + i, o2->locale));
	printf("\n");

	mpfr_init2(mval, o1->prec);
	mpfr_set_si(mval, 1000000, o1->rndmode);

	errno = 0;
	ret = ocrpt_mpfr_strfmon(o1, sval, sizeof(sval), "%^=*#12n", mval);
	printf("Hungarian money (national notation): %s (%d %d)\n", sval, ret, errno);

	errno = 0;
	ret = ocrpt_mpfr_strfmon(o1, sval, sizeof(sval), "%=*#12i", mval);
	printf("Hungarian money (international notation): %s (%d %d)\n\n", sval, ret, errno);

	char *sep1 = nl_langinfo_l(__MON_THOUSANDS_SEP, o2->locale), *sep2 = nl_langinfo_l(__THOUSANDS_SEP, o2->locale);
	printf("thousands separator: money '%s' regular '%s'\n", sep1, sep2);
	errno = 0;
	ret = ocrpt_mpfr_strfmon(o2, sval, sizeof(sval), "%^=*#12n", mval);
	printf("French money (national notation): %s (%d %d)\n", sval, ret, errno);

	errno = 0;
	ret = ocrpt_mpfr_strfmon(o2, sval, sizeof(sval), "%=*#12i", mval);
	printf("French money (international notation): %s (%d %d)\n\n", sval, ret, errno);

	mpfr_clear(mval);

	errno = 0;
	ret = strfmon(sval, sizeof(sval), "%^=*#12n", dval);
	printf("British money (national notation): %s (%d %d)\n", sval, ret, errno);

	errno = 0;
	strfmon(sval, sizeof(sval), "%=*#12i", dval);
	printf("British money (international notation): %s (%d %d)\n\n", sval, ret, errno);

	ocrpt_free(o1);
	ocrpt_free(o2);

	return 0;
}
