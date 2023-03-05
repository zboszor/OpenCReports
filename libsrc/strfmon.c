/*
 * ocrpt_mpfr_strfmon()
 *
 * Equivalent of strfmon() that works with values from GNU MPFR.
 *
 * Changes made for OpenCReports by Zoltán Böszörményi <zboszor@gmail.com>
 *
 * Original FreeBSD code is at:
 *
 * https://opensource.apple.com/source/Libc/Libc-320/stdlib/FreeBSD/strfmon.c
 *
 * Copyright (c) 2001 Alexey Zelkin <phantom@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <config.h>

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: src/lib/libc/stdlib/strfmon.c,v 1.14 2003/03/20 08:18:55 ache Exp $");
#endif

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <langinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpfr.h>
#include <opencreport.h>
#include "ocrpt-private.h"

/* internal flags */
#define	NEED_GROUPING		0x01	/* print digits grouped (default) */
#define	SIGN_POSN_USED		0x02	/* '+' or '(' usage flag */
#define	LOCALE_POSN		0x04	/* use locale defined +/- (default) */
#define	PARENTH_POSN		0x08	/* enclose negative amount in () */
#define	SUPRESS_CURR_SYMBOL	0x10	/* supress the currency from output */
#define	LEFT_JUSTIFY		0x20	/* left justify */
#define	USE_INTL_CURRENCY	0x40	/* use international currency symbol */
#define IS_NEGATIVE		0x80	/* is argument value negative ? */

/* internal macros */
#define PRINT(CH) do {						\
	if (s) {								\
		if (dst >= s + maxsize) 			\
			goto e2big_error;				\
		*dst++ = CH;						\
	} else									\
		dst++;								\
} while (0)

#define PRINTS(STR) do {			\
	char *tmps = STR;				\
	while (*tmps != '\0') {			\
		PRINT(*tmps);				\
		tmps++;						\
	}								\
} while (0)

#define GET_NUMBER(VAR)	do {					\
	VAR = 0;						\
	while (isdigit((unsigned char)*fmt)) {			\
		VAR *= 10;					\
		VAR += *fmt - '0';				\
		fmt++;						\
	}							\
} while (0)

#define GRPCPY(howmany) do {					\
	int i = howmany;					\
	while (i-- > 0) {					\
		avalue_size--;					\
		*--bufend = *(avalue+avalue_size+padded);	\
	}							\
} while (0)

#define GRPSEP do {						\
	int i;	\
	for (i = thousands_sep_len; i; i--)	\
		*--bufend = thousands_sep[i - 1];	\
	groups++;						\
} while (0)

static void __setup_vars(opencreport *, int, char *, char *, char *, char **);
static int __calc_left_pad(opencreport *, int, char *);
static char *__format_grouped_mpfr(opencreport *, mpfr_t, int *, int, int, int);

