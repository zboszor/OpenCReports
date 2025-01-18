<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "name", "property", "age", "male", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "2", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "3", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "4", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "5", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "6", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();
if (!$o->parse_xml("layout_xml5_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
