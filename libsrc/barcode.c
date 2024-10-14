/*
 * OpenCReports barcode rendering module
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#include <config.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <opencreport.h>
#include "fallthrough.h"
#include "ocrpt-private.h"
#include "exprutil.h"
#include "layout.h"
#include "barcode.h"

/*
 * EAN / UPC related functions
 */

static int ocrpt_barcode_ean_csum(const char *barcode, size_t len, bool special) {
	int32_t even_sum = 0, odd_sum = 0;
	int32_t even = 1; /* last char is even */

	while (len-- > 0) {
		if (even)
			even_sum += barcode[len] - '0';
		else
			odd_sum += barcode[len] - '0';
		even = !even;
	}

	if (!special) {
		/* standard upc/ean checksum */
		len = (3 * even_sum + odd_sum) % 10;
		return (10 - len) % 10; /* complement to 10 */
	}

	/* add-5 checksum */
	len = (3 * even_sum + 9 * odd_sum);
	return len % 10;
}

static bool ocrpt_check_ean_addon(ocrpt_string *barcode, char **space, size_t *len, size_t *addon) {
	*space = strchr(barcode->str, ' ');

	if (*space) {
		size_t len0 = *space - barcode->str;
		size_t addon0 = barcode->len - len0 - 1;

		if (len)
			*len = len0;
		if (addon)
			*addon = addon0;

		if (addon0 != 2 && addon0 != 5)
			return false;

		for (len0++; barcode->str[len0]; len0++)
			if (!isdigit(barcode->str[len0]))
				return false;
	} else {
		if (len)
			*len = barcode->len;
		if (addon)
			*addon = 0;
	}

	return true;
}

static bool ocrpt_barcode_ean13_verify(ocrpt_string *barcode) {
	char *space = NULL;
	size_t len;

	if (!ocrpt_check_ean_addon(barcode, &space, &len, NULL))
		return false;

	for (int32_t i = 0; i < len; i++)
		if (!isdigit(barcode->str[i]))
			return false;

	switch (len) {
	case 13:
		if (barcode->str[12] != (ocrpt_barcode_ean_csum(barcode->str, 12, false) + '0'))
			return false;
		/* fall through */
		FALLTHROUGH;
	case 12:
		return true;
	default:
		return false;
	}
}

static bool ocrpt_barcode_ean8_verify(ocrpt_string *barcode) {
	char *space = NULL;
	size_t len;

	if (!ocrpt_check_ean_addon(barcode, &space, &len, NULL))
		return false;

	for (int32_t i = 0; i < len; i++)
		if (!isdigit(barcode->str[i]))
			return false;

	switch (len) {
	case 8:
		if (barcode->str[7] != (ocrpt_barcode_ean_csum(barcode->str, 7, false) + '0'))
			return false;
		/* fall through */
		FALLTHROUGH;
	case 7:
		return true;
	default:
		return false;
	}
}

/* Expand the middle part of UPC-E to UPC-A */
static char *ocrpt_barcode_upc_e_to_a0(const char *barcode) {
	char *result = ocrpt_mem_malloc(16);

	strcpy(result, "00000000000"); /* 11 0's */

	switch(barcode[5]) { /* last char */
	case '0':
	case '1':
	case '2':
		strncpy(result + 1, barcode, 2);
		result[3] = barcode[5]; /* Manufacturer */
		memcpy(result + 8, barcode + 2, 3); /* Product */
		break;
	case '3':
		memcpy(result + 1, barcode, 3); /* Manufacturer */
		memcpy(result + 9, barcode + 3, 2); /* Product */
		break;
	case '4':
		memcpy(result + 1,  barcode, 4); /* Manufacturer */
		memcpy(result + 10, barcode + 4, 1); /* Product */
		break;
	default:
		memcpy(result + 1,  barcode, 5); /* Manufacturer */
		memcpy(result + 10, barcode + 5, 1); /* Product */
		break;
	}

	return result;
}

static char *ocrpt_barcode_upc_e_to_a(const char *barcode, size_t len) {
	char *result;
	int32_t chk;

	switch (len) {
	case 6:
		return ocrpt_barcode_upc_e_to_a0(barcode);
	case 7:
		/*
		 * the first char is '0' or '1':
		 * valid number system for UPC-E and no checksum
		 */
		if (barcode[0] == '0' || barcode[0] == '1') {
			result = ocrpt_barcode_upc_e_to_a0(barcode + 1);
			result[0] = barcode[0];
			return result;
		}

		/* Find out whether the 7th char is correct checksum */
		result = ocrpt_barcode_upc_e_to_a0(barcode);
		chk = ocrpt_barcode_ean_csum(result, 6, false);

		if (chk == (barcode[len - 1] - '0'))
			return result;

		/* Invalid 7 digit representation for UPC-E. */
		ocrpt_mem_free(result);
		return NULL;
	case 8:
		if (barcode[0] == '0' || barcode[0] == '1') {
			result = ocrpt_barcode_upc_e_to_a0(barcode + 1);
			result[0] = barcode[0];
			chk = ocrpt_barcode_ean_csum(result, 7, false);
			if (chk == (barcode[len - 1] - '0'))
				return result;

			ocrpt_mem_free(result);
		}
		/* fall through */
		FALLTHROUGH;
	default:
		/* Invalid representation for UPC-E. */
		return NULL;
	}
}

static bool ocrpt_barcode_upca_verify(ocrpt_string *barcode) {
	char *space;
	size_t len;

	if (!ocrpt_check_ean_addon(barcode, &space, &len, NULL))
		return false;

	for (int32_t i = 0; i < len; i++)
		if (!isdigit(barcode->str[i]))
			return false;

	switch (len) {
	case 12:
		if (barcode->str[11] != (ocrpt_barcode_ean_csum(barcode->str, 11, 0) + '0'))
			return false;
		/* fall through */
		FALLTHROUGH;
	case 11:
		return true;;
	default:
		return false;
	}
}

