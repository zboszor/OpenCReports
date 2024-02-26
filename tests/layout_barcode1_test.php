<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "bc", "type" ],
	[ "1",  "123456789010", "ean-13" ],
	[ "2",  "123456789011", "ean-13" ],
	[ "3",  "123456789012", "ean-13" ],
	[ "4",  "123456789013", "ean-13" ],
	[ "5",  "123456789010A", "code39" ],
	[ "6",  "123456789011B", "code39" ],
	[ "7",  "123456789012C", "code39" ],
	[ "8",  "123456789013D", "code39" ],
	[ "9",  "123456789010Aa", "code128b" ],
	[ "10", "123456789011Ba", "code128b" ],
	[ "11", "123456789012Ca", "code128b" ],
	[ "12", "123456789013Da", "code128b" ],
	[ "13", "123456789010", "code128c" ],
	[ "14", "123456789011", "code128c" ],
	[ "15", "123456789012", "code128c" ],
	[ "16", "123456789013", "code128c" ],
	[ "17", "123456789010Aa", "code128" ],
	[ "18", "123456789011Ba", "code128" ],
	[ "19", "123456789012Ca", "code128" ],
	[ "20", "123456789013Da", "code128" ],
];

$o = new OpenCReport();
$ds = $o->datasource_add_array("array");
$q = $ds->query_add("data", "array");

if (!$o->parse_xml("layout_barcode_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
