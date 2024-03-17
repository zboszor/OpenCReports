/*
 * OpenCReports header
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _LISTUTIL_H_
#define _LISTUTIL_H_

struct ocrpt_list {
	struct ocrpt_list *next;
	const void *data;
	size_t len;
};

#endif
