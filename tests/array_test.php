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
$ds = $o->datasource_add_array("array");

$id = $o->expr_parse("id");
$id->print();

$name = $o->expr_parse("name");
$name->print();

$age = $o->expr_parse("age * 2");
$age->print();

$adult = $o->expr_parse("a.adult");

$q = $ds->query_add("a", "array", "coltypes");
$qr = $q->get_result();
echo "Query columns:" . PHP_EOL;
for ($i = 0; $i < $qr->columns(); $i++)
	echo $i . ": '" . $qr->column_name($i) . "'" . PHP_EOL;

$id->resolve();
$name->resolve();
$age->resolve();
$adult->resolve();

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);

	echo PHP_EOL;

	echo "Expression: "; flush();
	$id->print();
	$r = $id->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo "Expression: "; flush();
	$name->print();
	$r = $name->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo "Expression: "; flush();
	$age->print();
	$r = $age->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo "Expression: "; flush();
	$adult->print();
	$r = $adult->eval();
	echo "Evaluated: "; flush();
	$r->print();
	echo "Expression is " . ($adult->cmp_results() ? "identical to" : "different from") . " previous row" . PHP_EOL;

	echo PHP_EOL;
}

echo "--- TESTING FOLLOWER ---" . PHP_EOL . PHP_EOL;

$q2 = $ds->query_add("b", "array2", "coltypes");
$q->add_follower($q2);

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

	echo "Expression: "; flush();
	$id->print();
	$r = $id->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo "Expression: "; flush();
	$name->print();
	$r = $name->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo "Expression: "; flush();
	$age->print();
	$r = $age->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo PHP_EOL;
}

$q2->free();

echo "--- TESTING FOLLOWER N:1 ---" . PHP_EOL . PHP_EOL;

$q2 = $ds->query_add("b", "array2", "coltypes");
$qr2 = $q2->get_result();
$match = $o->expr_parse("a.id = b.id");

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

	echo "Expression: "; flush();
	$id->print();
	$r = $id->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo "Expression: "; flush();
	$name->print();
	$r = $name->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo "Expression: "; flush();
	$age->print();
	$r = $age->eval();
	echo "Evaluated: "; flush();
	$r->print();

	echo PHP_EOL;
}

echo "--- END ---" . PHP_EOL;
