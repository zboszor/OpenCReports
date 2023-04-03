/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <opencreport.h>

int main(void) {
	char srcdir0[PATH_MAX];
	char *abs_srcdir = getenv("abs_srcdir");
	if (!abs_srcdir || !*abs_srcdir)
		abs_srcdir = getcwd(srcdir0, sizeof(srcdir0));
	char *csrcdir = ocrpt_canonicalize_path(abs_srcdir);
	size_t cslen = strlen(csrcdir);
	opencreport *o = ocrpt_init();
	char *files[] = {
		"images/images/matplotlib.svg",
		"images2/images/matplotlib.svg",
		"images2/images2/matplotlib.svg",
		"images/images/doesnotexist.png",
		NULL,
	};
	int i;

	ocrpt_add_search_path(o, csrcdir);

	for (i = 0; files[i]; i++) {
		char *file = ocrpt_find_file(o, files[i]);
		printf("file '%s' found canonically '%s'\n", files[i], file ? file + cslen + 1 : "");
		ocrpt_mem_free(file);
	}

	ocrpt_free(o);

	ocrpt_mem_free(csrcdir);

	return 0;
}
