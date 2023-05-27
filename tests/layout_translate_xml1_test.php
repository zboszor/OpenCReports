<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "name", "age", "adult"],
	[ "1", "Fred Flintstone", "31", "yes"],
	[ "2", "Wilma Flintstone ", "28", "yes"],
	[ "3", "Pebbles Flintstone", "5e-1", "no"],
	[ "4", "Betty Rubble ", "27", "yes"],
	[ "5", "Barney Rubble", "29", "yes"],
	[ "6", "Mr. George Slate", "53", "yes"],
	[ "7", "Joe Rockhead", "33", "yes"],
	[ "8", "Sam Slagheap", "37", "yes"],
	[ "9", "The Great Gazoo ", "1200", "yes"],
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();
if (!$o->parse_xml("layout_translate_xml1_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
