<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$array = [
	[ "id", "name", "property", "age", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes" ],
	[ NULL, "Wilma Flintstone", "charming", "28", "yes" ],
	[ "3", "Pebbles Flintstone", "young", "5e-1", "no" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();
$ds = $o->datasource_add_array("array");

/* Standalone "result" object to test copying expression results */
$rs = OpenCReport::result_new();

$id = 0;
create_expr($o, $id, "id");

$name = 0;
create_expr($o, $name, "name");

$err = 0;
create_expr($o, $err, "error('Error')");

$q = $ds->query_add("a", "array", "coltypes");

$id->resolve();
$name->resolve();
$err->resolve();

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);

	echo PHP_EOL;

	eval_print_expr($id);
	echo "Copied: "; flush();
	$rs->copy($id->get_result());
	$rs->print();

	eval_print_expr($name);
	echo "Copied: "; flush();
	$rs->copy($name->get_result());
	$rs->print();

	eval_print_expr($err);
	echo "Copied: "; flush();
	$rs->copy($err->get_result());
	$rs->print();

	echo PHP_EOL;
}

$id->free();
$name->free();
$err->free();
$rs->free();
