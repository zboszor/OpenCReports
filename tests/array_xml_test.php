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

create_exprs($o);

print_query_columns($q);

resolve_exprs();

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);

	echo PHP_EOL;

	eval_print_expr($id);

	eval_print_expr($name);

	eval_print_expr($age);

	eval_print_expr($adult);
	echo "Expression is " . ($adult->cmp_results() ? "identical to" : "different from") . " previous row" . PHP_EOL;

	echo PHP_EOL;
}

echo "--- END ---" . PHP_EOL;
