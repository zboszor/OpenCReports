<?php

$o = new OpenCReport();
$o->add_search_path(getcwd());

$conn_params = [ "filename" => "notexist.xls" ];

$ds = $o->datasource_add("pandas", "pandas", $conn_params);
if (is_null($ds)) {
	echo "Adding pandas datasource failed" . PHP_EOL;
	exit(0);
}

echo "Adding a non-existing pandas datasource succeeded (ERROR)." . PHP_EOL;
