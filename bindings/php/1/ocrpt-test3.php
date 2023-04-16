<?php

function my_dec(OpenCReport\Expr $e) {
	/* Shortcut implementation, no error handling */
	return 0;
}

$o = new OpenCReport();

$o1 = rlib_init();

var_dump($o);

var_dump($o1);

rlib_free($o);

var_dump($o);

rlib_free($o);
$o->function_add("dec", "my_dec", 1, false, false, false, false);

[ $e2, $err ] = $o->expr_parse("100--");
if ($e2 instanceof OpenCReport\Expr) {
	$e2->print();
	echo "e2 nodes: " . $e2->nodes() . PHP_EOL;
	$e2->optimize();
	$e2->print();
	echo "e2 nodes: " . $e2->nodes() . PHP_EOL;
} else {
	echo $err . PHP_EOL;
}
