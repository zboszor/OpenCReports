/*
 * List utilities
 *
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#include "opencreport.h"
#include "listutil.h"

DLL_EXPORT_SYM size_t ocrpt_list_length(ocrpt_list *l) {
	return l ? l->len : 0;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_makelist1(const void *data) {
	ocrpt_list *l = ocrpt_mem_malloc(sizeof(ocrpt_list));

	if (!l)
		return NULL;

	l->len = 1;
	l->data = data;
	l->next = NULL;

	return l;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_makelist(const void *data, ...) {
	ocrpt_list *l, *e;
	void *arg;
	va_list ap;

	l = ocrpt_makelist1(data);
	if (!l)
		return NULL;
	e = l;

	va_start(ap, data);
	while ((arg = va_arg(ap, void *))) {
		l = ocrpt_list_end_append(l, &e, arg);
	}
	va_end(ap);
	return l;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_nth(const ocrpt_list *l, uint32_t n) {
	ocrpt_list *l1;
	uint32_t i;

	if (n < 0)
		return NULL;

	for (i = 0, l1 = (ocrpt_list *)l; l && i < n - 1; l = l->next, i++)
		;

	return l1;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_last(const ocrpt_list *l) {
	if (!l)
		return NULL;

	ocrpt_list *ptr = (ocrpt_list *)l;
	while (ptr->next)
		ptr = ptr->next;

	return ptr;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_end_append(ocrpt_list *l, ocrpt_list **endptr, const void *data) {
	ocrpt_list *e = (endptr ? *endptr : NULL);

	if (!l) {
		e = l = ocrpt_makelist1(data);
		goto out;
	}
	if (!e)
		e = ocrpt_list_last(l);
	if (e->next)
		e = ocrpt_list_last(e);
	e->next = ocrpt_makelist1(data);
	if (!e->next)
		return l;

	l->len++;
	e = e->next;

	out:
	if (endptr)
		*endptr = e;
	return l;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_append(ocrpt_list *l, const void *data) {
	return ocrpt_list_end_append(l, NULL, data);
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_prepend(ocrpt_list *l, const void *data) {
	ocrpt_list *e;

	if (!l)
		return ocrpt_makelist1(data);
	e = ocrpt_mem_malloc(sizeof(ocrpt_list));
	if (!e)
		return l;
	e->next = l;
	e->data = data;
	e->len = l->len + 1;
	return e;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_end_remove(ocrpt_list *l, ocrpt_list **endptr, const void *data) {
	ocrpt_list *e, *prev = NULL;

	if (!l)
		return NULL;

	for (e = l; e; prev = e, e = e->next) {
		if (e->data == data)
			break;
	}

	if (!e)
		return l;

	size_t len = l->len;

	if (prev) {
		prev->next = e->next;

		if (prev->next == NULL && endptr)
			*endptr = prev;
	} else {
		l = e->next;
		if (endptr) {
			if (l) {
				if (l->next == NULL)
					*endptr = l;
			} else
				*endptr = NULL;
		}
	}

	if (l)
		l->len = len - 1;

	ocrpt_mem_free(e);


	return l;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_remove(ocrpt_list *l, const void *data) {
	ocrpt_list *end = NULL;
	return ocrpt_list_end_remove(l, &end, data);
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_next(ocrpt_list *l) {
	return l ? l->next : NULL;
}

DLL_EXPORT_SYM void *ocrpt_list_get_data(ocrpt_list *l) {
	if (!l)
		return NULL;

	return (void *)l->data;
}

DLL_EXPORT_SYM void ocrpt_list_free(ocrpt_list *l) {
	while (l) {
		ocrpt_list *prev = l;
		l = l->next;
		ocrpt_mem_free(prev);
	}
}

DLL_EXPORT_SYM void ocrpt_list_free_deep(ocrpt_list *l, ocrpt_mem_free_t freefunc) {
	while (l) {
		ocrpt_list *prev = l;
		l = l->next;
		freefunc(prev->data);
		ocrpt_mem_free(prev);
	}
}
