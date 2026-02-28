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

$conn_params = [ "filename" => "test.xls" ];

$ds = $o->datasource_add("pandas", "pandas", $conn_params);
if (is_null($ds)) {
	echo "Adding pandas datasource failed" . PHP_EOL;
	exit(0);
}

$q = $ds->query_add("a", "Sheet1");

if (is_null($q)) {
	echo "Adding query for Sheet1 from test.xls failed" . PHP_EOL;
	exit(0);
}

$r = $o->part_new()->row_new()->column_new()->report_new();
$r->set_main_query($q);

$r->add_new_row_cb("test_newrow_cb");

$o->execute();

echo PHP_EOL;
