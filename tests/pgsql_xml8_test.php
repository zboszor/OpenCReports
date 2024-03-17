<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$dsname = "pgsql";
$dstype = "postgresql";
$dbconnstr = "dbname=ocrpttest user=ocrpt";
$qname = "pgquery";
$query = "SELECT * FROM flintstones;";

if (!$o->parse_xml("pgsqlquery8.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$ds = $o->datasource_get("pgsql");

echo "Connecting to PostgreSQL database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;
