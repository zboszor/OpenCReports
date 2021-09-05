/*
 * OpenCReports test
 * Copyright (C) 2021-2021 Zoltán Böszörményi <zboszor@gmail.com>
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

#include <locale.h>
#include <monetary.h>
#include <stdio.h>

#include <opencreport.h>

int main(void) {
	opencreport *o1 = ocrpt_init();
	opencreport *o2 = ocrpt_init();
	double dval = 100.0;
	mpfr_t mval;
	char sval[64];
	int32_t i;

	/*
	 * Protect setting locale from parallel threads
	 * doing the same temporarily. It's not a problem
	 * with this test but can be in a larger
	 * multi-threaded program.
	 */
	ocrpt_lock_global_locale_mutex();
	setlocale(LC_ALL, "en_GB.UTF-8");
	ocrpt_unlock_global_locale_mutex();

	ocrpt_set_locale(o1, "hu_HU.UTF-8");
	ocrpt_set_locale(o2, "fr_FR.UTF-8");

	printf("Hungarian month names:\n");
	for (i = 0; i < 12; i++)
		printf("%s %s\n", o1->monthname[i], o1->abmonthname[i]);
	printf("\n");

	printf("Hungarian weekday names:\n");
	for (i = 0; i < 7; i++)
		printf("%s %s\n", o1->dayname[i], o1->abdayname[i]);
	printf("\n");

	printf("French month names:\n");
	for (i = 0; i < 12; i++)
		printf("%s %s\n", o2->monthname[i], o2->abmonthname[i]);
	printf("\n");

	printf("French weekday names:\n");
	for (i = 0; i < 7; i++)
		printf("%s %s\n", o2->dayname[i], o2->abdayname[i]);
	printf("\n");

	mpfr_init2(mval, o1->prec);
	mpfr_set_si(mval, 100, o1->rndmode);

	ocrpt_mpfr_strfmon(o1, sval, sizeof(sval), "%^=*#6n", mval);
	printf("Hungarian money: %s\n\n", sval);

	ocrpt_mpfr_strfmon(o2, sval, sizeof(sval), "%^=*#6n", mval);
	printf("French money: %s\n\n", sval);

	mpfr_clear(mval);

	strfmon(sval, sizeof(sval), "%^=*#6n", dval);
	printf("British money: %s\n\n", sval);

	ocrpt_free(o1);
	ocrpt_free(o2);

	return 0;
}
