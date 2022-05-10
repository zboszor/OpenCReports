/*
 * Formatting utilities
 *
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "opencreport.h"
#include "formatting.h"

static bool ocrpt_good_conv(char c) {
	switch (c) {
	case 'd': /* legacy number */
	case 'a': /* various floating point conversions for the C format string */
	case 'A':
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
		return true;
	default:
		return false;
	}
}

static bool ocrpt_good_money_conv(char c) {
	switch (c) {
	case 'i': /* international monetary format */
	case 'n': /* national monetary format */
		return true;
	default:
		return false;
	}
}

static int32_t ocrpt_formatstring_flags(const char *fmt, int32_t *advance) {
	int32_t flags = 0;
	int32_t pos, quit = 0;

	for (pos = 0; !quit && fmt[pos]; pos++) {
		switch (fmt[pos]) {
		case '#':
			flags |= OCRPT_FORMAT_FLAG_ALTERNATE;
			break;
		case '0':
			flags |= OCRPT_FORMAT_FLAG_0PADDED;
			break;
		case '-':
			flags |= OCRPT_FORMAT_FLAG_LEFTALIGN;
			break;
		case ' ':
			flags |= OCRPT_FORMAT_FLAG_SIGN_BLANK;
			break;
		case '+':
			flags |= OCRPT_FORMAT_FLAG_SIGN;
		case '\'':
		case '$':
			flags |= OCRPT_FORMAT_FLAG_GROUPING;
			break;
		case 'I':
			flags |= OCRPT_FORMAT_FLAG_ALTERNATE_DIGITS;
			break;
		default:
			quit = 1;
			break;
		}
		if (quit)
			break;
	}
	*advance = pos;
	return flags;
}

static int32_t ocrpt_formatstring_flags_money(const char *fmt, char *filler, int32_t *advance) {
	int32_t flags = 0;
	char fill = '\0';
	int32_t pos, quit = 0;

	for (pos = 0; !quit && fmt[pos]; pos++) {
		switch (fmt[pos]) {
		case '=':
			if (!fmt[pos + 1]) {
				quit = 1;
				break;
			}
			pos++;
			flags |= OCRPT_FORMAT_MFLAG_FILLCHAR;
			fill = fmt[pos];
			break;
		case '^':
			flags |= OCRPT_FORMAT_MFLAG_NOGROUPING;
			break;
		case '(':
			flags |= OCRPT_FORMAT_MFLAG_NEG_PAR;
			break;
		case '+':
			flags |= OCRPT_FORMAT_MFLAG_NEG_SIGN;
			break;
		case '!':
			flags |= OCRPT_FORMAT_MFLAG_OMIT_CURRENCY;
		case '-':
			flags |= OCRPT_FORMAT_MFLAG_LEFTALIGN;
			break;
		default:
			quit = 1;
			break;
		}
		if (quit)
			break;
	}

	*filler = fill;
	*advance = pos;
	return flags;
}

static void ocrpt_formatstring_length_prec(const char *fmt, int32_t *length, bool *length_set, int32_t *prec, bool *prec_set, int32_t *advance) {
	int32_t pos = 0;
	ocrpt_string *tmp = ocrpt_mem_string_new("", true);

	*length = 0;
	*length_set = false;
	*prec = 0;
	*prec_set = false;

	for (pos = 0; fmt[pos] && isdigit(fmt[pos]); pos++)
		ocrpt_mem_string_append_c(tmp, fmt[pos]);
	ocrpt_mem_string_append_c(tmp, '\0');
	if (strlen(tmp->str) > 0) {
		*length = atoi(tmp->str);
		*length_set = true;
	}

	if (fmt[pos] != '.') {
		ocrpt_mem_string_free(tmp, true);
		*advance = pos;
		return;
	}

	tmp->len = 0;
	for (pos++; fmt[pos] && isdigit(fmt[pos]); pos++)
		ocrpt_mem_string_append_c(tmp, fmt[pos]);
	ocrpt_mem_string_append_c(tmp, '\0');

	if (strlen(tmp->str) > 0) {
		*prec = atoi(tmp->str);
		*prec_set = true;
	}

	ocrpt_mem_string_free(tmp, true);
	*advance = pos;
	return;
}

