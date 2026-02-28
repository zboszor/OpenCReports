<?php

$o = new OpenCReport();
$o->add_search_path(getcwd());

$conn_params = [ "filename" => "test.xls" ];

$ds = $o->datasource_add("pandas", "pandas", $conn_params);
if (is_null($ds)) {
	echo "Adding pandas datasource failed" . PHP_EOL;
	exit(0);
}

$q = $ds->query_add("a", "Sheet2");
if (is_null($q)) {
	echo "Failed to add a spreadsheet query." . PHP_EOL;
	exit(0);
}

echo "Adding a query for a non-existing spreadsheet name succeeded (ERROR)." . PHP_EOL;
