<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "name", "property", "age", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes" ],
	[ "2", "Wilma Flintstone", "charming", "28", "yes" ],
	[ "3", "Pebbles Flintstone", "young", "5e-1", "no" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$array2 = [
	[ "id", "name", "property", "age", "adult" ],
	[ "2", "Betty Rubble", "beautiful", "27", "yes" ],
	[ "1", "Barney Rubble", "small", "29", "yes" ],
];

$o = new OpenCReport();

$ds = $o->datasource_add_array("array");

echo "added query a" . PHP_EOL . PHP_EOL;
$a = $ds->query_add("a", "array", "coltypes");
echo "added query b" . PHP_EOL . PHP_EOL;
$b = $ds->query_add("b", "array2", "coltypes");

echo "adding N:1 follower a -> b with v.var expression, should fail" . PHP_EOL;
$match = $o->expr_parse("v.var");
$retval = $a->add_follower_n_to_1($b, $match);
echo ($retval ? "succeeded" : "failed") . PHP_EOL . PHP_EOL;

echo "adding N:1 follower a -> b with r.var expression, should fail" . PHP_EOL;
$match = $o->expr_parse("r.var");
$retval = $a->add_follower_n_to_1($b, $match);
echo ($retval ? "succeeded" : "failed") . PHP_EOL . PHP_EOL;

echo "adding N:1 follower a -> b with m.var expression, should fail" . PHP_EOL;
$match = $o->expr_parse("m.var");
$retval = $a->add_follower_n_to_1($b, $match);
echo ($retval ? "succeeded" : "failed") . PHP_EOL . PHP_EOL;

echo "adding N:1 follower a -> b with unknown domain reference, should fail" . PHP_EOL;
$match = $o->expr_parse("a.id = c.id");
$retval = $a->add_follower_n_to_1($b, $match);
echo ($retval ? "succeeded" : "failed") . PHP_EOL . PHP_EOL;

echo "adding N:1 follower a -> b with unknown ident reference, should fail" . PHP_EOL;
$match = $o->expr_parse("a.id = b.xxx");
$retval = $a->add_follower_n_to_1($b, $match);
echo ($retval ? "succeeded" : "failed") . PHP_EOL . PHP_EOL;
