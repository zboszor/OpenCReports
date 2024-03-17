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

if (!$o->parse_xml("variable_expression_xml_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");
$qr = $q->get_result();

/* There is only one ocrpt_report pointer in o->parts, extract it. */
$r = get_first_report($o);

$e = $r->expr_parse("v.var1");
echo "Variable expression reprinted: "; flush();
$e->print();
echo PHP_EOL;

$e->resolve();

$row = 0;
$q->navigate_start();
$r->resolve_variables();

while ($q->navigate_next()) {
	$qr =$q->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);

	echo PHP_EOL;

	$r->evaluate_variables();

	echo "Expression: "; flush();
	$e->print();
	$rs = $e->eval();
	echo "Evaluated: "; flush();
	$rs->print();

	echo PHP_EOL;
}