static void ocrpt_formatstring_money_length_prec(const char *fmt, int32_t *length, int32_t *lprec, int32_t *prec, int32_t *advance) {
	int32_t pos = 0;
	ocrpt_string *tmp = ocrpt_mem_string_new("", true);

	*length = 0;
	*lprec = 0;
	*prec = 0;

	for (pos = 0; fmt[pos] && isdigit(fmt[pos]); pos++)
		ocrpt_mem_string_append_c(tmp, fmt[pos]);
	ocrpt_mem_string_append_c(tmp, '\0');
	if (strlen(tmp->str) > 0)
		*length = atoi(tmp->str);

	if (fmt[pos] == '#') {
		tmp->len = 0;
		for (pos++; fmt[pos] && isdigit(fmt[pos]); pos++)
			ocrpt_mem_string_append_c(tmp, fmt[pos]);
		ocrpt_mem_string_append_c(tmp, '\0');

		if (strlen(tmp->str) > 0)
			*lprec = atoi(tmp->str);
	}

	if (fmt[pos] != '.') {
		ocrpt_mem_string_free(tmp, true);
		*advance = pos;
		return;
	}

	tmp->len = 0;
	for (pos++; fmt[pos] && isdigit(fmt[pos]); pos++)
		ocrpt_mem_string_append_c(tmp, fmt[pos]);
	ocrpt_mem_string_append_c(tmp, '\0');

	if (strlen(tmp->str) > 0)
		*prec = atoi(tmp->str);

	ocrpt_mem_string_free(tmp, true);
	*advance = pos;
	return;
}

#define FORMAT_ERROR \
					do { \
						str->len = 0; \
						ocrpt_mem_string_append_printf(str, "format error"); \
						*error = true; \
						*out_type = type; \
						return str; \
					} while (0)

#define BRACKETED_FORMAT \
					int i; \
					for (i = adv; fmt[i] && fmt[i] != '}'; i++) \
						; \
					if (fmt[i] != '}') \
						FORMAT_ERROR;

#define COPY_FORMAT(start, end, extra) \
					do { \
						ocrpt_mem_string_append_len(str, fmt + start, end - start); \
						ocrpt_mem_string_append_c(str, '\0'); \
						*out_type = type; \
						*advance = end - start + extra; \
					} while (0)

#define SWITCH_TO_LITERAL(x) \
					do { \
						switch (type) { \
						case OCRPT_FORMAT_NONE: \
							type = OCRPT_FORMAT_LITERAL; \
							/* fall through */ \
						case OCRPT_FORMAT_LITERAL: \
							if (x == 0) { \
								ocrpt_mem_string_append_len(str, fmt + adv, 2); \
								adv += 2; \
							} else if (x == 1) { \
								ocrpt_mem_string_append_len(str, fmt + adv, 2); \
								if (fmt[adv + 1]) \
									adv += 2; \
								else { \
									adv++; \
									*advance = adv; \
									*out_type = type; \
									return str; \
								} \
							} else if (x == 2) { \
								ocrpt_mem_string_append_c(str, fmt[adv]); \
								if (fmt[adv + 1] == '%') \
									adv += 2; \
								else \
									adv++; \
							} else if (x == 3) { \
								ocrpt_mem_string_append_c(str, fmt[adv]); \
								adv++; \
							} \
							break; \
						default: \
							FORMAT_ERROR; \
						} \
					} while (0)

#define RETURN_LITERAL \
					do { \
						ocrpt_mem_string_append_c(str, '\0'); \
						*out_type = type; \
						*advance = adv; \
						return str; \
					} while (0);

#define RETURN_END_LITERAL(rettype, extra) \
					do { \
						ocrpt_mem_string_append(str, fmt + adv + extra); \
						adv += strlen(fmt + adv); \
						*out_type = rettype; \
						*advance = adv; \
						return str; \
					} while (0)

