/*
 * List utilities
 *
 * Copyright (C) 2019-2020 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#include "opencreport.h"

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
		if (e->next)
			e = e->next;
		else
			break;
	}
	va_end(ap);
	return l;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_last(ocrpt_list *l) {
	ocrpt_list *ptr;

	if (!l)
		return NULL;

	ptr = l;
	while (ptr->next)
		ptr = ptr->next;

	return ptr;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_end_append(ocrpt_list *l, ocrpt_list **endptr, const void *data) {
	ocrpt_list *e = (endptr ? *endptr :NULL);

	if (!l) {
		e = l = ocrpt_makelist1(data);
		goto out;
	}
	if (!e)
		e = ocrpt_list_last(l);
	if (e->next)
		e = ocrpt_list_last(e);
	e->next = ocrpt_makelist1(data);
	if (e->next)
		l->len++;

	out:
	if (endptr)
		*endptr = e;
	return l;
}

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_append(ocrpt_list *l, const void *data) {
	ocrpt_list *e;

	if (!l)
		return ocrpt_makelist1(data);
	e = ocrpt_list_last(l);
	return ocrpt_list_end_append(l, &e, data);
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

DLL_EXPORT_SYM ocrpt_list *ocrpt_list_remove(ocrpt_list *l, const void *data) {
	ocrpt_list *e, *prev = NULL;

	if (!l)
		return NULL;

	for (e = l; e; prev = e, e = e->next) {
		if (e->data == data)
			break;
	}

	if (!e)
		return l;

	if (prev)
		prev->next = e->next;
	else
		l = e->next;
	if (l)
		l->len--;

	ocrpt_mem_free(e);

	return l;
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
