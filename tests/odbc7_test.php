<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$o = new OpenCReport();

$ds = $o->datasource_add_odbc("odbc", "ocrpttest2", "ocrpt", NULL);
$ds2 = $o->datasource_add_odbc("odbc2", "ocrpttest2", "ocrpt", NULL);
$q = $ds->query_add("a", "SELECT * FROM flintstones");
$q2 = $ds2->query_add("b", "SELECT * FROM rubbles");

print_query_columns($q);
print_query_columns($q2);

$match = $o->expr_parse("a.id = b.id");

$q->add_follower_n_to_1($q2, $match);

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();
	$qr2 = $q2->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;

	print_result_row("a", $qr);
	echo PHP_EOL;

	print_result_row("b", $qr2);
	echo PHP_EOL;
}