#define HANDLE_NUMBER_FORMAT(legacy_grouping, skip) \
					do { \
						int32_t adv2, adv3; \
						int32_t flags; \
						bool legacy = false, length_set, prec_set; \
						int32_t length, prec; \
								\
						flags = ocrpt_formatstring_flags(fmt + adv + 1 + legacy_grouping + skip, &adv2); \
						ocrpt_formatstring_length_prec(fmt + adv + 1 + legacy_grouping + skip + adv2, &length, &length_set, &prec, &prec_set, &adv3); \
						if (legacy_grouping) \
							flags |= OCRPT_FORMAT_FLAG_GROUPING; \
						char c = fmt[adv + 1 + legacy_grouping + skip + adv2 + adv3]; \
						if ((ocrpt_good_conv(c) && expected_type == OCRPT_FORMAT_NUMBER) || (c == 's' && expected_type == OCRPT_FORMAT_STRING) ) { \
							type = expected_type; \
							if (c == 'd') { \
								/* Legacy format regarding length/precision count */ \
								if (prec) \
									length += prec + 1; \
								c = 'f'; \
								legacy = true; \
							} \
							str->len = 0; \
							ocrpt_mem_string_append_c(str, '%'); \
							if ((flags & OCRPT_FORMAT_FLAG_ALTERNATE)) \
								ocrpt_mem_string_append_c(str, '#'); \
							if ((flags & OCRPT_FORMAT_FLAG_0PADDED)) \
								ocrpt_mem_string_append_c(str, '0'); \
							if ((flags & OCRPT_FORMAT_FLAG_LEFTALIGN)) \
								ocrpt_mem_string_append_c(str, '-'); \
							if ((flags & OCRPT_FORMAT_FLAG_SIGN_BLANK)) \
								ocrpt_mem_string_append_c(str, ' '); \
							if ((flags & OCRPT_FORMAT_FLAG_SIGN)) \
								ocrpt_mem_string_append_c(str, '+'); \
							if ((flags & OCRPT_FORMAT_FLAG_GROUPING)) \
								ocrpt_mem_string_append_c(str, '\''); \
							if ((flags & OCRPT_FORMAT_FLAG_ALTERNATE_DIGITS)) \
								ocrpt_mem_string_append_c(str, 'I'); \
							if (expected_type == OCRPT_FORMAT_STRING) { \
								if (length > 0 && prec == 0) { \
									prec = length; \
									prec_set = true; \
									length = 0; \
								} \
							} \
							if (length) \
								ocrpt_mem_string_append_printf(str, "%d", length); \
							if (prec) \
								ocrpt_mem_string_append_printf(str, ".%d", prec); \
							else if (legacy) \
								ocrpt_mem_string_append(str, ".0"); \
							if (expected_type == OCRPT_FORMAT_NUMBER) \
								ocrpt_mem_string_append_printf(str, "R%c", c); \
							else \
								ocrpt_mem_string_append_c(str, c); \
							*advance = adv + adv2 + adv3 + 2 + legacy_grouping + skip; \
							*out_type = type; \
							if (prec_set && out_length > 0) \
								*out_length = prec; \
							if (lpadded) \
								*lpadded = !!(flags & OCRPT_FORMAT_FLAG_SIGN_BLANK); \
							return str; \
						} else { \
							FORMAT_ERROR; \
						} \
					} while (0)

