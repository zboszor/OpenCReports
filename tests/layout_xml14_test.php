<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "name", "property", "age", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes" ],
	[ "2", "Wilma Flintstone", "charming", "28", "yes" ],
	[ "3", "Pebbles Flintstone", "young", "5e-1", "no" ],
	[ "4", "Betty Rubble", "beautiful", "27", "yes" ],
	[ "5", "Barney Rubble", "small", "29", "yes" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$array2 = [
	[ "id", "name", "property", "age", "adult" ],
	[ "1", "Mr. George Slate", "grumpy", "53", "yes" ],
	[ "2", "Joe Rockhead", "friendly", "33", "yes" ],
	[ "3", "Sam Slagheap", "leader", "37", "yes" ],
	[ "4", "The Great Gazoo", "alien", "800", "yes" ],
];

$o = new OpenCReport();
if (!$o->parse_xml("layout_xml14_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