static bool ocrpt_barcode_upce_verify(ocrpt_string *barcode) {
	char *space, *result;
	size_t len;

	if (!ocrpt_check_ean_addon(barcode, &space, &len, NULL))
		return false;

	for (int32_t i = 0; i < len; i++)
		if (!isdigit(barcode->str[i]))
			return false;

	switch (len) {
	case 6:
	case 7:
	case 8:
		result = ocrpt_barcode_upc_e_to_a(barcode->str, len);
		if (!result)
			return false;

		ocrpt_mem_free(result);
		return true;
	default:
		return false;
	}
}

static bool ocrpt_barcode_isbn_verify(ocrpt_string *barcode) {
	int i, ndigit = 0;

	for (i = 0; i < barcode->len && barcode->str[i]; i++) {
		if (barcode->str[i] == '-')
			continue;

		if (!isdigit(barcode->str[i]))
			return false;

		ndigit++;

		if (ndigit == 9) { /* got it all */
			i++;
			break;
		}
	}

	if (ndigit != 9)
		return false; /* too short */

	/* skip an hyphen, if any */
	if (barcode->str[i] == '-')
		i++;

	/* accept one more char if any (the checksum) */
	if (isdigit(barcode->str[i]) || toupper(barcode->str[i])=='X')
		i++;

	if (!barcode->str[i])
		return true;

	/* also accept the extra price tag (blank + 5 digits), if any */
	if (strlen(barcode->str + i) != 6)
		return false;
	if (barcode->str[i] != ' ')
		return false;

	for (i++; i < barcode->len && barcode->str[i]; i++)
		if (!isdigit(barcode->str[i]))
			return false;

	return true; /* isbn + 5-digit addon */
}

static ocrpt_string eanupc_digits[] = {
	{ .str = "3211", .len = 4 }, { .str = "2221", .len = 4 },
	{ .str = "2122", .len = 4 }, { .str = "1411", .len = 4 },
	{ .str = "1132", .len = 4 }, { .str = "1231", .len = 4 },
	{ .str = "1114", .len = 4 }, { .str = "1312", .len = 4 },
	{ .str = "1213", .len = 4 }, { .str = "3112", .len = 4 }
};

static ocrpt_string ean_mirrortab[] = {
	{ .str = "------", .len = 6 }, { .str = "--1-11", .len = 6 },
	{ .str = "--11-1", .len = 6 }, { .str = "--111-", .len = 6 },
	{ .str = "-1--11", .len = 6 }, { .str = "-11--1", .len = 6 },
	{ .str = "-111--", .len = 6 }, { .str = "-1-1-1", .len = 6 },
	{ .str = "-1-11-", .len = 6 }, { .str = "-11-1-", .len = 6 }
};

static ocrpt_string upc_mirrortab[] = {
	{ .str = "---111", .len = 6 }, { .str = "--1-11", .len = 6 },
	{ .str = "--11-1", .len = 6 }, { .str = "--111-", .len = 6 },
	{ .str = "-1--11", .len = 6 }, { .str = "-11--1", .len = 6 },
	{ .str = "-111--", .len = 6 }, { .str = "-1-1-1", .len = 6 },
	{ .str = "-1-11-", .len = 6 }, { .str = "-11-1-", .len = 6 }
};

static ocrpt_string upc_mirrortab1[] = {
	{ .str = "111---", .len = 6 }, { .str = "11-1--", .len = 6 },
	{ .str = "11--1-", .len = 6 }, { .str = "11---1", .len = 6 },
	{ .str = "1-11--", .len = 6 }, { .str = "1--11-", .len = 6 },
	{ .str = "1---11", .len = 6 }, { .str = "1-1-1-", .len = 6 },
	{ .str = "1-1--1", .len = 6 }, { .str = "1--1-1", .len = 6 }
};

static ocrpt_string upc_mirrortab2[] = {
	{ .str = "11", .len = 2 }, { .str = "1-", .len = 2 },
	{ .str = "-1", .len = 2 }, { .str = "--", .len = 2 }
};

static ocrpt_string eanupc_guard[] = {
	{ .str = "0a1a", .len = 4 }, { .str = "1a1a1", .len = 5 },
	{ .str = "a1a", .len = 3 }
};

static ocrpt_string upce_guard[] = {
	{ .str = "0a1a", .len = 4 }, { .str = "1a1a1a", .len = 6 }
};

static ocrpt_string eanupc_supp_guard[] = {
	{ .str = "9112", .len = 4 }, { .str = "11", .len = 2 }
};

/*
 * Accept a 11 or 12 digit UPC-A barcode and
 * shrink it into an 8-digit UPC-E equivalent if possible.
 * Return NULL if impossible, the UPC-E barcode if possible.
 */
static char *ocrpt_barcode_upc_a_to_e(char *barcode, size_t len) {
	static char	result[16];
	int chksum;

	switch (len) {
	case 12:
		strcpy(result, barcode);
		result[11] = '\0';
		chksum = ocrpt_barcode_ean_csum(result, len, false);
		if (barcode[11] != (chksum - '0'))
			return NULL;
		break;
	case 11:
		chksum = ocrpt_barcode_ean_csum(barcode, len, false);
		break;
	default:
		return NULL;
	}

	strcpy(result, "00000000"); /* 8 0's*/

	/* UPC-E can only be used with number system 0 or 1 */
	if (barcode[0] != '0' && barcode[0] != '1')
		return NULL;

	result[0] = barcode[0];

	if ((barcode[3] == '0' || barcode[3] == '1' || barcode[3] == '2')
		&& !strncmp(barcode+4, "0000", 4)) {
		memcpy(&result[1], barcode+1, 2);
		memcpy(&result[3], barcode+8, 3);
		result[6] = barcode[3];
	} else if (!strncmp(barcode+4, "00000", 5)) {
		memcpy(&result[1], barcode+1, 3);
		memcpy(&result[4], barcode+9, 2);
		result[6] = '3';
	} else if (!strncmp(barcode+5, "00000", 5)) {
		memcpy(&result[1], barcode+1, 4);
		result[5] = barcode[10];
		result[6] = '4';
	} else if ((barcode[5] != '0') && !strncmp(barcode+6, "0000", 4)
		&& barcode[10] >= '5' && barcode[10] <= '9') {
		memcpy(&result[1], barcode+1, 5);
		result[6] = barcode[10];
	} else {
		return NULL;
	}

	result[7] = chksum + '0';

	return result;
}

