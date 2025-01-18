/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <opencreport.h>

const char *array[83][2] = {
	{ "type", "num" },
	{ "Acai berry", "75" },
	{ "Acerola", "25" },
	{ "Apple", "4" },
	{ "Appleberry", "32" },
	{ "Apricot", "6" },
	{ "Avocado", "6" },
	{ "Banana", "2" },
	{ "Blackberry", "1" },
	{ "Blackcurrant", "32" },
	{ "Black sapote", "3" },
	{ "Blood orange", "3" },
	{ "Blueberry", "15" },
	{ "Boysenberry", "35" },
	{ "Cantaloupe", "3" },
	{ "Carambola", "3" },
	{ "Chayote", "2" },
	{ "Cherimoya", "5" },
	{ "Cherry", "20" },
	{ "Coconut", "2" },
	{ "Cranberry", "35" },
	{ "Currant", "50" },
	{ "Date", "5" },
	{ "Dewberry", "50" },
	{ "Dragonfruit", "1" },
	{ "Durian", "1" },
	{ "Elderberry", "32" },
	{ "Feijoa", "15" },
	{ "Fig", "4" },
	{ "Gooseberry", "50" },
	{ "Grape", "5" },
	{ "Grapefruit", "1" },
	{ "Guava", "2" },
	{ "Honeydew", "6" },
	{ "Huckleberry", "40" },
	{ "Jackfruit", "4" },
	{ "Jambul", "4" },
	{ "Kiwi", "2" },
	{ "Key lime", "4" },
	{ "Kumquat", "30" },
	{ "Lemon", "3" },
	{ "Lime", "3" },
	{ "Longan", "3" },
	{ "Loquat", "3" },
	{ "Lychee", "20" },
	{ "Mandarin", "4" },
	{ "Mango", "2" },
	{ "Mamey", "2" },
	{ "Marula", "2" },
	{ "Medlar", "15" },
	{ "Mombin", "4" },
	{ "Mulberry", "40" },
	{ "Nectarine", "2" },
	{ "Orange", "5" },
	{ "Papaw", "3" },
	{ "Papaya", "3" },
	{ "Passion fruit", "5" },
	{ "Pawpaw", "2" },
	{ "Peach", "4" },
	{ "Pear", "3" },
	{ "Persimmon", "4" },
	{ "Pineapple", "1" },
	{ "Pineberry", "30" },
	{ "Plum", "35" },
	{ "Pomegranate", "2" },
	{ "Pomelo", "1" },
	{ "Quince", "5" },
	{ "Raspberry", "60" },
	{ "Redcurrant", "20" },
	{ "Rhubarb", "2" },
	{ "Sea buckthorn", "1" },
	{ "Serviceberry", "50" },
	{ "Soursop", "2" },
	{ "Starfruit", "1" },
	{ "Strawberry", "15" },
	{ "Tamarillo", "1" },
	{ "Tangerine", "3" },
	{ "Ugli fruit", "5" },
	{ "Watermelon", "2" },
	{ "White currant", "50" },
	{ "Yellow plum", "5" },
	{ "Yellow watermelon", "1" },
	{ "Zinfandel grape", "5" },
};

const int32_t coltypes[2] = {
	OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "page_header_footer_vars_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
