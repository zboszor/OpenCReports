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
	[ "3", "Pebbles Flintstone", "young", "5e-1", "no" ],
	[ "4", "Betty Rubble", "beautiful", "27", "yes" ],
	[ "5", "Barney Rubble", "small", "29", "yes" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

$ds = $o->datasource_add("array", "array");
$q = $ds->query_add("a", "array", "coltypes");

$e1 = $o->expr_parse("r.self + 2");

/* Initialize the value of "e1" to 0 */
$e1->init_results(OpenCReport::RESULT_NUMBER);
for ($i = 0; $i < OpenCReport::EXPR_RESULTS; $i++)
	$e1->set_nth_result_long($i, 0);
$e1->set_iterative_start_value(true);
$e1->resolve();
$e1->optimize();
echo "e1 expr (starts with initial value): "; flush();
$e1->print();

$e2 = $o->expr_parse("r.self + 2");
/* Initialize the value of "e2" to 0 */
$e2->init_results(OpenCReport::RESULT_NUMBER);
for ($i = 0; $i < OpenCReport::EXPR_RESULTS; $i++)
	$e2->set_nth_result_long($i, 0);
/* This iterative expression starts with evaluated value */
//ocrpt_expr_set_iterative_start_value(e2, true);
$e2->resolve();
$e2->optimize();
echo "e2 expr (starts with evaluated value): "; flush();
$e2->print();

echo "Both e1 and e2 were initialized to 0" . PHP_EOL . PHP_EOL;

$q->navigate_start();

while ($q->navigate_next()) {
	$r = $e1->eval();
	echo "e1 value: "; flush();
	$r->print();

	$r = $e2->eval();
	echo "e2 value: "; flush();
	$r->print();

	echo PHP_EOL;
}

$e1->free();
$e2->free();