static size_t width_of_encoded(char *encoded, size_t len) {
	size_t i, w;

	for (i = 0, w = 0; i < len; i++) {
		if (isdigit(encoded[i]))
			w += encoded[i] - '0';
		else if (islower(encoded[i]))
			w += encoded[i] - 'a' + 1;
	}

	return w;
}

static void ocrpt_barcode_ean_encode(opencreport *o, ocrpt_barcode *bc, ocrpt_string *barcode, const char *type) {
	char text[24];
	char *space;
	ocrpt_string *mirror;
	size_t addon = 0, len;
	int i, checksum;
	bool is_upca = (strcmp(type, "upc-a") == 0);
	bool is_upce = (strcmp(type, "upc-e") == 0);
	bool is_ean13 = (strcmp(type, "ean-13") == 0);
	bool is_isbn = (strcmp(type, "isbn") == 0);

	if (!ocrpt_check_ean_addon(barcode, &space, &len, &addon))
		return;

	ocrpt_string *encoded = ocrpt_mem_string_resize(bc->encoded, 256);
	if (encoded) {
		if (!bc->encoded)
			bc->encoded = encoded;
		encoded->len = 0;
	} else
		return;

	int extralen = 0;
	if (is_upca) {
		/* add the leading 0 */
		text[0] = '0';
		strcpy(text + 1, barcode->str);
		extralen = 1;
	} else if (is_upce) {
		char *e_to_a = ocrpt_barcode_upc_e_to_a(barcode->str, len);
		char *a_to_e = ocrpt_barcode_upc_a_to_e(e_to_a, strlen(e_to_a));

		strcpy(text, a_to_e);
		len = strlen(text);

		ocrpt_mem_free(a_to_e);
		ocrpt_mem_free(e_to_a);
	} else
		strcpy(text, barcode->str);

	/* Build the checksum and the bars */
	if (is_upca || is_ean13 || is_isbn) {
		if (!(is_upca && len == 12) && !(is_ean13 && len == 13)) {
			checksum = ocrpt_barcode_ean_csum(text, len + extralen, false);
			text[12] = '0' + checksum; /* add it to the text */
			text[13] = '\0';
		}

		ocrpt_mem_string_append_len(encoded, eanupc_guard[0].str, eanupc_guard[0].len);
		if (is_ean13 || is_isbn)
			encoded->str[0] = '9'; /* extra space for the digit */
		else if (is_upca)
			encoded->str[0] = '9'; /* UPC has one digit before the symbol, too */

		mirror = &ean_mirrortab[text[0] - '0'];

		/* left part */
		for (i = 1; i < 7; i++) {
			ocrpt_string *swl = &eanupc_digits[text[i] - '0'];
			size_t prev_len = encoded->len;

			if (mirror->str[i - 1] == '1') {
				/* mirror this */
				for (size_t l = swl->len; l > 0; l--)
					ocrpt_mem_string_append_c(encoded, swl->str[l - 1]);
			} else
				ocrpt_mem_string_append_len(encoded, swl->str, swl->len);
			/*
			 * Write the ascii digit. UPC has a special case
			 * for the first digit, which is out of the bars
			 */
			if (is_upca && i == 1) {
				/* bars are long */
				encoded->str[prev_len + 1] += 'a' - '1';
				encoded->str[prev_len + 3] += 'a' - '1';
			}
		}

		ocrpt_mem_string_append_len(encoded, eanupc_guard[1].str, eanupc_guard[1].len);

		/* right part */
		for (i = 7; i < 13; i++) {
			ocrpt_string *swl = &eanupc_digits[text[i] - '0'];
			size_t prev_len = encoded->len;

			ocrpt_mem_string_append_len(encoded, swl->str, swl->len);

			if (is_upca && i == 12) {
				/* bars are long */
				encoded->str[prev_len + 0] += 'a' - '1';
				encoded->str[prev_len + 2] += 'a' - '1';
			}
		}

		ocrpt_mem_string_append_len(encoded, eanupc_guard[2].str, eanupc_guard[2].len); /* end */
	} else if (is_upce) {
		checksum = text[7] - '0';

		ocrpt_mem_string_append_len(encoded, upce_guard[0].str, upce_guard[0].len);
		encoded->str[0] = '9'; /* UPC-A has one digit before the symbol, too */

		mirror = (text[0] == '0' ? &upc_mirrortab[checksum] : &upc_mirrortab1[checksum]);

		size_t prev_len;
		for (i = 0; i < 6; i++) {
			ocrpt_string *swl = &eanupc_digits[text[i + 1] - '0'];
			prev_len = encoded->len;

			if (mirror->str[i] != '1') {
				/* negated wrt EAN13, mirror this */
				for (size_t l = swl->len; l > 0; l--)
					ocrpt_mem_string_append_c(encoded, swl->str[l - 1]);
			} else
				ocrpt_mem_string_append_len(encoded, swl->str, swl->len);
		}

		/* bars are long */
		encoded->str[prev_len + 0] += 'a' - '1';
		encoded->str[prev_len + 2] += 'a' - '1';

		/* end */
		ocrpt_mem_string_append_len(encoded, upce_guard[1].str, upce_guard[1].len);
	} else {
		/* EAN-8  almost identical to EAN-13 but no mirroring */
		if (len != 8) {
			checksum = ocrpt_barcode_ean_csum(text, len, false);
			text[7] = '0' + checksum; /* add it to the text */
			text[8] = '\0';
		}

		ocrpt_mem_string_append_len(encoded, eanupc_guard[0].str, eanupc_guard[0].len);

		/* left part */
		for (i = 0; i < 4; i++) {
			ocrpt_string *swl = &eanupc_digits[text[i] - '0'];
			ocrpt_mem_string_append_len(encoded, swl->str, swl->len);
		}

		ocrpt_mem_string_append_len(encoded, eanupc_guard[1].str, eanupc_guard[1].len); /* middle */

		/* right part */
		for (i = 4; i < 8; i++) {
			ocrpt_string *swl = &eanupc_digits[text[i] - '0'];
			ocrpt_mem_string_append_len(encoded, swl->str, swl->len);
		}

		ocrpt_mem_string_append_len(encoded, eanupc_guard[2].str, eanupc_guard[2].len);
	}

	/*
	 * if an some add-on is specified, it must be encoded.
	 */
	if (space) {
		space++;
		strcpy(text, space);

		const char *mirrorstr;
		if (addon == 5) {
			checksum = ocrpt_barcode_ean_csum(text, len, true /* special way */);
			mirror = &upc_mirrortab[checksum];
			mirrorstr = mirror->str + 1; /* only last 5 digits */
		} else {
			checksum = atoi(text) % 4;
			mirror = &upc_mirrortab2[checksum];
			mirrorstr = mirror->str;
		}

		ocrpt_mem_string_append_c(encoded, '+');
		for (i = 0; i < strlen(text); i++) {
			if (!i)
				ocrpt_mem_string_append_len(encoded, eanupc_supp_guard[0].str, eanupc_supp_guard[0].len); /* separation and head */
			else
				ocrpt_mem_string_append_len(encoded, eanupc_supp_guard[1].str, eanupc_supp_guard[1].len);

			ocrpt_string *swl = &eanupc_digits[text[i] - '0'];
			if (mirrorstr[i] != '1') {
				/* negated wrt EAN13 */
				/* mirror this */
				for (size_t l = swl->len; l > 0; l--)
					ocrpt_mem_string_append_c(encoded, swl->str[l - 1]);
			} else
				ocrpt_mem_string_append_len(encoded, swl->str, swl->len);
		}
	}
}

