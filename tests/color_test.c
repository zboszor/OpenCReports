/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <opencreport.h>

int main(void) {
	opencreport *o = ocrpt_init();
	char *colornames[] = { "Black", "Red", "bobkratz", NULL };
	int i;

	for (i = 0; colornames[i]; i++) {
		ocrpt_color c;

		ocrpt_get_color(o, colornames[i], &c, false);

		printf("%s: %.4lf %.4lf %.4lf\n", colornames[i], c.r, c.g, c.b);
	}

	ocrpt_free(o);

	return 0;
}
