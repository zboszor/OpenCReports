<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

const ROWS=10000000;

$row = 0;

function test_newrow_cb(OpenCReport $o, OpenCReport\Part\Row\Column\Report $r) {
	global $row;
	global $e;

	$row++;
	if ($row % 100000 == 0)
		echo "at row " . $row . PHP_EOL;
	if ($row == ROWS) {
		$rs = $e->get_result();
		$rs->print();
		unset($rs);
	}
}

$o = new OpenCReport();

$ds = $o->datasource_add_postgresql("pgsql", NULL, NULL, "ocrpttest", "ocrpt", NULL);
if (is_null($ds)) {
	echo "ocrpt_datasource_add_postgresql failed" . PHP_EOL;
	exit(0);
}

$q = $ds->query_add("a", "select '0.0000001'::numeric as num from generate_series(1," . ROWS . ");");
if (is_null($q)) {
	echo "ocrpt_query_add_postgresql failed" . PHP_EOL;
	exit(0);
}

$r = $o->part_new()->row_new()->column_new()->report_new();
$r->set_main_query($q);

$r->add_new_row_cb("test_newrow_cb");

$r->variable_new(OpenCReport::VARIABLE_SUM, "var1", "a.num");

$e = $r->expr_parse("v.var1");
$e->resolve();
$e->optimize();

$o->execute();