static void ocrpt_barcode_isbn_encode(opencreport *o, ocrpt_barcode *bc, ocrpt_string *barcode, const char *type) {
	ocrpt_string *ean = ocrpt_mem_string_resize(bc->priv, 24);
	if (ean) {
		if (!bc->priv)
			bc->priv = ean;
		ean->len = 0;
	} else
		return;

	/* For ISBN, the string must be normalized and prefixed with "978" */
	ocrpt_mem_string_append_len(ean, "978", 3);

	size_t i;
	for (i = 0; i < barcode->len && barcode->str[i]; i++) {
		if (isdigit(barcode->str[i]))
			ocrpt_mem_string_append_c(ean, barcode->str[i]);

		if (ean->len == 12) /* checksum added later */
			break;
	}

	const char *space = strchr(barcode->str + i, ' ');
	if (space)
		ocrpt_mem_string_append_printf(ean, "%s", space);

	ocrpt_barcode_ean_encode(o, bc, ean, type);
}

/*
 * CODE39 related functions
 */

static char code39_alphabet[] =
	"1234567890" "ABCDEFGHIJ" "KLMNOPQRST" "UVWXYZ-. *" "$/+%";

/* the checksum alphabet has a different order, without '*' */
static char code39_alphabet_4csum[] =
	"0123456789" "ABCDEFGHIJ" "KLMNOPQRST" "UVWXYZ-. $" "/+%";

/* The first 40 symbols repeat this bar pattern */
static char *code39_bars[] = {
	"31113", "13113", "33111", "11313", "31311",
	"13311", "11133", "31131", "13131", "11331"
};

/* The first 4 decades use these space patterns */
static char *code39_spaces[] = {
	"1311", "1131", "1113", "3111"
};

/* the last four symbols are special */
static char *code39_special_bars[] = {
	"11111", "11111", "11111", "11111"
};

static char *code39_special_spaces[] = {
	"3331", "3313", "3133", "1333"
};

static ocrpt_string code39_fillers[] = {
	{ .str = "0a3a1c1c1a", .len = 10 },
	{ .str = "1a3a1c1c1a", .len = 10 },
};

/* extended code39 translation table */
static char *code39ext[] = {
	"%U", /* for completeness only, NUL cannot be encoded by barcode */
	"$A","$B","$C","$D","$E","$F","$G","$H","$I","$J","$K","$L","$M",
	"$N","$O","$P","$Q","$R","$S","$T","$U","$V","$W","$X","$Y","$Z",
	"%A","%B","%C","%D","%E"," ",
	"/A","/B","/C","/D","/E","/F","/G","/H","/I","/J","/K","/L","-",
	".","/O","0","1","2","3","4","5","6","7","8","9","/Z",
	"%F","%G","%H","%I","%J","%V",
	"A","B","C","D","E","F","G","H","I","J","K","L","M",
	"N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
	"%K","%L","%M","%N","%O","%W",
	"+A","+B","+C","+D","+E","+F","+G","+H","+I","+J","+K","+L","+M",
	"+N","+O","+P","+Q","+R","+S","+T","+U","+V","+W","+X","+Y","+Z",
	"%P","%Q","%R","%S","%T"
};

static bool ocrpt_barcode_code39_verify(ocrpt_string *barcode) {
	int i, lower = 0, upper = 0;

	for (i = 0; i < barcode->len; i++) {
		char c = barcode->str[i];

		if (isupper(c))
			upper++;
		if (islower(c))
			lower++;
		if (!strchr(code39_alphabet, toupper(c)))
			return false;
	}

	if (lower && upper)
		return false;

	return true;
}

