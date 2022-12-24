<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "name", "property", "age", "adult" ],
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$srcdir = getenv("abs_srcdir");
if (is_bool($srcdir))
	$srcdir = getcwd();
$srcdir = OpenCReport::canonicalize_path($srcdir);

$o = new OpenCReport();

if (!$o->parse_xml("layout_pdf_xml32_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$o->add_search_path($srcdir . "/images/images");

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
