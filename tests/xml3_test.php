<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$o = new OpenCReport();

$ds = $o->datasource_add_xml("xml");

create_exprs($o, false);

$q = $ds->query_add("a", "xmldata2.xml");
print_query_columns($q);

resolve_exprs();

$row = 0;
$q->navigate_start();

while ($q->navigate_next()) {
	$qr = $q->get_result();

	echo "Row #" . $row . PHP_EOL;
	$row++;
	print_result_row("a", $qr);

	echo PHP_EOL;

	eval_print_expr($id);

	eval_print_expr($name);

	eval_print_expr($age);

	eval_print_expr($adult);
	echo "Expression is " . ($adult->cmp_results() ? "identical to" : "different from") . " previous row" . PHP_EOL;

	echo PHP_EOL;
}

echo "--- END ---" . PHP_EOL;

free_exprs();