static bool ocrpt_barcode_code39ext_verify(ocrpt_string *barcode) {
	int i;

	for (i = 0; i < barcode->len; i++)
		if (barcode->str[i] <= 0)
			return false;

	return true;
}

static void ocrpt_barcode_code39_add_code(ocrpt_string *encoded, int code) {
	char *b, *s;

	if (code < 40) {
		b = code39_bars[code % 10];
		s = code39_spaces[code / 10];
	} else {
		b = code39_special_bars[code - 40];
		s = code39_special_spaces[code - 40];
	}

	ocrpt_mem_string_append_printf(encoded, "1%c%c%c%c%c%c%c%c%c",
		b[0], s[0], b[1], s[1], b[2], s[2], b[3], s[3], b[4]);
}

static void ocrpt_barcode_code39_encode(opencreport *o, ocrpt_barcode *bc, ocrpt_string *barcode, const char *type) {
	ocrpt_string *encoded = ocrpt_mem_string_resize(bc->encoded, (barcode->len + 3) * 10 + 2);
	if (encoded) {
		if (!bc->encoded)
			bc->encoded = encoded;
		encoded->len = 0;
	} else
		return;

	ocrpt_mem_string_append_len(encoded, code39_fillers[0].str, code39_fillers[0].len);

	int code, checksum = 0;

	for (size_t i = 0; i < barcode->len; i++) {
		char *c = strchr(code39_alphabet, toupper(barcode->str[i]));

		if (!c) {
			encoded->len = 0;
			return;
		}

		code = c - code39_alphabet;
		ocrpt_barcode_code39_add_code(encoded, code);

		c = strchr(code39_alphabet_4csum, *c);
		if (c) /* the '*' is not there */
			checksum += (c - code39_alphabet_4csum);
	}

	/* Add the checksum */
	code = (strchr(code39_alphabet, code39_alphabet_4csum[checksum % 43]) - code39_alphabet);
	ocrpt_barcode_code39_add_code(encoded, code);

	ocrpt_mem_string_append_len(encoded, code39_fillers[1].str, code39_fillers[1].len); /* end */
}

/*
 * The encoding functions fills the "partial" and "textinfo" fields.
 * Replace the ascii with extended coding and call 39_encode
 */
static void ocrpt_barcode_code39ext_encode(opencreport *o, ocrpt_barcode *bc, ocrpt_string *barcode, const char *type) {
	/* worst case 2 chars per original text */
	ocrpt_string *new_barcode = ocrpt_mem_string_resize(bc->priv, barcode->len * 2 + 1);
	if (new_barcode) {
		if (!bc->priv)
			bc->priv = new_barcode;
		new_barcode->len = 0;
	} else
		return;

	for (size_t i = 0; i < barcode->len; i++)
		ocrpt_mem_string_append_printf(new_barcode, code39ext[(unsigned char)barcode->str[i]]);

	ocrpt_barcode_code39_encode(o, bc, new_barcode, type);
}

/*
 * CODE128 related functions
 */

static const ocrpt_string code128_codeset[] = {
	{ .str = "212222", .len = 6 }, { .str = "222122", .len = 6 }, { .str = "222221", .len = 6 }, { .str = "121223", .len = 6 }, { .str = "121322", .len = 6 }, /*   0 -   4 */
	{ .str = "131222", .len = 6 }, { .str = "122213", .len = 6 }, { .str = "122312", .len = 6 }, { .str = "132212", .len = 6 }, { .str = "221213", .len = 6 }, /*   5 -   9 */
	{ .str = "221312", .len = 6 }, { .str = "231212", .len = 6 }, { .str = "112232", .len = 6 }, { .str = "122132", .len = 6 }, { .str = "122231", .len = 6 }, /*  10 -  14 */
	{ .str = "113222", .len = 6 }, { .str = "123122", .len = 6 }, { .str = "123221", .len = 6 }, { .str = "223211", .len = 6 }, { .str = "221132", .len = 6 }, /*  15 -  19 */
	{ .str = "221231", .len = 6 }, { .str = "213212", .len = 6 }, { .str = "223112", .len = 6 }, { .str = "312131", .len = 6 }, { .str = "311222", .len = 6 }, /*  20 -  24 */
	{ .str = "321122", .len = 6 }, { .str = "321221", .len = 6 }, { .str = "312212", .len = 6 }, { .str = "322112", .len = 6 }, { .str = "322211", .len = 6 }, /*  25 -  29 */
	{ .str = "212123", .len = 6 }, { .str = "212321", .len = 6 }, { .str = "232121", .len = 6 }, { .str = "111323", .len = 6 }, { .str = "131123", .len = 6 }, /*  30 -  34 */
	{ .str = "131321", .len = 6 }, { .str = "112313", .len = 6 }, { .str = "132113", .len = 6 }, { .str = "132311", .len = 6 }, { .str = "211313", .len = 6 }, /*  35 -  39 */
	{ .str = "231113", .len = 6 }, { .str = "231311", .len = 6 }, { .str = "112133", .len = 6 }, { .str = "112331", .len = 6 }, { .str = "132131", .len = 6 }, /*  40 -  44 */
	{ .str = "113123", .len = 6 }, { .str = "113321", .len = 6 }, { .str = "133121", .len = 6 }, { .str = "313121", .len = 6 }, { .str = "211331", .len = 6 }, /*  45 -  49 */
	{ .str = "231131", .len = 6 }, { .str = "213113", .len = 6 }, { .str = "213311", .len = 6 }, { .str = "213131", .len = 6 }, { .str = "311123", .len = 6 }, /*  50 -  54 */
	{ .str = "311321", .len = 6 }, { .str = "331121", .len = 6 }, { .str = "312113", .len = 6 }, { .str = "312311", .len = 6 }, { .str = "332111", .len = 6 }, /*  55 -  59 */
	{ .str = "314111", .len = 6 }, { .str = "221411", .len = 6 }, { .str = "431111", .len = 6 }, { .str = "111224", .len = 6 }, { .str = "111422", .len = 6 }, /*  60 -  64 */
	{ .str = "121124", .len = 6 }, { .str = "121421", .len = 6 }, { .str = "141122", .len = 6 }, { .str = "141221", .len = 6 }, { .str = "112214", .len = 6 }, /*  65 -  69 */
	{ .str = "112412", .len = 6 }, { .str = "122114", .len = 6 }, { .str = "122411", .len = 6 }, { .str = "142112", .len = 6 }, { .str = "142211", .len = 6 }, /*  70 -  74 */
	{ .str = "241211", .len = 6 }, { .str = "221114", .len = 6 }, { .str = "413111", .len = 6 }, { .str = "241112", .len = 6 }, { .str = "134111", .len = 6 }, /*  75 -  79 */
	{ .str = "111242", .len = 6 }, { .str = "121142", .len = 6 }, { .str = "121241", .len = 6 }, { .str = "114212", .len = 6 }, { .str = "124112", .len = 6 }, /*  80 -  84 */
	{ .str = "124211", .len = 6 }, { .str = "411212", .len = 6 }, { .str = "421112", .len = 6 }, { .str = "421211", .len = 6 }, { .str = "212141", .len = 6 }, /*  85 -  89 */
	{ .str = "214121", .len = 6 }, { .str = "412121", .len = 6 }, { .str = "111143", .len = 6 }, { .str = "111341", .len = 6 }, { .str = "131141", .len = 6 }, /*  90 -  94 */
	{ .str = "114113", .len = 6 }, { .str = "114311", .len = 6 }, { .str = "411113", .len = 6 }, { .str = "411311", .len = 6 }, { .str = "113141", .len = 6 }, /*  95 -  99 */
	{ .str = "114131", .len = 6 }, { .str = "311141", .len = 6 }, { .str = "411131", .len = 6 }, { .str = "b1a4a2", .len = 6 }, { .str = "b1a2a4", .len = 6 }, /* 100 - 104 */
	{ .str = "b1a2c2", .len = 6 }, { .str = "b3c1a1b", .len = 7 }
};

