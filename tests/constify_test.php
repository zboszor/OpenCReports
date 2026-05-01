<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$constify_array = [
	[ "font_name", "font_size", "memo", "expr" ],
	[ "Arial", "10", "yes", "3 * id" ],
	[ "Courier", "12", "no", "id * 3" ],
	[ "Times New Roman", "11", "yes", "name" ]
];

$constify_coltypes = [
	OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING
];

$array = [
	[ "id", "name" ],
	[ "1", "George of the Jungle" ],
	[ "2", "Alice in Wonderland" ]
];

$coltypes = [ OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING ];

$o = new OpenCReport();

$ds = $o->datasource_add("array", "array");

/* Latin1 -> UTF-8, a NOP for plain ASCII characters */
$ds->set_encoding("ISO-8859-1");

$q = $ds->query_add("a", "array", "coltypes");
print_query_columns($q);

$q_constify = $ds->query_add("constify", "constify_array", "constify_coltypes");
print_query_columns($q_constify);

$e = array();
$row_constify = 0;
$q_constify->navigate_start();

while ($q_constify->navigate_next()) {
	$qr = $q_constify->get_result();
	$cols_constify = $qr->columns();

	echo "Row #" . $row_constify + 1 . PHP_EOL;
	print_result_row("constify", $qr);

	echo PHP_EOL;

	$e[$row_constify][0] = $o->expr_parse("font_name");
	$e[$row_constify][1] = $o->expr_parse("font_size");
	$e[$row_constify][2] = $o->expr_parse("memo");
	$e[$row_constify][3] = $o->expr_parse("eval(expr)");

	for ($i = 0; $i < $cols_constify; $i++) {
		$e[$row_constify][$i]->resolve_from_query($q_constify);
		$e[$row_constify][$i]->constify($q_constify);
		$e[$row_constify][$i]->optimize();

		echo "Expression: ";
		$e[$row_constify][$i]->print();
	}

	echo PHP_EOL;

	$row_constify++;
}

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();

	if ($row > 0)
		echo PHP_EOL;

	echo "Row #" . $row . PHP_EOL;
	print_result_row("a", $qr);

	echo PHP_EOL;

	for ($i = 0; $i < $row_constify; $i++) {
		if ($i > 0)
			echo PHP_EOL;

		echo "Constified expressions from row " . $i + 1 . " of 'constify':" . PHP_EOL;
		for ($j = 0; $j < $cols_constify; $j++) {
			eval_print_expr($e[$i][$j]);
		}
	}

	$row++;
}
