<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$o = new OpenCReport();

$p1 = $o->part_new();
$p2 = $o->part_new();

echo "Allocated two parts for report structure" . PHP_EOL;

/* Both parts will be freed implicitly */
unset($o);

echo "Freed report structure" . PHP_EOL;

$o = new OpenCReport();

$p1 = $o->part_new();
$pr1 = $p1->row_new();
$pd1 = $pr1->column_new();
$r1 = $pd1->report_new();

$p2 = $o->part_new();
$pr2 = $p2->row_new();
$pd2 = $pr2->column_new();
$r2 = $pd2->report_new();

echo "Allocated two parts and one report for each part" . PHP_EOL;

print_part_reports("p1", $p1);
print_part_reports("p2", $p2);

unset($o);

echo "Freed report structure" . PHP_EOL;

$o = new OpenCReport();

$p1 = $o->part_new();

$p2 = $o->part_new();
$pr2 = $p2->row_new();
$pd2 = $pr2->column_new();
$r1 = $pd2->report_new();
$r2 = $pd2->report_new();

echo "Allocated two parts and two reports for the second part" . PHP_EOL;

print_part_reports("p1", $p1);
print_part_reports("p2", $p2);

unset($o);

echo "Freed report structure" . PHP_EOL;
