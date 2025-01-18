<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
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

$ds = $o->datasource_add("array", "array");

/* Latin1 -> UTF-8, a NOP for plain ASCII characters */
$ds->set_encoding("ISO-8859-1");

create_exprs($o, false);

$q = $ds->query_add("a", "array", "coltypes");
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
