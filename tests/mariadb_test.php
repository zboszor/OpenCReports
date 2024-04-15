<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$conn_params = [
	"dbname" => "ocrpttest",
	"user" => "ocrpt"
];

$ds = $o->datasource_add("mariadb", "mariadb", $conn_params);

echo "Connecting to MariaDB database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;
