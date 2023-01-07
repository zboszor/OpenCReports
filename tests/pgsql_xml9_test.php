<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$o = new OpenCReport();

$dsname = "pgsql";
$dstype = "postgresql";
$dbconnstr = "dbname=ocrpttest user=ocrpt";
$qname = "pgquery";

if (!$o->parse_xml("pgsqlquery9.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$ds = $o->datasource_get("pgsql");

echo "Connecting to PostgreSQL database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;

$q = $o->query_get("pgquery");
echo "Adding query was " . (($q instanceof OpenCReport\Query) ? "" : "NOT ") . "successful" . PHP_EOL;

print_query_columns($q);

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);

	echo PHP_EOL;
}