#define START_A 103
#define START_B 104
#define START_C 105
#define STOP 106
#define SHIFT 98 /* only A and B */
#define CODE_A 101 /* only B and C */
#define CODE_B 100 /* only A and C */
#define CODE_C 99 /* only A and B */
#define FUNC_1 102 /* all of them */
#define FUNC_2 97 /* only A and B */
#define FUNC_3 96 /* only A and B */
/* FUNC_4 is CODE_A when in A and CODE_B when in B */

/* code128b includes all printable ascii chars */

static bool ocrpt_barcode_code128b_verify(ocrpt_string *barcode) {
	for (size_t i = 0; i < barcode->len; i++)
		if (barcode->str[i] < 32 || (barcode->str[i] & 0x80) == 0x80) /* signed char check */
			return false;

	return true;
}

static void ocrpt_barcode_code128b_encode(opencreport *o, ocrpt_barcode *bc, ocrpt_string *barcode, const char *type) {
	/* the encoded code is 6 * (head + text + check + tail) + final + term. */
	ocrpt_string *encoded = ocrpt_mem_string_resize(bc->encoded, (barcode->len + 4) * 6 + 2);
	if (encoded) {
		if (!bc->encoded)
			bc->encoded = encoded;
		encoded->len = 0;
	} else
		return;

	int checksum = 0;

	ocrpt_mem_string_append_c(encoded, '0'); /* the first space */
	ocrpt_mem_string_append_len(encoded, code128_codeset[START_B].str, code128_codeset[START_B].len);
	checksum += START_B; /* the start char is counted in the checksum */

	for (size_t i = 0; i < barcode->len; i++) {
		int code = barcode->str[i] - 32;

		ocrpt_mem_string_append_len(encoded, code128_codeset[code].str, code128_codeset[code].len);

		checksum += code * (i + 1); /* first * 1 + second * 2 + third * 3... */
	}

	checksum %= 103;
	ocrpt_mem_string_append_len(encoded, code128_codeset[checksum].str, code128_codeset[checksum].len);
	ocrpt_mem_string_append_len(encoded, code128_codeset[STOP].str, code128_codeset[STOP].len);
}

/*
 * code 128-c is only digits, but two per symbol
 */

static bool ocrpt_barcode_code128c_verify(ocrpt_string *barcode) {
	/* must be an even number of digits */
	if (barcode->len % 2)
		return false;
	/* and must be all digits */
	for (size_t i = 0; i < barcode->len; i++)
		if (!isdigit(barcode->str[i]))
			return false;

	return true;
}

static void ocrpt_barcode_code128c_encode(opencreport *o, ocrpt_barcode *bc, ocrpt_string *barcode, const char *type) {
	/* the encoded code is 6 * (head + text + check + tail) + final + term. */
	ocrpt_string *encoded = ocrpt_mem_string_resize(bc->encoded, (barcode->len + 3) * 6 + 2);
	if (encoded) {
		if (!bc->encoded)
			bc->encoded = encoded;
		encoded->len = 0;
	} else
		return;

	int checksum = 0;

	ocrpt_mem_string_append_c(encoded, '0'); /* the first space */
	ocrpt_mem_string_append_len(encoded, code128_codeset[START_C].str, code128_codeset[START_C].len);
	checksum += START_C; /* the start char is counted in the checksum */

	for (size_t i = 0; i < barcode->len; i += 2) {
		int code = (barcode->str[i] - '0') * 10 + barcode->str[i + 1] - '0';

		ocrpt_mem_string_append_len(encoded, code128_codeset[code].str, code128_codeset[code].len);

		checksum += code * (i / 2 + 1); /* first * 1 + second * 2 + third * 3... */
	}

	checksum %= 103;
	ocrpt_mem_string_append_len(encoded, code128_codeset[checksum].str, code128_codeset[checksum].len);
	ocrpt_mem_string_append_len(encoded, code128_codeset[STOP].str, code128_codeset[STOP].len);
}

