<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$array2 = [
	[ "id", "name", "property", "age", "adult" ],
	[ "2", "Betty Rubble", "beautiful", "27", "yes" ],
	[ "1", "Barney Rubble", "small", "29", "yes" ],
];

function create_exprs(OpenCReport $o) {
	global $id;
	global $name;
	global $age;
	global $adult;

	$id = $o->expr_parse("id");
	$id->print();

	$name = $o->expr_parse("name");
	$name->print();

	$age = $o->expr_parse("age * 2");
	$age->print();

	$adult = $o->expr_parse("a.adult");

	$id->resolve();
	$name->resolve();
	$age->resolve();
	$adult->resolve();
}

function free_exprs() {
	global $id;
	global $name;
	global $age;
	global $adult;

	$id->free();
	$name->free();
	$age->free();
	$adult->free();
}

$o = new OpenCReport();

if (!$o->parse_xml("csvquery.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$ds = $o->datasource_get("mycsv");
$q = $o->query_get("a");
$qr = $q->get_result();
echo "Query columns:" . PHP_EOL;
for ($i = 0; $i < $qr->columns(); $i++)
	echo $i . ": '" . $qr->column_name($i) . "'" . PHP_EOL;

create_exprs($o);

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
}

free_exprs();

$q->free();
unset($q);
$ds->free();
unset($ds);

echo "--- TESTING FOLLOWER ---" . PHP_EOL . PHP_EOL;

if (!$o->parse_xml("csvquery2.xml")) {
	printf("XML parse error\n");
	exit(0);
}

$ds = $o->datasource_get("mycsv");
if (is_null($ds)) {
	echo "datasource 'mycsv' not found" . PHP_EOL;
	exit(1);
}
$q = $o->query_get("a");
if (is_null($q)) {
	echo "query 'a' not found" . PHP_EOL;
	exit(1);
}
$qr = $q->get_result();
echo "q cols " . $qr->columns() , PHP_EOL;

$ds2 = $o->datasource_get("myarray");
if (is_null($ds2)) {
	echo "datasource 'myarray' not found". PHP_EOL;
	exit(1);
}
$q2 = $o->query_get("b");
if (is_null($q2)) {
	echo "query 'b' not found" . PHP_EOL;
	exit(1);
}
$qr2 = $q2->get_result();
echo "q2 cols " . $qr2->columns() . PHP_EOL;

create_exprs($o);

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

if (!$o->parse_xml("csvquery3.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");
$q2 = $o->query_get("b");

create_exprs($o);

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
