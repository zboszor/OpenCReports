<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$array = [
	[ "id", "name", "property", "age", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes" ],
	[ "2", "Wilma Flintstone", "charming", "28", "yes" ],
	[ "3", "Pebbles Flintstone", "young", "5e-1", "no" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

$ds = $o->datasource_add_array("array");

$r = $o->part_new()->row_new()->column_new()->report_new();

$q = $ds->query_add("a", "array", "coltypes");

$r->variable_new(OpenCReport::VARIABLE_EXPRESSION, "var1", "id + 1");

/* Exercise duplicate variable name */
$r->variable_new(OpenCReport::VARIABLE_EXPRESSION, "var1", "id + 1");

/* Exercise expression variable containing another (known) expression variable */
$v = $r->variable_new(OpenCReport::VARIABLE_EXPRESSION, "var2", "v.var1");
echo "adding 'var2' " . (isset($v) ? "succeeded" : "failed") . PHP_EOL;

/* Exercise other variable containing expression variable */
$v = $r->variable_new(OpenCReport::VARIABLE_SUM, "var3", "v.var1");
echo "adding 'var3' " . (isset($v) ? "succeeded" : "failed") . PHP_EOL;

/* Exercise another variable type containing non-expression-type variable */
$r->variable_new(OpenCReport::VARIABLE_HIGHEST, "var4", "v.var3");

/* Exercise another variable type containing unknown variable */
$r->variable_new(OpenCReport::VARIABLE_HIGHEST, "var5", "v.varX");