#define HANDLE_MONETARY_FORMAT(start, resetlen, newadvance) \
					do { \
						char filler = '\0', conv; \
						int32_t adv1, adv2; \
						int32_t flags, length, lprec, prec; \
																\
						flags = ocrpt_formatstring_flags_money(start, &filler, &adv1); \
						ocrpt_formatstring_money_length_prec(start + adv1, &length, &lprec, &prec, &adv2); \
						if (prec > 0) \
							FORMAT_ERROR; \
																\
						conv = *(start + adv1 + adv2); \
						if (!ocrpt_good_money_conv(conv)) \
							FORMAT_ERROR; \
																\
						if (resetlen) \
							str->len = 0; \
						ocrpt_mem_string_append_c(str, '%'); \
						if ((flags & OCRPT_FORMAT_MFLAG_NOGROUPING)) \
							ocrpt_mem_string_append_c(str, '^'); \
						if ((flags & OCRPT_FORMAT_MFLAG_NEG_PAR)) \
							ocrpt_mem_string_append_c(str, '('); \
						if ((flags & OCRPT_FORMAT_MFLAG_NEG_SIGN)) \
							ocrpt_mem_string_append_c(str, '+'); \
						if ((flags & OCRPT_FORMAT_MFLAG_OMIT_CURRENCY)) \
							ocrpt_mem_string_append_c(str, '!'); \
						if ((flags & OCRPT_FORMAT_MFLAG_LEFTALIGN)) \
							ocrpt_mem_string_append_c(str, '-'); \
						if ((flags & OCRPT_FORMAT_MFLAG_FILLCHAR)) \
							ocrpt_mem_string_append_printf(str, "=%c", filler); \
						if (length > 0) \
							ocrpt_mem_string_append_printf(str, "%d", length); \
						if (lprec > 0) \
							ocrpt_mem_string_append_printf(str, "#%d", lprec); \
						if (prec > 0) \
							ocrpt_mem_string_append_printf(str, ".%d", prec); \
						ocrpt_mem_string_append_c(str, conv); \
						ocrpt_mem_string_append_c(str, '\0'); \
																\
						*out_type = type; \
						*advance = newadvance; \
						return str; \
					} while (0)