/*
 * generic (full-featured) code128 implementation: it selects between
 * A, B, C according to the data being encoded. F1, F2, F3, F4 are expressed
 * using ascii chars 0xc1, 0xc2, 0xc3, 0xc4 (0301, 0302, 0303, 0304).
 * Char '\0' is expressed by 0x80 (0200).
 */

static bool ocrpt_barcode_code128_verify(ocrpt_string *barcode) {
	for (size_t i = 0; i < barcode->len; i++) {
		unsigned char c = (unsigned char)barcode->str[i];

		if ((c > 0x80 && (c < 0xc1 || c > 0xc4)))
			return false;
	}

	return true;
}


/*
 * These functions are extracted from Barcode_128_encode for clarity.
 * It deals with choosing the symbols used to represent the text
 * and returns a dynamic array of integers, terminated by -1.
 *
 * The algorithm used in choosing the codes comes from App 2 of
 * "El Codigo Estandar EAN/UCC 128", courtesy of AECOC, Spain.
 * Thanks to Dani Pardo for getting permission and giving me a copy
 * of the document
 */

#define NEED_CODE_A(c) ((c) < 32 || (c) == 0x80) 
#define NEED_CODE_B(c) ((c) >= 96 && (c) < 128)

static int ocrpt_barcode_code128a_or_b(const char *barcode, size_t len) {
	for (size_t i = 0; i < len; i++) {
		if (NEED_CODE_A((unsigned char)barcode[i]))
			return 'A';
		if (NEED_CODE_B((unsigned char)barcode[i]))
			return 'B';
	}

	return 0; /* any */
}

/* code is either 'A' or 'B', and value must be valid */
static int ocrpt_barcode_code128_encode_as(int code, int value) {
	/* first check the special chars */
	if (value == 0xc1)
		return FUNC_1;
	if (value == 0xc2)
		return FUNC_2;
	if (value == 0xc3)
		return FUNC_3;
	if (value == 0xc4) {
		/* F4 */
		if (code == 'A')
			return CODE_A;
		return CODE_B;
	}

	/* then check ascii values */
	if (value >= 0x20 && value <= 0x5f)
		return value - 0x20; /* both codes */
	if (value == 0x80)
		return 64; /* code A */
	if (value < 0x20)
		return value + 64; /* code A */
	if (value >= 0x60)
		return value - 0x20; /* code B */

	/* this is unreachable code */
	return -1;
}

#define ocrpt_mem_string_append_int(s, i) \
	{ \
		int32_t ii = (i); \
		ocrpt_mem_string_append_len_binary(s, (char *)&ii, sizeof(ii)); \
	}

static ocrpt_string *ocrpt_barcode_code128_make_array(ocrpt_barcode *bc, ocrpt_string *barcode) {
	/* allocate twice the text length + 5, as this is the worst case */
	ocrpt_string *codes = ocrpt_mem_string_resize(bc->priv, ((barcode->len * 2) + 5) * sizeof(int32_t));
	if (codes) {
		if (!bc->priv)
			bc->priv = codes;
		codes->len = 0;
	} else
		return NULL;

	/* choose the starting code */
	int32_t code;
	if (barcode->len == 2 && isdigit(barcode->str[0]) && isdigit(barcode->str[1]))
		code = 'C';
	else if (isdigit(barcode->str[0]) && isdigit(barcode->str[1]) && isdigit(barcode->str[2]) && isdigit(barcode->str[3]))
		code = 'C';
	else {
		code = ocrpt_barcode_code128a_or_b(barcode->str, barcode->len);
		if (!code)
			code = 'B'; /* default */
	}

	ocrpt_mem_string_append_int(codes, START_A + code - 'A');

	size_t i, j, rlen;
	for (i = 0, rlen = barcode->len; i < barcode->len; /* increments are in the loop */) {
		switch(code) {
		case 'C':
			if ((unsigned char)barcode->str[i] == 0xc1) { /* F1 is valid */
				ocrpt_mem_string_append_int(codes, FUNC_1);
				i++;
				rlen--;
			} else if (isdigit(barcode->str[i]) && isdigit(barcode->str[i + 1])) {
				/* encode two digits */
				ocrpt_mem_string_append_int(codes, (barcode->str[i] - '0') * 10 + barcode->str[i + 1] - '0');
				i += 2;
				rlen -= 2;
			} else {
				/* change code */
				code = ocrpt_barcode_code128a_or_b(barcode->str + i, rlen);
				if (!code)
					code = 'B';
				ocrpt_mem_string_append_int(codes, (code == 'A') ? CODE_A : CODE_B);
			}
			break;
		case 'B':
		case 'A':
			for (j = i; j < barcode->len && isdigit(barcode->str[j]); j++)
				;
			j -= i;
			if (j >= 4) { /* if there are 4 or more digits, turn to C */
				if (j & 1) {
					/* odd number: encode one first */
					ocrpt_mem_string_append_int(codes, barcode->str[i] - ' ');
					i++;
					rlen--;
				}
				ocrpt_mem_string_append_int(codes, CODE_C);
				code = 'C';
			} else if (code == 'A' && NEED_CODE_B((unsigned char)barcode->str[i])) {
				/* check whether we should use SHIFT or change code */
				j = ocrpt_barcode_code128a_or_b(barcode->str + i + 1, rlen - 1);
				if (j == 'B') {
					ocrpt_mem_string_append_int(codes, CODE_B);
					code = 'B';
				} else {
					ocrpt_mem_string_append_int(codes, SHIFT);
					ocrpt_mem_string_append_int(codes, ocrpt_barcode_code128_encode_as('B', barcode->str[i]));
					i++;
					rlen--;
				}
			} else if (code == 'B' && NEED_CODE_A((unsigned char)barcode->str[i])) {
				/* check whether we should use SHIFT or change code */
				j = ocrpt_barcode_code128a_or_b(barcode->str + i + 1, rlen - 1);
				if (j == 'A') {
					ocrpt_mem_string_append_int(codes, CODE_A);
					code = 'A';
				} else {
					ocrpt_mem_string_append_int(codes, SHIFT);
					ocrpt_mem_string_append_int(codes, ocrpt_barcode_code128_encode_as('A', barcode->str[i]));
					i++;
					rlen--;
				}
			} else {
				ocrpt_mem_string_append_int(codes, ocrpt_barcode_code128_encode_as(code, barcode->str[i]));
				i++;
				rlen--;
			}
			break;
		}		
	}

	/* add the checksum */
	int32_t *code_arr = (int32_t *)codes->str;
	int32_t checksum = code_arr[0];

	for (j = 1, i = codes->len / 4; j < i; j++)
		checksum += j * code_arr[j];

	checksum %= 103;

	ocrpt_mem_string_append_int(codes, checksum);
	ocrpt_mem_string_append_int(codes, STOP);

	return codes;
}

