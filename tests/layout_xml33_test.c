/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
//#include "test_common.h"

const char *array[121][6] = {
	{ "id", "name", "property", "age", "male", "adult" },
	{ "1", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "2", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "3", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "4", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "5", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "6", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "7", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "8", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "9", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "10", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "11", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "12", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "13", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "14", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "15", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "16", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "17", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "18", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "19", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "20", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "21", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "22", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "23", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "24", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "25", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "26", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "27", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "28", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "29", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "30", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "31", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "32", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "33", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "34", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "35", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "36", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "37", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "38", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "39", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "40", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "41", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "42", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "43", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "44", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "45", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "46", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "47", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "48", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "49", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "50", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "51", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "52", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "53", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "54", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "55", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "56", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "57", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "58", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "59", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "60", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "1", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "2", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "3", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "4", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "5", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "6", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "7", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "8", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "9", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "10", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "11", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "12", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "13", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "14", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "15", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "16", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "17", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "18", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "19", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "20", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "21", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "22", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "23", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "24", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "25", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "26", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "27", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "28", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "29", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "30", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "31", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "32", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "33", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "34", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "35", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "36", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "37", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "38", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "39", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "40", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "41", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "42", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "43", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "44", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "45", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "46", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "47", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "48", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "49", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "50", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "51", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "52", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "53", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "54", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
	{ "55", "Fred Flintstone", "strong", "31", "yes", "yes" },
	{ "56", "Barney Rubble", "small", "29", "yes", "yes" },
	{ "57", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" },
	{ "58", "Wilma Flintstone", "charming", "28", "no", "yes" },
	{ "59", "Betty Rubble", "beautiful", "27", "no", "yes" },
	{ "60", "Pebbles Flintstone", "young", "5e-1", "no", "no" },
};

const int32_t coltypes[6] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_xml33_test.xml")) {
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