DLL_EXPORT_SYM ssize_t ocrpt_mpfr_strfmon(opencreport *o, char * __restrict s, size_t maxsize, const char * __restrict format, ...) {
	va_list		ap;
	char 		*dst;		/* output destination pointer */
	const char 	*fmt;		/* current format poistion pointer */
	char		*asciivalue;	/* formatted mpfr_t pointer */

	int		flags;		/* formatting options */
	int		pad_char;	/* padding character */
	int		pad_size;	/* pad size */
	int		width;		/* field width */
	int		left_prec;	/* left precision */
	int		right_prec;	/* right precision */
	mpfr_t	val;		/* local value */
	mpfr_srcptr	val_ptr;/* passed-in value */
	char	space_char = ' '; /* space after currency */
	char	cs_precedes,	/* values gathered from nl_langinfo_l() */
			sep_by_space,
			sign_posn,
			*signstr,
			*currency_symbol;
	char	*tmpptr;	/* temporary vars */
	int		sverrno;

	va_start(ap, format);

	dst = s;
	fmt = format;
	asciivalue = NULL;
	currency_symbol = NULL;
	pad_size = 0;

	mpfr_init2(val, o->prec);

	while (*fmt) {
		/* pass nonformating characters AS IS */
		if (*fmt != '%')
			goto literal;

		/* '%' found ! */

		/* "%%" mean just '%' */
		if (*(fmt+1) == '%') {
			fmt++;
	literal:
			PRINT(*fmt++);
			continue;
		}

		/* set up initial values */
		flags = (NEED_GROUPING|LOCALE_POSN);
		pad_char = ' ';		/* padding character is "space" */
		left_prec = -1;		/* no left precision specified */
		right_prec = -1;	/* no right precision specified */
		width = -1;		/* no width specified */
		val_ptr = NULL;	/* we have no value to print now */

		/* Flags */
		while (1) {
			switch (*++fmt) {
				case '=':	/* fill character */
					pad_char = *++fmt;
					if (pad_char == '\0')
						goto format_error;
					continue;
				case '^':	/* not group currency  */
					flags &= ~(NEED_GROUPING);
					continue;
				case '+':	/* use locale defined signs */
					if (flags & SIGN_POSN_USED)
						goto format_error;
					flags |= (SIGN_POSN_USED|LOCALE_POSN);
					continue;
				case '(':	/* enclose negatives with () */
					if (flags & SIGN_POSN_USED)
						goto format_error;
					flags |= (SIGN_POSN_USED|PARENTH_POSN);
					continue;
				case '!':	/* suppress currency symbol */
					flags |= SUPRESS_CURR_SYMBOL;
					continue;
				case '-':	/* alignment (left)  */
					flags |= LEFT_JUSTIFY;
					continue;
				default:
					break;
			}
			break;
		}

		/* field Width */
		if (isdigit((unsigned char)*fmt)) {
			GET_NUMBER(width);
			/* Do we have enough space to put number with
			 * required width ?
			 */
			if (dst + width >= s + maxsize)
				goto e2big_error;
		}

		/* Left precision */
		if (*fmt == '#') {
			if (!isdigit((unsigned char)*++fmt))
				goto format_error;
			GET_NUMBER(left_prec);
		}

		/* Right precision */
		if (*fmt == '.') {
			if (!isdigit((unsigned char)*++fmt))
				goto format_error;
			GET_NUMBER(right_prec);
		}

		/* Conversion Characters */
		switch (*fmt++) {
			case 'i':	/* use internaltion currency format */
				flags |= USE_INTL_CURRENCY;
				break;
			case 'n':	/* use national currency format */
				flags &= ~(USE_INTL_CURRENCY);
				break;
			default:	/* required character is missing or
					   premature EOS */
				goto format_error;
		}

		if (flags & USE_INTL_CURRENCY) {
			currency_symbol = ocrpt_mem_strdup(nl_langinfo_l(__INT_CURR_SYMBOL, o->locale));
			if (currency_symbol != NULL) {
				space_char = *(currency_symbol + 3);
				*(currency_symbol + 3) = '\0';
			}
		} else
			currency_symbol = ocrpt_mem_strdup(nl_langinfo_l(__CURRENCY_SYMBOL, o->locale));

		if (currency_symbol == NULL)
			goto end_error;			/* ENOMEM. */

		/* value itself */
		val_ptr = va_arg(ap, mpfr_srcptr);
		mpfr_set_prec(val, mpfr_get_prec(val_ptr));

		/* detect sign */
		if (mpfr_sgn(val_ptr) < 0) {
			flags |= IS_NEGATIVE;
			mpfr_abs(val, val_ptr, o->rndmode);
		} else
			mpfr_set(val, val_ptr, o->rndmode);

		/* fill left_prec with amount of padding chars */
		if (left_prec >= 0) {
			pad_size = __calc_left_pad(o, (flags ^ IS_NEGATIVE),
							currency_symbol) -
				   __calc_left_pad(o, flags, currency_symbol);
			if (pad_size < 0)
				pad_size = 0;
		}

		asciivalue = __format_grouped_mpfr(o, val, &flags, left_prec, right_prec, pad_char);
		if (asciivalue == NULL)
			goto end_error;		/* errno already set     */
						/* to ENOMEM by malloc() */

		/* set some variables for later use */
		__setup_vars(o, flags, &cs_precedes, &sep_by_space,
				&sign_posn, &signstr);

		/*
		 * Description of some LC_MONETARY's values:
		 *
		 * p_cs_precedes & n_cs_precedes
		 *
		 * = 1 - $currency_symbol precedes the value
		 *       for a monetary quantity with a non-negative value
		 * = 0 - symbol succeeds the value
		 *
		 * p_sep_by_space & n_sep_by_space
		 *
		 * = 0 - no space separates $currency_symbol
		 *       from the value for a monetary quantity with a
		 *	 non-negative value
		 * = 1 - space separates the symbol from the value
		 * = 2 - space separates the symbol and the sign string,
		 *       if adjacent.
		 *
		 * p_sign_posn & n_sign_posn
		 *
		 * = 0 - parentheses enclose the quantity and the
		 *	 $currency_symbol
		 * = 1 - the sign string precedes the quantity and the 
		 *       $currency_symbol
		 * = 2 - the sign string succeeds the quantity and the 
		 *       $currency_symbol
		 * = 3 - the sign string precedes the $currency_symbol
		 * = 4 - the sign string succeeds the $currency_symbol
		 */

		tmpptr = dst;

		while (pad_size-- > 0)
			PRINT(' ');

		if (sign_posn == 0 && (flags & IS_NEGATIVE))
			PRINT('(');

		if (cs_precedes == 1) {
			if (sign_posn == 1 || sign_posn == 3) {
				PRINTS(signstr);
				if (sep_by_space == 2)		/* XXX: ? */
					PRINT(' ');
			}

			if (!(flags & SUPRESS_CURR_SYMBOL)) {
				PRINTS(currency_symbol);

				if (sign_posn == 4) {
					if (sep_by_space == 2)
						PRINT(space_char);
					PRINTS(signstr);
					if (sep_by_space == 1)
						PRINT(' ');
				} else if (sep_by_space == 1)
					PRINT(space_char);
			}
		} else if (sign_posn == 1)
			PRINTS(signstr);

		PRINTS(asciivalue);

		if (cs_precedes == 0) {
			if (sign_posn == 3) {
				if (sep_by_space == 1)
					PRINT(' ');
				PRINTS(signstr);
			}

			if (!(flags & SUPRESS_CURR_SYMBOL)) {
				if ((sign_posn == 3 && sep_by_space == 2)
				    || (sep_by_space == 1
				    && (sign_posn == 0
				    || sign_posn == 1
				    || sign_posn == 2
				    || sign_posn == 4)))
					PRINT(space_char);
				PRINTS(currency_symbol); /* XXX: len */
				if (sign_posn == 4) {
					if (sep_by_space == 2)
						PRINT(' ');
					PRINTS(signstr);
				}
			}
		}

		if (sign_posn == 2) {
			if (sep_by_space == 2)
				PRINT(' ');
			PRINTS(signstr);
		}

		if (sign_posn == 0 && (flags & IS_NEGATIVE))
			PRINT(')');

		if (dst - tmpptr < width) {
			if (flags & LEFT_JUSTIFY) {
				while (dst - tmpptr < width)
					PRINT(' ');
			} else {
				pad_size = dst-tmpptr;
				memmove(tmpptr + width-pad_size, tmpptr,
				    pad_size);
				memset(tmpptr, ' ', width-pad_size);
				dst += width-pad_size;
			}
		}
	}

	PRINT('\0');
	va_end(ap);
	mpfr_clear(val);
	ocrpt_mem_free(currency_symbol);
	ocrpt_mem_free(asciivalue);
	return (dst - s - 1);	/* return size of put data except trailing '\0' */

e2big_error:
	errno = E2BIG;
	goto end_error;

format_error:
	errno = EINVAL;

end_error:
	sverrno = errno;
	ocrpt_mem_free(currency_symbol);
	ocrpt_mem_free(asciivalue);
	errno = sverrno;
	va_end(ap);
	mpfr_clear(val);
	return (-1);
}

