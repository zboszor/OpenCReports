<?php

require_once 'test_common.php';

$row = 0;

function test_newrow_cb(OpenCReport $o, OpenCReport\Report $r) {
	global $row;
	global $q;

	$qr = $q->get_result();

	if ($row > 0)
		echo PHP_EOL;
	echo "Row #" . $row . PHP_EOL;
	$row++;

	print_result_row("a", $qr);
}

$o = new OpenCReport();
$o->add_search_path(getcwd());

if (!$o->parse_xml("xlsds.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$ds = $o->datasource_get("pandas");
if (is_null($ds)) {
	echo "Adding pandas datasource failed" . PHP_EOL;
	exit(0);
}

$q = $o->query_get("a");

if (is_null($q)) {
	echo "Adding query for Sheet1 from test.xls failed" . PHP_EOL;
	exit(0);
}

$r = $o->part_new()->row_new()->column_new()->report_new();
$r->set_main_query($q);

$r->add_new_row_cb("test_newrow_cb");

$o->execute();

echo PHP_EOL;
