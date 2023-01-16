<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$array = [
	[ "text" ],
	[ "bad-tempered" ],
	[ "old" ],
	[ "ladies" ],
	[ "love" ],
	[ "our" ],
	[ "chic" ],
	[ "kitchen" ],
	[ "sink" ],
];

$coltypes = [ OpenCReport::RESULT_STRING ];

$o = new OpenCReport();

$ds = $o->datasource_add_array("array");

$r = $o->part_new()->row_new()->column_new()->report_new();

$q = $ds->query_add("a", "array", "coltypes");
$qr = $q->get_result();

$v = $r->variable_new_full(OpenCReport::RESULT_STRING, "var1", "upper(left(a.text, 1))", NULL, NULL, "r.self + r.baseexpr", NULL);

echo "Base expression for 'var1' reprinted: "; flush();
$v->baseexpr()->print();
echo PHP_EOL;

echo "Result expression for 'var1' reprinted: "; flush();
$v->resultexpr()->print();
echo PHP_EOL;

$row = 0;
$q->navigate_start();
echo "ocrpt_query_navigate_start done" . PHP_EOL;
$r->resolve_variables();
echo "ocrpt_report_resolve_variables done". PHP_EOL;

$e = $r->expr_parse("v.var1");
echo "Variable expression reprinted: "; flush();
$e->print();
echo PHP_EOL;

$e->resolve();

while ($q->navigate_next()) {
	$qr = $q->get_result();

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
