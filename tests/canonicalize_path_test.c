/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <opencreport.h>

int main(int argc, char **argv) {
	char srcdir0[PATH_MAX];
	char *srcdir = getenv("abs_srcdir");
	if (!srcdir || !*srcdir)
		srcdir = getcwd(srcdir0, sizeof(srcdir0));
	char *csrcdir = ocrpt_canonicalize_path(srcdir);
	size_t slen = strlen(csrcdir);

	ocrpt_string *s1 = ocrpt_mem_string_new_with_len(NULL, strlen(csrcdir) * 2);

	int i, len = strlen(csrcdir), slashes = 0;
	for (i = 0; i < len; i++, s1->len++) {
		if (csrcdir[i] == '/') {
			slashes++;
			for (int j = 0; j < slashes; j++)
				s1->str[s1->len++] = csrcdir[i];
		}
		s1->str[s1->len] = csrcdir[i];
	}
	s1->str[s1->len] = '\0';

	ocrpt_string *s2 = ocrpt_mem_string_new(s1->str, true);

	ocrpt_mem_string_append_printf(s1, "/%s", "../tests/images/images");
	/* Both "images2" are symlinks to the "images" subdirs at the same directory level. */
	ocrpt_mem_string_append_printf(s2, "/%s", "../tests/images2/images2");

	char *s1c = ocrpt_canonicalize_path(s1->str);
	char *s2c = ocrpt_canonicalize_path(s2->str);

	if (strcmp(s1c, s2c) == 0) {
		printf("Two subdirs relative to abs_srcdir are equal after canonicalization:\n");
		printf("\t%s\n", s1c + slen + 1);
		printf("\t%s\n", s2c + slen + 1);
	}

	ocrpt_mem_string_free(s1, true);
	ocrpt_mem_string_free(s2, true);

	ocrpt_mem_free(s1c);
	ocrpt_mem_free(s2c);

	s1 = ocrpt_mem_string_new(csrcdir, true);

	/* "images/images5" is a bad recursive symlink, canonicalization returns it as is */
	ocrpt_mem_string_append(s1, "/images/images5");

	s1c = ocrpt_canonicalize_path(s1->str);
	printf("Bad recursive symlink, returned as is: %s\n", s1c + slen + 1);

	ocrpt_mem_free(s1c);
	ocrpt_mem_string_free(s1, true);

	ocrpt_mem_free(csrcdir);

	s1c = ocrpt_canonicalize_path("images2/images2");
	printf("Canonicalized path of 'images2/images2' relative to abs_srcdir: '%s'\n", s1c + slen + 1);
	ocrpt_mem_free(s1c);

	return 0;
}
