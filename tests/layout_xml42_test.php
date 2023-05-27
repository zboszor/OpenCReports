<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$LOREMIPSUM = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

$array = [
	[ "id", "name", "image", "age", "adult" ],
	[ "1", "Fred Flintstone", "images/images/matplotlib.svg", "31", "yes" ],
	[ "2", "Wilma Flintstone " . $LOREMIPSUM, "images/images/matplotlib.svg", "28", "yes" ],
	[ "3", "Pebbles Flintstone", NULL, "5e-1", "no" ],
	[ "4", "Betty Rubble " . $LOREMIPSUM, "images/images/matplotlib.svg", "27", "yes" ],
	[ "5", "Barney Rubble", "images/images/matplotlib.svg", "29", "yes" ],
	[ "6", "Mr. George Slate", "images/images/matplotlib.svg", "53", "yes" ],
	[ "7", "Joe Rockhead", "images/images/matplotlib.svg", "33", "yes" ],
	[ "8", "Sam Slagheap", "images/images/matplotlib.svg", "37", "yes" ],
	[ "9", "The Great Gazoo " . $LOREMIPSUM, "images/images/matplotlib.svg", "1200", "yes" ],
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();
if (!$o->parse_xml("layout_xml42_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