static ocrpt_string *ocrpt_get_next_format_string(opencreport *o, const char *fmt, enum ocrpt_formatstring_type expected_type, enum ocrpt_formatstring_type *out_type, int32_t *advance, bool *error, int32_t *out_length, bool *lpadded) {
	ocrpt_string *str;
	enum ocrpt_formatstring_type type = OCRPT_FORMAT_NONE;
	int32_t adv = 0;

	*error = false;

	if (fmt == NULL) {
		*out_type = type;
		*advance = 0;
		return NULL;
	}

	if (fmt[adv] == '\0') {
		*out_type = type;
		*advance = 0;
		return NULL;
	}

	str = ocrpt_mem_string_new(NULL, false);

	while (fmt[adv]) {
		if (expected_type == OCRPT_FORMAT_LITERAL) {
			/* Shortcut when a literal is expected. */
			while (fmt[adv]) {
				/* Convert %% to % but don't bother with anything else. */
				if (fmt[adv] == '%' && fmt[adv + 1] == '%') {
					ocrpt_mem_string_append_c(str, '%');
					adv += 2;
				} else {
					ocrpt_mem_string_append_c(str, fmt[adv]);
					adv++;
				}
			}
			*out_type = OCRPT_FORMAT_LITERAL;
			*advance = adv;
			return str;
		}

		if (fmt[adv] == '!') {
			/* Possibly a new style format string */
			switch (fmt[adv + 1]) {
			case '@':
				/* Possibly a new style date format string for strfmon style formatting. */
				if (expected_type != OCRPT_FORMAT_DATETIME)
					SWITCH_TO_LITERAL(0);
				else {
					switch (type) {
					case OCRPT_FORMAT_LITERAL:
						RETURN_LITERAL;
					case OCRPT_FORMAT_NONE:
						/* Date... */
						type = OCRPT_FORMAT_DATETIME;
						if (fmt[adv + 2] == '{') {
							BRACKETED_FORMAT;
							COPY_FORMAT(adv + 3, i - 6, 4);
							return str;
						} else
							RETURN_END_LITERAL(type, 2);
						break;
					default:
						FORMAT_ERROR;
					}
				}
				break;
			case '$':
				/* Possibly a new style monetary number for strfmon style formatting */
				if (expected_type != OCRPT_FORMAT_NUMBER) {
					SWITCH_TO_LITERAL(0);
				} else {
					switch (type) {
					case OCRPT_FORMAT_LITERAL:
						RETURN_LITERAL;
					case OCRPT_FORMAT_NONE:
						/* Monetary number... */
						type = OCRPT_FORMAT_MONEY;
						if (fmt[adv + 2] == '{') {
							BRACKETED_FORMAT;

							ocrpt_mem_string_append_len(str, fmt + adv + 3, i - adv - 3);
							ocrpt_mem_string_append_c(str, '\0');

							if (str->str[0] != '%')
								FORMAT_ERROR;

							HANDLE_MONETARY_FORMAT(str->str + 1, true, i - adv + 1);
						} else {
							/*
							 * Accept any filler char as ornament
							 * before the actual format string
							 */
							while (fmt[adv + 2] != '\0' && fmt[adv + 2] != '%') {
								ocrpt_mem_string_append_c(str, fmt[adv + 2]);
								adv++;
							}

							if (fmt[adv + 2] == '\0')
								FORMAT_ERROR;

							HANDLE_MONETARY_FORMAT(fmt + adv + 3, false, adv + adv1 + adv2 + 4);
						}
						break;
					default:
						FORMAT_ERROR;
					}
				}
				break;
			case '#':
				/* Possibly a new style number for printf style formatting */
				if (expected_type != OCRPT_FORMAT_NUMBER) {
					if (expected_type != OCRPT_FORMAT_LITERAL && (fmt[adv + 2] == '%' || (fmt[adv + 2] == '{' && fmt[adv + 3] == '%'))) {
						switch (type) {
						case OCRPT_FORMAT_LITERAL:
							RETURN_LITERAL;
						default:
							FORMAT_ERROR;
						}
					}
					SWITCH_TO_LITERAL(0);
				} else {
					switch (type) {
					case OCRPT_FORMAT_LITERAL:
						RETURN_LITERAL;
					case OCRPT_FORMAT_NONE:
						/* Number... */
						type = OCRPT_FORMAT_NUMBER;
						if (fmt[adv + 2] == '{') {
							BRACKETED_FORMAT;
							COPY_FORMAT(adv + 3, i, 4);

							ocrpt_string *str1;
							enum ocrpt_formatstring_type type1;
							int32_t adv1;
							bool error1;

							str1 = ocrpt_get_next_format_string(o, str->str, OCRPT_FORMAT_NUMBER, &type1, &adv1, &error1, out_length, lpadded);
							ocrpt_mem_string_free(str, true);
							*advance = adv1 + 4;
							return str1;
						} else {
							/*
							 * Accept any filler char as ornament
							 * before the actual format string.
							 */
							while (fmt[adv + 2] != '\0' && fmt[adv + 2] != '%') {
								ocrpt_mem_string_append_c(str, fmt[adv + 2]);
								adv++;
							}

							if (fmt[adv + 2] == '\0')
								FORMAT_ERROR;

							HANDLE_NUMBER_FORMAT(0, 2);
						}

						break;
					default:
						FORMAT_ERROR;
					}
				}
				break;
			case '&':
				/* Possibly a new style string for printf style formatting */
				if (expected_type != OCRPT_FORMAT_STRING) {
					if (expected_type != OCRPT_FORMAT_LITERAL && (fmt[adv + 2] == '%' || (fmt[adv + 2] == '{' && fmt[adv + 3] == '%'))) {
						switch (type) {
						case OCRPT_FORMAT_LITERAL:
							RETURN_LITERAL;
						default:
							FORMAT_ERROR;
						}
					}
					SWITCH_TO_LITERAL(0);
				} else {
					switch (type) {
					case OCRPT_FORMAT_LITERAL:
						RETURN_LITERAL;
					case OCRPT_FORMAT_NONE:
						/* String... */
						type = OCRPT_FORMAT_STRING;
						if (fmt[adv + 2] == '{') {
							BRACKETED_FORMAT;
							COPY_FORMAT(adv + 3, i, 4);

							ocrpt_string *str1;
							enum ocrpt_formatstring_type type1;
							int32_t adv1;
							bool error1;

							str1 = ocrpt_get_next_format_string(o, str->str, OCRPT_FORMAT_STRING, &type1, &adv1, &error1, out_length, lpadded);
							ocrpt_mem_string_free(str, true);
							*advance = adv1 + 4;
							return str1;
						} else {
							/*
							 * Accept any filler char as ornament
							 * before the actual format string.
							 */
							while (fmt[adv + 2] != '\0' && fmt[adv + 2] != '%') {
								ocrpt_mem_string_append_c(str, fmt[adv + 2]);
								adv++;
							}

							if (fmt[adv + 2] == '\0')
								FORMAT_ERROR;

							HANDLE_NUMBER_FORMAT(0, 2);
						}

						break;
					default:
						FORMAT_ERROR;
					}
				}
				break;
			default:
				SWITCH_TO_LITERAL(1);
			}
		} else if (fmt[adv] == '%') {
			/*
			 * Legacy format string handling
			 */
			if (!fmt[adv + 1] || fmt[adv + 1] == '%') {
				SWITCH_TO_LITERAL(2);
			} else if (strlen(fmt + adv + 1) > 1 && fmt[adv + 1] == '$') {
				/* Legacy thousand grouping in numbers */
				HANDLE_NUMBER_FORMAT(1, 0);
			} else {
				switch (expected_type) {
				case OCRPT_FORMAT_DATETIME:
					switch (type) {
					case OCRPT_FORMAT_LITERAL:
						RETURN_LITERAL;
					case OCRPT_FORMAT_NONE:
						RETURN_END_LITERAL(OCRPT_FORMAT_DATETIME, 0);
					default:
						FORMAT_ERROR;
					}
				case OCRPT_FORMAT_NUMBER:
				case OCRPT_FORMAT_STRING:
					switch (type) {
					case OCRPT_FORMAT_LITERAL:
						RETURN_LITERAL;
					case OCRPT_FORMAT_NONE:
						break;
					default:
						FORMAT_ERROR;
					}
					HANDLE_NUMBER_FORMAT(0, 0);
				case OCRPT_FORMAT_NONE:
				case OCRPT_FORMAT_LITERAL:
				case OCRPT_FORMAT_MONEY:
					break;
				}
			}
		} else {
			SWITCH_TO_LITERAL(3);
		}
	}

	*advance = adv;
	*out_type = type;
	return str;
}

