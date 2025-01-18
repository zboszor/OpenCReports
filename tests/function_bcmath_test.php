<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

function my_addtento30(OpenCReport\Expr $e) {
	/* Complete implementation, akin to the C unit test example */
	$opres = $e->operand_get_result(0);
	if ($e->get_num_operands() != 1 || is_null($opres)) {
		$e->make_error_result("invalid operand(s)");
		return;
	}

	switch ($opres->get_type()) {
	case OpenCReport::RESULT_NUMBER:
		$s = $opres->get_number("%.15RF");
		$e->set_number(bcadd($s, "1000000000000000000000000000000.000001", 6));
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

$o = new OpenCReport();

/* Override the stock increment and decrement functions with constant 1 and 0 */
$o->function_add("addtento30", "my_addtento30", 1, false, false, false, false);

$e1 = $o->expr_parse("addtento30(100)");
if ($e1 instanceof OpenCReport\Expr) {
	$e1->print(); flush();
	echo "e1 nodes: " . $e1->nodes() . PHP_EOL;
	$e1->optimize();
	$e1->print(); flush();
	echo "e1 nodes: " . $e1->nodes() . PHP_EOL;
} else {
	echo $o->expr_error() . PHP_EOL;
}
