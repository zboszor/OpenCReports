<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$OCRPTENV = "This is a test string";
echo "OCRPTENV is set." . PHP_EOL;

[ $e, $err ] = $o->expr_parse("m.OCRPTENV");
if ($e instanceof OpenCReport\Expr) {
	echo "Before resolving: "; flush();
	$e->print();
	$e->resolve();
	$e->optimize();
	echo "After resolving: "; flush();
	$e->print();
} else {
	echo "expr failed to parse: " . $err . PHP_EOL;
}

unset($OCRPTENV);
echo "OCRPTENV is unset." . PHP_EOL;

[ $e, $err ] = $o->expr_parse("m.OCRPTENV");
if ($e instanceof OpenCReport\Expr) {
	echo "Before resolving: "; flush();
	$e->print();
	$e->resolve();
	$e->optimize();
	echo "After resolving: "; flush();
	$e->print();
} else {
	echo "expr failed to parse: " . $err . PHP_EOL;
}