static void
__setup_vars(opencreport *o, int flags, char *cs_precedes, char *sep_by_space,
		char *sign_posn, char **signstr) {

	if ((flags & IS_NEGATIVE) && (flags & USE_INTL_CURRENCY)) {
		*cs_precedes = *nl_langinfo_l(__INT_N_CS_PRECEDES, o->locale);
		*sep_by_space = *nl_langinfo_l(__INT_N_SEP_BY_SPACE, o->locale);
		*sign_posn = (flags & PARENTH_POSN) ? 0 : *nl_langinfo_l(__INT_N_SIGN_POSN, o->locale);
		*signstr = (*nl_langinfo_l(__NEGATIVE_SIGN, o->locale) == '\0') ? "-"
		    : nl_langinfo_l(__NEGATIVE_SIGN, o->locale);
	} else if (flags & USE_INTL_CURRENCY) {
		*cs_precedes = *nl_langinfo_l(__INT_P_CS_PRECEDES, o->locale);
		*sep_by_space = *nl_langinfo_l(__INT_P_SEP_BY_SPACE, o->locale);
		*sign_posn = (flags & PARENTH_POSN) ? 0 : *nl_langinfo_l(__INT_P_SIGN_POSN, o->locale);
		*signstr = nl_langinfo_l(__POSITIVE_SIGN, o->locale);
	} else if (flags & IS_NEGATIVE) {
		*cs_precedes = *nl_langinfo_l(__N_CS_PRECEDES, o->locale);
		*sep_by_space = *nl_langinfo_l(__N_SEP_BY_SPACE, o->locale);
		*sign_posn = (flags & PARENTH_POSN) ? 0 : *nl_langinfo_l(__N_SIGN_POSN, o->locale);
		*signstr = (*nl_langinfo_l(__NEGATIVE_SIGN, o->locale) == '\0') ? "-"
		    : nl_langinfo_l(__NEGATIVE_SIGN, o->locale);
	} else {
		*cs_precedes = *nl_langinfo_l(__P_CS_PRECEDES, o->locale);
		*sep_by_space = *nl_langinfo_l(__P_SEP_BY_SPACE, o->locale);
		*sign_posn = (flags & PARENTH_POSN) ? 0 : *nl_langinfo_l(__P_SIGN_POSN, o->locale);
		*signstr = nl_langinfo_l(__POSITIVE_SIGN, o->locale);
	}

	/* Set defult values for unspecified information. */
	if (*cs_precedes != 0)
		*cs_precedes = 1;
	if (*sep_by_space == CHAR_MAX)
		*sep_by_space = 0;
	if (*sign_posn == CHAR_MAX)
		*sign_posn = 0;
}

