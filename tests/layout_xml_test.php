<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "name", "property", "age", "adult" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

if (!$o->parse_xml("layout_xml_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);
$o->execute();
$o->spool();
