<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();
$exprs = [
	/* Various numeric functions */
	"|1|",
	"|nulln()|",
	"-1",
	"-nulln()",
	"factorial(nulln())",
	"factorial(-1)",
	"factorial(0)",
	"factorial(1)",
	"factorial(3)",
	"3!",
	"!3!",
	"5 % 3",
	"4.9 % 2.9",
	"4.9 % 3.1",
	"fmod(5, 3)",
	"fmod(4.9, 2.9)",
	"fmod(4.9, 3.1)",
	"remainder(5, 3)",
	"remainder(4.9, 2.9)",
	"remainder(4.9, 3.1)",

	/* Left-associative operator sequence */
	"1 + 2 + 3",
	"1 * 2 * 3",
	"1 * 2 / 3",
	"1 / 2 * 3",

	/* Operator precedence without parenthesis */
	"1 + 2 * 3",
	"1 - 2 * 3",
	"-1 + -2 * -3",
	"-1 - -2 * -3",

	/* Operator precedence with parenthesis */
	"(1 + 2) * 3",
	"(1 - 2) * 3",
	"(-1 + -2) * -3",
	"(-1 - -2) * -3",

	/* Increment/decrement */
	"1++",
	"++1",
	"1--",
	"--1",

	/* Identical operators (optimizer test) */
	"6 / 3 / 2 ",	/* 1 */
	"(6 / 3) / 2",	/* 1 */
	"6 / (3 / 2)",	/* 4 */
	"a / b / c",	/* can be same level of a single div() call */
	"(a / b) / c",	/* can be same level of a single div() call */
	"a / (b / c)",	/* b/c must stay as one subexpression */

	/* Facebook challange with implicit multiplication */
	"1/(1+1)(2+2)",
	"1/(1+1)*(2+2)",

	/* Misc. expressions */
	"(2+2).2",
	"1/2x",
	"1/2e",
	"2e",
	"add(1, 2, 3, 4, 5, (6))",
	"add(a, 1, b, 2)",
	"a + 1 + b + 2",

	/* String comparison expressions */
	"'a' = 'a'",
	"'a' = 'b'",
	"'a' <> 'a'",
	"'a' <> 'b'",
	"'a' < 'a'",
	"'a' < 'b'",
	"'a' <= 'a'",
	"'a' <= 'b'",
	"'a' > 'a'",
	"'a' > 'b'",
	"'a' >= 'a'",
	"'a' >= 'b'",

	/* Logical operators */
	"0 && 0",
	"0 && 1",
	"1 && 0",
	"1 && 1",
	"0 && 0.5",
	"0.5 && 0.5",
	"0 && 0.4",
	"0.4 && 0.4",

	"0 || 0",
	"0 || 1",
	"1 || 0",
	"1 || 1",
	"0 || 0.5",
	"0.5 || 0.5",
	"0 || 0.4",
	"0.4 || 0.4",

	"!0",
	"!1",
	"!0.4",

	/* Binary operators */
	"1 | 2",
	"1 & 2",
	"3 & 2",
	"1 ^ 2",
	"3 ^ 2",
	"~0",
	"~1",
	"1 << 2",
	"12 >> 2",

	/* String arithmetics tests */
	"'a' + 'b'",		/* should be 'ab' */
	"'a' + nulls()",	/* should be NULL */
	"concat('a', 'b')",	/* same */
	"'a' - 'b'",		/* should be an error */
	"left('árvíztűrő tükörfúrógép', 9)",
	"mid('árvíztűrő tükörfúrógép', 0, 9)",	/* start index 0 and 1 are the same */
	"mid('árvíztűrő tükörfúrógép', 1, 9)",
	"mid('árvíztűrő tükörfúrógép', 6, 10)",
	"mid('árvíztűrő tükörfúrógép', -12, 9)",
	"right('árvíztűrő tükörfúrógép', 7)",
	"lower('ÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP')",
	"upper('árvíztűrő tükörfúrógép')",
	"proper('ÁRVÍZTŰRŐ TÜKÖRFÚRÓGÉP')",
	"proper('árvíztűrő tükörfúrógép')",

	/* String arithmetics NULL tests */
	"left('a', nulln())",
	"left(nulls(), 1)",
	"left(nulls(), nulln())",
	"right('a', nulln())",
	"right(nulls(), 1)",
	"right(nulls(), nulln())",

	/* val() conversions */
	"val('1.5')",
	"val(1.5)",
	"val('yes')",
	"val(yes)",

	/* fxpval() conversion */
	"fxpval('123456', 2)",
	"fxpval(val('123456'), 2)",

	/* str() conversion */
	"str(1234.56, 6, 4)",
	"str(1234.56, 10, 4)",

	/* NULL tests */
	"null(1)",
	"null('a')",
	"nulldt()",
	"nulln()",
	"nulls()",

	/* Error tests */
	"error('custom error')",
];

foreach ($exprs as &$str) {
	echo "string: " . $str . PHP_EOL;
	[ $e, $err ] = $o->expr_parse($str);
	if ($e instanceof OpenCReport\Expr) {
		echo "expr reprinted: " . $e->print();
		echo "expr nodes: " . $e->nodes() . PHP_EOL;

		$e->optimize();
		echo "expr optimized: " . $e->print();
		echo "expr nodes: " . $e->nodes();

		$r = $e->eval();
		$r->print();
		echo PHP_EOL;
	} else {
		echo $err . PHP_EOL;
	}

	echo PHP_EOL;
}
