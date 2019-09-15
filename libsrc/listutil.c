/*
 * List utilities
 *
 * Copyright (C) 2019 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdarg.h>
#include <stdio.h>
#include "memutil.h"
#include "listutil.h"

List *makelist1(const void *data) {
	List *l = ocrpt_mem_malloc(sizeof(List));

	if (!l)
		return NULL;

	l->len = 1;
	l->data = data;
	l->next = NULL;

	return l;
}

List *makelist(const void *data, ...) {
	List *l, *e;
	void *arg;
	va_list ap;

	l = makelist1(data);
	if (!l)
		return NULL;
	e = l;

	va_start(ap, data);
	while ((arg = va_arg(ap, void *))) {
		l = list_end_append(l, e, arg);
		if (e->next)
			e = e->next;
		else
			break;
	}
	va_end(ap);
	return l;
}

List *list_last(List *l) {
	List *ptr;

	if (!l)
		return NULL;

	ptr = l;
	while (ptr->next)
		ptr = ptr->next;

	return ptr;
}

List *list_end_append(List *l, List *e, const void *data) {
	if (!l)
		return makelist1(data);
	if (!e)
		e = list_last(l);
	if (e->next)
		e = list_last(e);
	e->next = makelist1(data);
	if (e->next)
		l->len++;
	return l;
}

List *list_append(List *l, const void *data) {
	List *e;

	if (!l)
		return makelist1(data);
	e = list_last(l);
	return list_end_append(l, e, data);
}

List *list_prepend(List *l, const void *data) {
	List *e;

	if (!l)
		return makelist1(data);
	e = ocrpt_mem_malloc(sizeof(List));
	if (!e)
		return l;
	e->next = l;
	e->data = data;
	e->len = l->len + 1;
	return e;
}

List *list_remove(List *l, const void *data) {
	List *e, *prev = NULL;

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

void list_free(List *l) {
	while (l) {
		List *prev = l;
		l = l->next;
		ocrpt_mem_free(prev);
	}
}

void list_free_deep(List *l, ocrpt_mem_free_t freefunc) {
	while (l) {
		List *prev = l;
		l = l->next;
		freefunc(prev->data);
		ocrpt_mem_free(prev);
	}
}
