/*
 * OpenCReports main header
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>

#include "opencreport-private.h"

#ifndef O_BINARY
#define O_BINARY (0)
#endif

void ocrpt_free_part(const struct opencreport_part *part) {
	ocrpt_mem_free(part->path);
	if (part->allocated)
		ocrpt_mem_free(part->xmlbuf);
	ocrpt_mem_free(part);
}

int32_t ocrpt_add_report_from_buffer_internal(opencreport *o, const char *buffer, bool allocated, const char *report_path) {
	struct opencreport_part *part = ocrpt_mem_malloc(sizeof(struct opencreport_part));
	int partsold;

	if (!part)
		return -1;

	part->xmlbuf = buffer;
	part->allocated = allocated;
	part->parsed = 0;
	part->path = ocrpt_mem_strdup(report_path);
	if (!part->path) {
		ocrpt_free_part(part);
		return -1;
	}

	partsold = list_length(o->parts);
	o->parts = list_append(o->parts, part);
	if (list_length(o->parts) != partsold + 1) {
		ocrpt_free_part(part);
		return -1;
	}

	return 0;
}

int32_t ocrpt_add_report_from_buffer(opencreport *o, const char *buffer) {
	return ocrpt_add_report_from_buffer_internal(o, buffer, 0, cwdpath);
}

int32_t ocrpt_add_report(opencreport *o, const char *filename) {
	struct stat st;
	char *str, *fnamecopy, *dir;
	int fd;
	ssize_t len, orig;

	if (!filename)
		return -1;

	if (stat(filename, &st) == -1)
		return -1;

	fd = open(filename, O_RDONLY | O_BINARY);
	if (fd < 0)
		return -1;

	str = ocrpt_mem_malloc(st.st_size + 1);
	if (!str) {
		close(fd);
		return -1;
	}

	orig = st.st_size;
	do {
		len = read(fd, str, orig);
		if (len < 0)
			break;
		orig -= len;
	} while (orig > 0);

	close(fd);

	if (len < 0) {
		ocrpt_mem_free(str);
		return -1;
	}

	fnamecopy = ocrpt_mem_strdup(filename);
	dir = dirname(fnamecopy);
	ocrpt_mem_free(fnamecopy);

	return ocrpt_add_report_from_buffer_internal(o, str, 1, dir);
}

void ocrpt_free_parts(opencreport *o) {
	list_free_deep(o->parts, (ocrpt_mem_free_t)ocrpt_free_part);
	o->parts = NULL;
}

int32_t ocrpt_parse(opencreport *o) {
	return ocrpt_parse2(o, false);
}

int32_t ocrpt_parse2(opencreport *o, bool allow_bad_xml) {
	return -1;
}
