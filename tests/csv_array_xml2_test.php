<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$array2 = [
	[ "id", "name", "property", "age", "adult" ],
	[ "2", "Betty Rubble", "beautiful", "27", "yes" ],
	[ "1", "Barney Rubble", "small", "29", "yes" ],
];

$o = new OpenCReport();

if (!$o->parse_xml("csvquery-notypes.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");
$qr = $q->get_result();
echo "Query columns:" . PHP_EOL;
for ($i = 0; $i < $qr->columns(); $i++)
	echo $i . ": '" . $qr->column_name($i) . "'" . PHP_EOL;

create_exprs_with_val($o);

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
	echo "Expression is " . ($adult->cmp_results() ? "identical to" : "different from") . " previous row" . PHP_EOL;;

	echo PHP_EOL;

	unset($qr);
}

free_exprs();

$q->free();

echo "--- TESTING FOLLOWER ---" . PHP_EOL . PHP_EOL;

if (!$o->parse_xml("csvquery2-notypes.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");
$qr = $q->get_result();
echo "q cols " . $qr->columns() , PHP_EOL;

$q2 = $o->query_get("b");
$qr2 = $q2->get_result();
echo "q2 cols " . $qr2->columns() . PHP_EOL;

create_exprs_with_val($o);

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

free_exprs();

$q->free();
unset($q);
$q2->free();
unset($q2);

echo "--- TESTING FOLLOWER N:1 ---" . PHP_EOL . PHP_EOL;

if (!$o->parse_xml("csvquery3-notypes.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");
$q2 = $o->query_get("b");

create_exprs_with_val($o);

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

free_exprs();

echo "--- END ---" . PHP_EOL;
