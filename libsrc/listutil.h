/*
 * List utilities
 *
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _LISTUTIL_H_
#define _LISTUTIL_H_

#include <stddef.h>
#include "memutil.h"

struct List {
	struct List *next;
	const void *data;
	size_t len;
};
typedef struct List List;

#define list_length(l) ((l) ? (l)->len : 0)

extern List *makelist1(const void *data);
extern List *makelist(const void *data1, ...);
extern List *list_last(List *l);
extern List *list_end_append(List *l, List *e, const void *data);
extern List *list_append(List *l, const void *data);
extern List *list_prepend(List *l, const void *data);
extern List *list_remove(List *l, const void *data);
extern void list_free(List *l);
extern void list_free_deep(List *l, ocrpt_mem_free_t freefunc);

#endif
