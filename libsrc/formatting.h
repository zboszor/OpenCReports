/*
 * Formatting utilities
 *
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _FORMATTING_H_
#define _FORMATTING_H_

#include "opencreport.h"

enum ocrpt_formatstring_type {
	OCRPT_FORMAT_NONE,
	OCRPT_FORMAT_LITERAL,
	OCRPT_FORMAT_STRING,
	OCRPT_FORMAT_NUMBER,
	OCRPT_FORMAT_MONEY,
	OCRPT_FORMAT_DATETIME
};

/* Number formatstring flags */
#define OCRPT_FORMAT_FLAG_ALTERNATE			(0x01)	/* '#' */
#define OCRPT_FORMAT_FLAG_0PADDED			(0x02)	/* '0' */
#define OCRPT_FORMAT_FLAG_LEFTALIGN			(0x04)	/* '-' */
#define OCRPT_FORMAT_FLAG_SIGN_BLANK		(0x08)	/* ' ' */
#define	OCRPT_FORMAT_FLAG_SIGN				(0x10)	/* '+' */
#define OCRPT_FORMAT_FLAG_GROUPING			(0x20)	/* '\'' or '$' (legacy format) */
#define OCRPT_FORMAT_FLAG_ALTERNATE_DIGITS	(0x40)	/* 'I' */

/* Monetary formatstring flags */
#define OCRPT_FORMAT_MFLAG_FILLCHAR			(0x01)	/* '=f' */
#define OCRPT_FORMAT_MFLAG_NOGROUPING		(0x02)	/* '^' */
#define OCRPT_FORMAT_MFLAG_NEG_PAR			(0x04)	/* '(' */
#define OCRPT_FORMAT_MFLAG_NEG_SIGN			(0x08)	/* '+' */
#define OCRPT_FORMAT_MFLAG_OMIT_CURRENCY		(0x10)	/* '!' */
#define OCRPT_FORMAT_MFLAG_LEFTALIGN			(0x20)	/* '-' */

struct ocrpt_format_string_element_t {
	int32_t type;
	int32_t flags;
	int32_t length;
	int32_t prec;
	char conv;
	ocrpt_string *string;
};

void ocrpt_utf8forward(const char *s, int l, int *l2, int blen, int *blen2);
void ocrpt_utf8backward(const char *s, int l, int *l2, int blen, int *blen2);

void ocrpt_format_string_literal(opencreport *o, ocrpt_expr *e, ocrpt_string *string, ocrpt_string *formatstring, ocrpt_string *value);
void ocrpt_format_string(opencreport *o, ocrpt_expr *e, ocrpt_string *string, ocrpt_string *formatstring, ocrpt_expr **data, int32_t n_expr);

#endif