static void ocrpt_barcode_code128_encode(opencreport *o, ocrpt_barcode *bc, ocrpt_string *barcode, const char *type) {
	ocrpt_string *codes = ocrpt_barcode_code128_make_array(bc, barcode);
	if (!codes) {
		if (bc->encoded)
			bc->encoded->len = 0;
		return;
	}

	int32_t *code_arr = (int32_t *)codes->str;
	size_t len = codes->len / sizeof(int32_t);

	/* the encoded string is 6 * codelen + ini + term (+margin) */
	ocrpt_string *encoded = ocrpt_mem_string_resize(bc->encoded, 6 * len + 4);
	if (encoded) {
		if (!bc->encoded)
			bc->encoded = encoded;
		encoded->len = 0;
	} else
		return;

	ocrpt_mem_string_append_c(encoded, '0'); /* the first space */

	size_t i;
	for (i = 0; i < len; i++) {
		int code = code_arr[i];

		ocrpt_mem_string_append_len(encoded, code128_codeset[code].str, code128_codeset[code].len);
	}

	/* avoid bars that fall lower than other bars */
	for (i = 0; i < encoded->len; i++)
		if (isalpha(encoded->str[i]))
			encoded->str[i] += '1' - 'a';
}

/*
 * Interface to layout code
 */

typedef bool (* ocrpt_barcode_verify_func)(ocrpt_string *);
typedef void (* ocrpt_barcode_encode_func)(opencreport *, ocrpt_barcode *, ocrpt_string *, const char *);

struct ocrpt_barcode_funcs {
	const char *type;
	ocrpt_barcode_verify_func verify;
	ocrpt_barcode_encode_func encode;
};

static struct ocrpt_barcode_funcs ocrpt_barcode_funcs[] = {
	{ "upc-a", ocrpt_barcode_upca_verify, ocrpt_barcode_ean_encode },
	{ "ean-13", ocrpt_barcode_ean13_verify, ocrpt_barcode_ean_encode },
	{ "upc-e", ocrpt_barcode_upce_verify, ocrpt_barcode_ean_encode },
	{ "ean-8", ocrpt_barcode_ean8_verify, ocrpt_barcode_ean_encode },
	{ "isbn", ocrpt_barcode_isbn_verify, ocrpt_barcode_isbn_encode },
	{ "code39", ocrpt_barcode_code39_verify, ocrpt_barcode_code39_encode },
	{ "code39ext", ocrpt_barcode_code39ext_verify, ocrpt_barcode_code39ext_encode },
	{ "code128b", ocrpt_barcode_code128b_verify, ocrpt_barcode_code128b_encode },
	{ "code128c", ocrpt_barcode_code128c_verify, ocrpt_barcode_code128c_encode },
	{ "code128", ocrpt_barcode_code128_verify, ocrpt_barcode_code128_encode },
	{ NULL, NULL, NULL }
};

static bool ocrpt_barcode_verify(ocrpt_string *barcode, ocrpt_string *type, int32_t *type_idx) {
	int32_t i;

	if (type) {
		for (i = 0; ocrpt_barcode_funcs[i].type; i++)
			if (strcasecmp(type->str, ocrpt_barcode_funcs[i].type) == 0) {
				if (type_idx)
					*type_idx = i;
				return ocrpt_barcode_funcs[i].verify(barcode);
			}

		return false;
	}

	for (i = 0; ocrpt_barcode_funcs[i].type; i++) {
		if (ocrpt_barcode_funcs[i].verify(barcode)) {
			if (type_idx)
				*type_idx = i;
			return true;
		}
	}

	return false;
}

bool ocrpt_barcode_encode(opencreport *o, ocrpt_barcode *barcode) {
	int32_t type_idx = -1;

	if (!barcode)
		return false;

	barcode->encoded_width = 0;

	ocrpt_string *b = NULL, *t = NULL;

	if (EXPR_VALID_STRING_MAYBE_UNINITIALIZED(barcode->value))
		b = EXPR_STRING(barcode->value);

	if (!b || !b->len)
		return false;

	if (EXPR_VALID_STRING_MAYBE_UNINITIALIZED(barcode->bctype))
		t = EXPR_STRING(barcode->bctype);

	if (!ocrpt_barcode_verify(b, (t && t->len > 0) ? t : NULL , &type_idx))
		return false;

	if (ocrpt_barcode_funcs[type_idx].encode)
		ocrpt_barcode_funcs[type_idx].encode(o, barcode, b, ocrpt_barcode_funcs[type_idx].type);

	if (barcode->encoded && barcode->encoded->len)
		barcode->encoded_width = width_of_encoded(barcode->encoded->str, barcode->encoded->len);

	return barcode->encoded_width > 0;
}
