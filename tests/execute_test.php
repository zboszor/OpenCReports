<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$parts = [];
$reports = [];

function test_part_iteration_cb(OpenCReport $o, OpenCReport\Part $p) {
	global $parts;

	$i = 0;
	foreach ($parts as &$p0) {
		if ($p0->equals($p)) {
			echo "part " . $i . " iteration done" . PHP_EOL;
			return;
		}
		$i++;
	}

	echo "unknown part???" . PHP_EOL;
}

function test_part_added_cb(OpenCReport $o, OpenCReport\Part $p) {
	global $parts;

	array_push($parts, $p);
	echo "appended part " . (count($parts) - 1) . PHP_EOL;
}

function test_report_start_cb(OpenCReport $o, OpenCReport\Part\Row\Column\Report $r) {
	global $reports;

	$i = 0;
	foreach ($reports as &$r0) {
		if ($r0->equals($r)) {
			echo "report " . $i . " started" . PHP_EOL;
			return;
		}
		$i++;
	}

	echo "unknown report???" . PHP_EOL;
}

function test_report_done_cb(OpenCReport $o, OpenCReport\Part\Row\Column\Report $r) {
	global $reports;

	$i = 0;
	foreach ($reports as &$r0) {
		if ($r0->equals($r)) {
			echo "report " . $i . " done" . PHP_EOL;
			return;
		}
		$i++;
	}

	echo "unknown report???" . PHP_EOL;
}

function test_report_iteration_cb(OpenCReport $o, OpenCReport\Part\Row\Column\Report $r) {
	global $reports;

	$i = 0;
	foreach ($reports as &$r0) {
		if ($r0->equals($r)) {
			echo "report " . $i . " iteration done" . PHP_EOL;
			return;
		}
		$i++;
	}

	echo "unknown report???" . PHP_EOL;
}

function test_report_precalc_done_cb(OpenCReport $o, OpenCReport\Part\Row\Column\Report $r) {
	global $reports;

	$i = 0;
	foreach ($reports as &$r0) {
		if ($r0->equals($r)) {
			echo "report " . $i . " precalculation done" . PHP_EOL;
			return;
		}
		$i++;
	}

	echo "unknown report???" . PHP_EOL;
}

function test_precalc_done_cb(OpenCReport $o) {
	echo "all reports' precalculation done" . PHP_EOL;
}

function test_report_added_cb(OpenCReport $o, OpenCReport\Part\Row\Column\Report $r) {
	global $reports;

	array_push($reports, $r);

	echo "appended report " . (count($reports) - 1) . PHP_EOL;

	$r->add_start_cb("test_report_start_cb");
	$r->add_done_cb("test_report_done_cb");
	$r->add_iteration_cb("test_report_iteration_cb");
	$r->add_precalculation_done_cb("test_report_precalc_done_cb");
}

$o = new OpenCReport();

$o->add_precalculation_done_cb("test_precalc_done_cb");
$o->add_part_added_cb("test_part_added_cb");
$o->add_report_added_cb("test_report_added_cb");

if (!$o->parse_xml("part_xml_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

for ($p = $o->part_get_next(); !is_null($p); $p = $p->part_get_next())
	$p->add_iteration_cb("test_part_iteration_cb");

$o->execute();
