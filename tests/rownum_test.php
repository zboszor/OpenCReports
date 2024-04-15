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

$array2 = [
	[ "id", "name", "property", "age", "adult" ],
	[ "2", "Betty Rubble", "beautiful", "27", "yes" ],
	[ "1", "Barney Rubble", "small", "29", "yes" ],
];

$o = new OpenCReport();

$ds = $o->datasource_add("array", "array");

$id = 0;
create_expr($o, $id, "id");

$rownum1 = 0;
create_expr($o, $rownum1, "rownum()");

$rownum2 = 0;
create_expr($o, $rownum2, "rownum('a')");

$rownum3 = 0;
create_expr($o, $rownum3, "rownum('b')");

$q = $ds->query_add("a", "array", "coltypes");
$q2 = $ds->query_add("b", "array2", "coltypes");
$q->add_follower($q2);

$id->resolve();
$rownum1->resolve();
$rownum2->resolve();
$rownum3->resolve();

echo "--- TESTING FOLLOWER ---" . PHP_EOL . PHP_EOL;

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();
	$qr2 = $q2->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);
	print_result_row("b", $qr2);

	echo PHP_EOL;

	eval_print_expr($id);

	eval_print_expr($rownum1);

	eval_print_expr($rownum2);

	eval_print_expr($rownum3);

	echo PHP_EOL;
}

/*
 * rownum3 referenced query 'b'
 * free this expression to avoid use-after-free
 * after freeing and re-creating q2 a.k.a. query 'b'
 */
$rownum3->free();
unset($rownum3);

$q2->free();
unset($q2);

echo "--- TESTING FOLLOWER N:1 ---" . PHP_EOL . PHP_EOL;

$q2 = $ds->query_add("b", "array2", "coltypes");
$qr2 = $q2->get_result();

$match = $o->expr_parse("a.id = b.id");

$rownum3 = $o->expr_parse("rownum('b')");

$q->add_follower_n_to_1($q2, $match);

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();
	$qr2 = $q2->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);
	print_result_row("b", $qr2);

	echo PHP_EOL;

	eval_print_expr($id);

	eval_print_expr($rownum1);

	eval_print_expr($rownum2);

	eval_print_expr($rownum3);

	echo PHP_EOL;
}

echo "--- END ---" . PHP_EOL;
