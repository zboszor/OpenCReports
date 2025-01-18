<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

function my_inc(OpenCReport\Expr $e) {
	/* Complete implementation, akin to the C unit test example */
	$opres = $e->operand_get_result(0);
	if ($e->get_num_operands() != 1 || is_null($opres)) {
		$e->make_error_result("invalid operand(s)");
		return;
	}

	switch ($opres->get_type()) {
	case OpenCReport::RESULT_NUMBER:
		$e->set_long(1);
		break;
	case OpenCReport::RESULT_ERROR:
		$e->make_error_result($opres->get_string());
		break;
	case OpenCReport::RESULT_STRING:
	case OpenCReport::RESULT_DATETIME:
	default:
		$e->make_error_result(e, "invalid operand(s)");
		break;
	}
}

function my_dec(OpenCReport\Expr $e) {
	/* Shortcut implementation, no error handling */
	return 0;
}

$o = new OpenCReport();

/* Override the stock increment and decrement functions with constant 1 and 0 */
$o->function_add("inc", "my_inc", 1, false, false, false, false);
$o->function_add("dec", "my_dec", 1, false, false, false, false);

$e1 = $o->expr_parse("100++");
if ($e1 instanceof OpenCReport\Expr) {
	$e1->print(); flush();
	echo "e1 nodes: " . $e1->nodes() . PHP_EOL;
	$e1->optimize();
	$e1->print(); flush();
	echo "e1 nodes: " . $e1->nodes() . PHP_EOL;
} else {
	echo $o->expr_error() . PHP_EOL;
}

$e2 = $o->expr_parse("100--");
if ($e2 instanceof OpenCReport\Expr) {
	$e2->print(); flush();
	echo "e2 nodes: " . $e2->nodes() . PHP_EOL;
	$e2->optimize();
	$e2->print(); flush();
	echo "e2 nodes: " . $e2->nodes() . PHP_EOL;
} else {
	echo $o->expr_error() . PHP_EOL;
}
