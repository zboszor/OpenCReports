<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

$dsname = "pgsql";
$dstype = "postgresql";
$dbconnstr = "dbname=ocrpttest user=ocrpt";
$qname = "pgquery";

if (!$o->parse_xml("pgsqlquery10.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$ds = $o->datasource_get("pgsql");

echo "Connecting to PostgreSQL database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;

$q = $o->query_get("a");
echo "Adding query 'a' was " . (($q instanceof OpenCReport\Query) ? "" : "NOT ") . "successful" . PHP_EOL;
$q2 = $o->query_get("b");
echo "Adding query 'b' was " . (($q2 instanceof OpenCReport\Query) ? "" : "NOT ") . "successful" . PHP_EOL;

print_query_columns($q, "a");
print_query_columns($q2, "b");

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();
	$qr2 = $q2->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);
	print_result_row("b", $qr2);

	echo PHP_EOL;
}
