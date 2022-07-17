/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include "opencreport.h"

int main(void) {
	ocrpt_result *r;

	setenv("OCRPTENV", "This is a test string", 1);
	r = ocrpt_environment_get("OCRPTENV");
	printf("OCRPTENV is set. Value is: ");
	ocrpt_result_print(r);
	ocrpt_result_free(r);

	unsetenv("OCRPTENV");
	printf("OCRPTENV is unset. Value is: ");
	r = ocrpt_environment_get("OCRPTENV");
	ocrpt_result_print(r);
	ocrpt_result_free(r);

	return 0;
}