static int
__calc_left_pad(opencreport *o, int flags, char *cur_symb) {

	char cs_precedes, sep_by_space, sign_posn, *signstr;
	int left_chars = 0;

	__setup_vars(o, flags, &cs_precedes, &sep_by_space, &sign_posn, &signstr);

	if (cs_precedes != 0) {
		left_chars += strlen(cur_symb);
		if (sep_by_space != 0)
			left_chars++;
	}

	switch (sign_posn) {
		case 1:
			left_chars += strlen(signstr);
			break;
		case 3:
		case 4:
			if (cs_precedes != 0)
				left_chars += strlen(signstr);
	}
	return (left_chars);
}

static int
get_groups(int size, char *grouping) {

	int	chars = 0;

	if (*grouping == CHAR_MAX || *grouping <= 0)	/* no grouping ? */
		return (0);

	while (size > (int)*grouping) {
		chars++;
		size -= (int)*grouping++;
		/* no more grouping ? */
		if (*grouping == CHAR_MAX || *grouping <= 0)
			break;
		/* rest grouping with same value ? */
		if (*grouping == 0) {
			chars += (size - 1) / *(grouping - 1);
			break;
		}
	}
	return (chars);
}

/* convert mpfr_t to ASCII */
static char *__format_grouped_mpfr(opencreport *o, mpfr_t value, int *flags, int left_prec, int right_prec, int pad_char) {
	char		*rslt;
	char		*avalue;
	int		avalue_size;
	char		fmt[32];

	size_t		bufsize;
	char		*bufend;

	int		padded, decimal_point_len, thousands_sep_len;

	char		*grouping;
	char		*decimal_point;
	char		*thousands_sep;

	int groups = 0;

	grouping = nl_langinfo_l(__MON_GROUPING, o->locale);
	decimal_point = nl_langinfo_l(__MON_DECIMAL_POINT, o->locale);
	if (*decimal_point == '\0')
		decimal_point = nl_langinfo_l(__DECIMAL_POINT, o->locale);
	decimal_point_len = strlen(decimal_point);
	thousands_sep = nl_langinfo_l(__MON_THOUSANDS_SEP, o->locale);
	if (*thousands_sep == '\0')
		thousands_sep = nl_langinfo_l(__THOUSANDS_SEP, o->locale);
	thousands_sep_len = strlen(thousands_sep);

	/* fill left_prec with default value */
	if (left_prec == -1)
		left_prec = 0;

	/* fill right_prec with default value */
	if (right_prec == -1) {
		if (*flags & USE_INTL_CURRENCY)
			right_prec = *nl_langinfo_l(__INT_FRAC_DIGITS, o->locale);
		else
			right_prec = *nl_langinfo_l(__FRAC_DIGITS, o->locale);

		/* Sometimes it's (char)-1, i.e. UCHAR_MAX sign-extended to int */
		if (right_prec >= CHAR_MAX || right_prec < 0)	/* POSIX locale ? */
			right_prec = 2;
	}

	if (*flags & NEED_GROUPING)
		left_prec += get_groups(left_prec, grouping);

	/* convert to string */
	snprintf(fmt, sizeof(fmt), "%%%d.%dRf", left_prec + right_prec + 1, right_prec);
	avalue_size = mpfr_asprintf(&avalue, fmt, value);
	if (avalue_size < 0)
		return (NULL);

	/* make sure that we've enough space for result string */
	bufsize = strlen(avalue) * 2 + 1;
	rslt = ocrpt_mem_malloc(bufsize);
	if (rslt == NULL) {
		mpfr_free_str(avalue);
		return (NULL);
	}
	memset(rslt, 0, bufsize);
	bufend = rslt + bufsize - 1;	/* reserve space for trailing '\0' */

	/* skip spaces at beggining */
	padded = 0;
	while (avalue[padded] == ' ') {
		padded++;
		avalue_size--;
	}

	if (right_prec > 0) {
		int i;
		bufend -= right_prec;
		memcpy(bufend, avalue + avalue_size + padded - right_prec, right_prec);
		for (i = decimal_point_len; i; i--)
			*--bufend = decimal_point[i - 1];
		avalue_size -= (right_prec + 1);
	}

	if ((*flags & NEED_GROUPING) &&
	    *thousands_sep != '\0' &&	/* XXX: need investigation */
	    *grouping != CHAR_MAX &&
	    *grouping > 0) {
		while (avalue_size > (int)*grouping) {
			GRPCPY(*grouping);
			GRPSEP;
			grouping++;

			/* no more grouping ? */
			if (*grouping == CHAR_MAX || *grouping <= 0)
				break;

			/* rest grouping with same value ? */
			if (*grouping == 0) {
				grouping--;
				while (avalue_size > *grouping) {
					GRPCPY(*grouping);
					GRPSEP;
				}
			}
		}
		if (avalue_size != 0)
			GRPCPY(avalue_size);
		padded -= groups;
	} else {
		bufend -= avalue_size;
		memcpy(bufend, avalue+padded, avalue_size);
		if (right_prec == 0)
			padded--;	/* decrease assumed $decimal_point */
	}

	/* do padding with pad_char */
	if (padded > 0) {
		bufend -= padded;
		memset(bufend, pad_char, padded);
	}

	bufsize = bufsize - (bufend - rslt);
	memmove(rslt, bufend, bufsize);
	mpfr_free_str(avalue);
	return (rslt);
}
