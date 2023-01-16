<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$array = [
	[ "id", "name", "age", "male" ],
	[ "1", "Fred Flintstone", "31", "yes" ],
	[ "2", "Barney Rubble", "29", "yes" ],
	[ "3", "Bamm-Bamm Rubble", "2", "yes" ],
	[ "4", "Wilma Flintstone", "28", "no" ],
	[ "5", "Betty Rubble", "27", "no" ],
	[ "6", "Pebbles Flintstone", "5e-1", "no" ],
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$row = 0;

function test_newrow_cb(OpenCReport $o, OpenCReport\Part\Row\Column\Report $r) {
	global $row;
	global $e;

	if ($row > 0)
		echo PHP_EOL;
	echo "Row #" . $row . PHP_EOL;
	$row++;

	$rs = $e->get_result();
	$e->print();
	$rs->print();
}

function test_break_trigger_cb(OpenCReport $o, OpenCReport\Part\Row\Column\Report $r, OpenCReport\Part\Row\Column\Report\Break $br) {
	echo "break '" . $br->name() . "' triggered" . PHP_EOL;
}

$o = new OpenCReport();

if (!$o->parse_xml("break_multi2_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");

/* There is only one ocrpt_report pointer in o->parts, extract it. */
$r = get_first_report($o);

/* This is a precalculate="yes" variable, resulting in delayed expression calculation */
$e = $r->expr_parse("v.age_avg");
$e->set_delayed(true);

$r->add_new_row_cb("test_newrow_cb");

$br = $r->break_get("male");
$br->add_trigger_cb("test_break_trigger_cb");

$br = $r->break_get("adult");
$br->add_trigger_cb("test_break_trigger_cb");

$o->execute();