void ocrpt_utf8forward(const char *s, int l, int *l2, int blen, int *blen2) {
	int i = 0, j = 0;

	while (j < l && i < blen) {
		if ((s[i] & 0xf8) == 0xf0)
			i+= 4, j++;
		else if ((s[i] & 0xf0) == 0xe0)
			i += 3, j++;
		else if ((s[i] & 0xe0) == 0xc0)
			i += 2, j++;
		else
			i++, j++;
	}

	if (i > blen)
		i = blen;

	if (l2)
		*l2 = j;
	if (blen2)
		*blen2 = i;
}

void ocrpt_utf8backward(const char *s, int l, int *l2, int blen, int *blen2) {
	int i = blen, j = 0;

	while (j < l && i > 0) {
		i--;
		if (((s[i] & 0xf8) == 0xf0) || ((s[i] & 0xf0) == 0xe0) || ((s[i] & 0xe0) == 0xc0) || ((s[i] & 0x80) == 0x00))
			j++;
	}

	if (l2)
		*l2 = j;
	if (blen2)
		*blen2 = i;
}

void ocrpt_format_string(opencreport *o, ocrpt_expr *e, const char *formatstring, int32_t formatlen, ocrpt_expr **expr, int32_t n_expr) {
	int32_t i, advance;
	locale_t locale;

	/* Use the specified locale, so that thousand separators, etc. work. */
	locale = uselocale(o->locale);

	ocrpt_string *string = ocrpt_mem_string_resize(e->result[o->residx]->string, 16);
	if (string) {
		if (!e->result[o->residx]->string) {
			e->result[o->residx]->string = string;
			e->result[o->residx]->string_owned = true;
		}
		string->len = 0;
	}

	advance = 0;
	for (i = 0; i < n_expr; i++) {
		ocrpt_result *data = expr[i]->result[o->residx];
		enum ocrpt_formatstring_type types[2] = { OCRPT_FORMAT_NONE, OCRPT_FORMAT_LITERAL };
		int32_t type_idx;
		bool data_handled = false;

		switch (data->type) {
		case OCRPT_RESULT_STRING:
			types[0] = OCRPT_FORMAT_STRING;
			break;
		case OCRPT_RESULT_NUMBER:
			types[0] = OCRPT_FORMAT_NUMBER;
			break;
		case OCRPT_RESULT_DATETIME:
			types[0] = OCRPT_FORMAT_DATETIME;
			break;
		case OCRPT_RESULT_ERROR:
			ocrpt_expr_make_error_result(o, e, data->string->str);
			return;
		}

		type_idx = 0;
		while (advance < formatlen && formatstring[advance]) {
			ocrpt_string *tmp;
			char *result;
			enum ocrpt_formatstring_type type;
			int32_t adv, length;
			ssize_t len;
			bool error, lpadded;

			length = -1;
			lpadded = 0;
			assert(type_idx < 2);
			tmp = ocrpt_get_next_format_string(o, formatstring + advance, types[type_idx], &type, &adv, &error, &length, &lpadded);

			if (error) {
				ocrpt_expr_make_error_result(o, e, tmp->str);
				ocrpt_mem_string_free(tmp, true);
				return;
			}

			if (types[type_idx] == type || (types[type_idx] == OCRPT_FORMAT_NUMBER && type == OCRPT_FORMAT_MONEY)) {
				type_idx++;
				data_handled = true;
			}

			switch (type) {
			case OCRPT_FORMAT_LITERAL:
				if (!data_handled || i == (n_expr - 1))
					ocrpt_mem_string_append(string, tmp->str);
				else
					goto end_inner_loop;
				break;
			case OCRPT_FORMAT_NUMBER:
				if (!data->isnull && mpfr_asprintf(&result, tmp->str, data->number) >= 0) {
					ocrpt_mem_string_append(string, result);
					mpfr_free_str(result);
				}
				data_handled = true;
				break;
			case OCRPT_FORMAT_MONEY:
				if (!data->isnull && (len = ocrpt_mpfr_strfmon(o, NULL, 0, tmp->str, data->number)) >= 0) {
					result = ocrpt_mem_malloc(len + 1);
					ocrpt_mpfr_strfmon(o, result, len, tmp->str, data->number);
					result[len] = 0;
					ocrpt_mem_string_append(string, result);
					ocrpt_mem_free(result);
				}
				data_handled = true;
				break;
			case OCRPT_FORMAT_DATETIME:
				if (!data->isnull) {
					char dt[256];

					strftime_l(dt, sizeof(dt), tmp->str, &data->datetime, o->locale);
					ocrpt_mem_string_append(string, dt);
				}
				data_handled = true;
				break;
			case OCRPT_FORMAT_STRING:
				if (!data->isnull) {
					int32_t blen = data->string->len;

					if (length > 0) {
						int32_t slen;
						ocrpt_utf8forward(data->string->str, length, &slen, data->string->len, &blen);

						if (lpadded) {
							char *padstr;
							int32_t padlen;

							padlen = length - slen;
							padstr = ocrpt_mem_malloc(padlen + 1);
							memset(padstr, ' ', padlen);
							padstr[padlen] = 0;

							ocrpt_mem_string_append(string, padstr);

							ocrpt_mem_free(padstr);
						}
					}

					ocrpt_mem_string_append_len(string, data->string->str, blen);
				}
				data_handled = true;
				break;
			case OCRPT_FORMAT_NONE:
				break;
			}

			end_inner_loop:
			ocrpt_mem_string_free(tmp, true);

			advance += adv;

			if (data_handled && i != (n_expr - 1))
				break;
		}
	}

	uselocale(locale);
}
