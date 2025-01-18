<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$o = new OpenCReport();

if (!$o->parse_xml("mariadbquery3.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$ds = $o->datasource_get("mariadb");
echo "Connecting to MariaDB database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;

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
