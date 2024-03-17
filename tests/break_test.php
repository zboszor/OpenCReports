<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$array = [
	[ "id", "name", "property", "age", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes" ],
	[ "2", "Wilma Flintstone", "charming", "28", "yes" ],
	[ "3", "Pebbles Flintstone", "young", "5e-1", "no" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

$ds = $o->datasource_add_array("array");

$q = $ds->query_add("a", "array", "coltypes");

$r = $o->part_new()->row_new()->column_new()->report_new();

$br = $r->break_new("age");
if (!isset($br)) {
	echo "adding break failed" . PHP_EOL;
	exit(0);
}

$e = $r->expr_parse("age > 18");

if (!$br->breakfield_add($e)) {
	echo "adding breakfield failed" . PHP_EOL;
	exit(0);
}

echo "adding a break and a breakfield to it succeeded" . PHP_EOL;
