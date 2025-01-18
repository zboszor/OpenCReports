<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$conn_params = [
	"dbname" => "ocrpttest2",
	"user" => "ocrpt"
];

$ds = $o->datasource_add("odbc", "odbc", $conn_params);

echo "Connecting to ODBC database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;
