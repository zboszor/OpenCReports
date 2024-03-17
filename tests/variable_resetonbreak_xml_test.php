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
	[ "3", "3", "Pebbles Flintstone", "young", "5e-1", "no" ],
	[ "4", NULL, "Betty Rubble", "beautiful", "27", "yes" ],
	[ "5", "5", "Barney Rubble", "small", "29", "yes" ]
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

$o = new OpenCReport();

if (!$o->parse_xml("variable_resetonbreak_xml_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");
$qr = $q->get_result();

/* There is only one ocrpt_report pointer in o->parts, extract it. */
$r = get_first_report($o);

/* There is only one break in the report, extract it */
$br = $r->break_get_first();

$e = [];

for ($i = 0; $i < count($test_vars); $i++)
	$e[$i] = $r->expr_parse($test_vars[$i]);

printf("First run of the query\n\n");

$row = 0;
$q->navigate_start();
$r->resolve_breaks();
$r->resolve_variables();

for ($i = 0; $i < count($e); $i++) {
	$e[$i]->resolve();
	$e[$i]->optimize();
}

while ($q->navigate_next()) {
	$qr = $q->get_result();

	if ($br->check_fields()) {
		echo "Break triggers" . PHP_EOL;

		$rownum = $r->get_query_rownum();
		if ($rownum > 1)
			$br->reset_vars();
	}

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
