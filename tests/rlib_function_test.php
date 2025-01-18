<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

function my_add_func($a, $b) {
	return $a + $b;
}

function my_concat_func($s, $n) {
	return str_repeat($s, $n);
}

$o = rlib_init();
rlib_add_function($o, "my_add", "my_add_func", 2);
rlib_add_function($o, "my_concat", "my_concat_func", 2);

$e = $o->expr_parse("my_add(1,2)");
$e->optimize();
eval_print_expr($e);

$e = $o->expr_parse("my_concat('aaa', 3, 4)");
echo $o->expr_error() . PHP_EOL;

$e = $o->expr_parse("my_concat('abc', 3)");
$e->optimize();
eval_print_expr($e);
