<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$LOREMIPSUM = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

$array = [
	[ "id", "name", "property", "age", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes" ],
	[ "2",
		"Wilma Flintstone\n" . $LOREMIPSUM . "\n" .
		"Wilma Flintstone\n" . $LOREMIPSUM . "\n" .
		"Wilma Flintstone\n" . $LOREMIPSUM . "\n" .
		"Wilma Flintstone\n" . $LOREMIPSUM . "\n" .
		"Wilma Flintstone\n" . $LOREMIPSUM . "\n" .
		"Wilma Flintstone\n" . $LOREMIPSUM . "\n" .
		"Wilma Flintstone\n" . $LOREMIPSUM,
		"charming", "28", "yes" ],
	[ "3", "Pebbles Flintstone", "young", "5e-1", "no" ],
	[ "4",
		"Betty Rubble\n" . $LOREMIPSUM . "\n" .
		"Betty Rubble\n" . $LOREMIPSUM . "\n" .
		"Betty Rubble\n" . $LOREMIPSUM . "\n" .
		"Betty Rubble\n" . $LOREMIPSUM . "\n" .
		"Betty Rubble\n" . $LOREMIPSUM . "\n" .
		"Betty Rubble\n" . $LOREMIPSUM . "\n" .
		"Betty Rubble\n" . $LOREMIPSUM,
		"beautiful", "27", "yes" ],
	[ "5", "Barney Rubble", "small", "29", "yes" ],
	[ "6", "Mr. George Slate", "grumpy", "53", "yes" ],
	[ "7", "Joe Rockhead", "friendly", "33", "yes" ],
	[ "8", "Sam Slagheap", "leader", "37", "yes" ],
	[ "9",
		"The Great Gazoo\n" . $LOREMIPSUM . "\n" .
		"The Great Gazoo\n" . $LOREMIPSUM . "\n" .
		"The Great Gazoo\n" . $LOREMIPSUM . "\n" .
		"The Great Gazoo\n" . $LOREMIPSUM . "\n" .
		"The Great Gazoo\n" . $LOREMIPSUM . "\n" .
		"The Great Gazoo\n" . $LOREMIPSUM . "\n" .
		"The Great Gazoo\n" . $LOREMIPSUM,
		"hostile alien", "1200", "yes" ],
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();
if (!$o->parse_xml("layout_xml36_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
