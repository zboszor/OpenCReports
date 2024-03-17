<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$array = [
	[ "id", "id2", "name", "property", "age", "adult" ],
	[ "1", "1", "Fred Flintstone", "strong", "31", "yes" ],
	[ "2", NULL, "Wilma Flintstone", "charming", "28", "yes" ],
	[ "3", "3", "Pebbles Flintstone", "young", "5e-1", "no" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$test_vars = [
	"id2",
	"v.var_id_count",
	"v.var_id_countall",
	"v.var_id_sum",
	"v.var_id_highest",
	"v.var_id_lowest",
	"v.var_id_avg",
	"v.var_id_avgall",
	"v.var_id2_count",
	"v.var_id2_countall",
	"v.var_id2_sum",
	"v.var_id2_highest",
	"v.var_id2_lowest",
	"v.var_id2_avg",
	"v.var_id2_avgall"
];

#define N_TEST_VARS (sizeof(test_vars) / sizeof(char *))

$o = new OpenCReport();

if (!$o->parse_xml("variable_xml_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");
$qr = $q->get_result();

/* There is only one ocrpt_report pointer in o->parts, extract it. */
$r = get_first_report($o);

$e = [];

for ($i = 0; $i < count($test_vars); $i++)
	$e[$i] = $r->expr_parse($test_vars[$i]);

$row = 0;
$q->navigate_start();
$r->resolve_variables();

for ($i = 0; $i < count($e); $i++) {
	$e[$i]->resolve();
	$e[$i]->optimize();
}

while ($q->navigate_next()) {
	$qr = $q->get_result();

	$r->evaluate_variables();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);

	echo PHP_EOL;

	for ($i = 0; $i < count($e); $i++)
		eval_print_expr($e[$i]);

	echo PHP_EOL;
}

for ($i = 0; $i < count($e); $i++)
	$e[$i]->free();
