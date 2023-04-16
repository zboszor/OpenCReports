<?php

$array = [
	[ "id", "name", "property", "age", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes" ],
	[ "2", "Wilma Flintstone", "charming", "28", "yes" ],
	[ "3", "Pebbles Flintstone", "young", "5e-1", "no" ],
	[ "4", "Betty Rubble", "beautiful", "27", "yes" ],
	[ "5", "Barney Rubble", "small", "29", "yes" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

$ds1 = $o->datasource_add_array('array1');
$q1 = $ds1->query_add('array1', 'array', 'coltypes');

$ds2 = $o->datasource_add_array('array2');
$q2 = $ds2->query_add('array2', 'array', 'coltypes');

var_dump($o);

var_dump($ds1);
var_dump($q1);

var_dump($ds2);
var_dump($q2);

[ $e, $err ] = $o->expr_parse("2 * age");

var_dump($e);

unset($o);

$e->print();
