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

$o = new OpenCReport();

if (!$o->parse_xml("arrayquery.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$ds = $o->datasource_get("myarray");
echo 'datasource: array ' . (($ds instanceof OpenCReport\Datasource) ? "" : "NOT ") . "found" . PHP_EOL;
if (!($ds instanceof OpenCReport\Datasource))
	exit(0);

$q = $o->query_get("a");
echo 'query: "a" ' . (($q instanceof OpenCReport\Query) ? "" : "NOT ") . 'found' . PHP_EOL;
if (!($q instanceof OpenCReport\Query))
	exit(0);

[ $id, $err ] = $o->expr_parse("id");
$id->print();

[ $name, $err ] = $o->expr_parse("name");
$name->print();

[ $age, $err ] = $o->expr_parse("age * 2");
$age->print();

[ $adult, $err ] = $o->expr_parse("a.adult");

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

echo "--- END ---" . PHP_EOL;
