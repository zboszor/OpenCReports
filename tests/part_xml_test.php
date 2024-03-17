<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

if (!$o->parse_xml("part_xml_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

for ($p = $o->part_get_first(), $i = 0; isset($p); $p = $p->get_next(), $i++) {
	$partname = "part " . $i;
	print_part_reports($partname, $p);
}
