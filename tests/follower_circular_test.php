<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
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
$ds = $o->datasource_add("array", "array");

echo "added query a" . PHP_EOL . PHP_EOL;
$a = $ds->query_add("a", "array", "coltypes");
echo "added query b" . PHP_EOL . PHP_EOL;
$b = $ds->query_add("b", "array2", "coltypes");

echo "adding N:1 follower a -> b, should succeed" . PHP_EOL;
$match_ab = $o->expr_parse("a.id = b.id");
$retval = $a->add_follower_n_to_1($b, $match_ab);
echo "added N:1 follower a -> b, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding N:1 follower a -> b (duplicate), should fail" . PHP_EOL;
$match_ab = $o->expr_parse("a.id = b.id");
$retval = $a->add_follower_n_to_1($b, $match_ab);
echo "added N:1 follower a -> b, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding follower a -> b (N:1 exists), should fail" . PHP_EOL;
$retval = $a->add_follower($b);
echo "added follower a -> b, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding N:1 follower b -> a (reverse N:1 exists), should fail" . PHP_EOL;
$match_ab = $o->expr_parse("a.id = b.id");
$retval = $b->add_follower_n_to_1($a, $match_ab);
echo "added N:1 follower b -> a, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding follower b -> a (reverse N:1 exists), should fail" . PHP_EOL;
$retval = $b->add_follower($a);
echo "added follower b -> a, retval " . (int)$retval . PHP_EOL . PHP_EOL;

unset($c);

echo "adding N:1 follower b -> c (query c does not exist), should fail" . PHP_EOL;
$match_bc = $o->expr_parse("b.id = c.id");
try {
	$retval = $b->add_follower_n_to_1($c, $match_bc);
} catch (TypeError $e) {
	$retval = false;
}
echo "added N:1 follower b -> c, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding follower b -> c (query c does not exist), should fail" . PHP_EOL;
try {
	$retval = $b->add_follower($c);
} catch (TypeError $e) {
	$retval = false;
}
echo "added follower b -> c, retval " . (int)$retval . PHP_EOL . PHP_EOL;

$c = $ds->query_add("c", "array2", "coltypes");
echo "added query c" . PHP_EOL . PHP_EOL;

echo "adding N:1 follower b -> c, should succeed" . PHP_EOL;
$match_bc = $o->expr_parse("b.id = c.id");
$retval = $b->add_follower_n_to_1($c, $match_bc);
echo "added N:1 follower b -> c, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding follower b -> c (N:1 follower exists), should fail" . PHP_EOL;
$retval = $b->add_follower($c);
echo "added follower b -> c, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding N:1 follower c -> a (circular followers), should fail" . PHP_EOL;
$match_ac = $o->expr_parse("a.id = c.id");
$retval = $c->add_follower_n_to_1($a, $match_ac);
echo "added N:1 follower c -> a, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding follower c -> a (a->b->c exists), should fail" . PHP_EOL;
$retval = $c->add_follower($a);
echo "added N:1 follower c -> a, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding N:1 follower a -> c (c would be a follower on two paths), should fail" . PHP_EOL;
$match_ac = $o->expr_parse("a.id = c.id");
$retval = $a->add_follower_n_to_1($c, $match_ac);
echo "added N:1 follower a -> c, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding follower a -> c (c would be a follower on two paths), should fail" . PHP_EOL;
$retval = $a->add_follower($c);
echo "added follower a -> c, retval " . (int)$retval . PHP_EOL . PHP_EOL;

$d = $ds->query_add("d", "array2", "coltypes");
echo "added query d" . PHP_EOL . PHP_EOL;

echo "adding follower c -> d, should succeed" . PHP_EOL;
$match_cd = $o->expr_parse("c.id = d.id");
$retval = $c->add_follower_n_to_1($d, $match_cd);
echo "added follower c -> d, retval " . (int)$retval . PHP_EOL . PHP_EOL;

echo "adding follower d -> a, should fail" . PHP_EOL;
$match_cd = $o->expr_parse("c.id = d.id");
$retval = $d->add_follower_n_to_1($a, $match_cd);
echo "added follower d -> a, retval " . (int)$retval . PHP_EOL . PHP_EOL;
