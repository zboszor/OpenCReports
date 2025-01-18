<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$exprs = [
	/* Good syntax expressions */

	/* Numeric constants */
	"1",
	"-1",
	"1.235",
	"-1.235",
	"0.1234",
	"-0.1234",
	".1234",
	"-.1234",
	"1e5",
	"-1e5",
	"1e-5",
	"-1e-5",
	"1.235e5",
	"-1.235e5",
	"1.235e-5",
	"-1.235e-5",
	"0.1234e5",
	"0.1234e-5",
	"-0.1234e5",
	"-0.1234e-5",
	".1234e5",
	".1234e-5",
	"-.1234e5",
	"-.1234e-5",
	"1E5",
	"-1E5",
	"1E-5",
	"-1E-5",
	"1.235E5",
	"-1.235E5",
	"1.235E-5",
	"-1.235E-5",
	"0.1234E5",
	"0.1234E-5",
	"-0.1234E5",
	"-0.1234E-5",
	".1234E5",
	".1234E-5",
	"-.1234E5",
	"-.1234E-5",

	/* String literals */
	"'apple'''",
	"'apple'''''",
	"'apple''xx'",
	"'apple''''xx'",
	'"apple"""',
	'"apple"""""',
	'"apple""xx"',
	'"apple""""xx"',
	'"apple"' . PHP_EOL . '"vv"',
	'"apple' . PHP_EOL . 'qq"' . PHP_EOL . PHP_EOL . '"_hul"',
	"'apple\nqq'\n\n'_hul'",
	"\"app'le\" 'b\"b' \"cc\"",
	"\"app'le\"'b\"b'\"cc\"",

	/* Boolean constants */
	"true",
	"false",
	"yes",
	"no",

	/* Basic identifiers */
	"a",
	"_",
	"_a",
	"a_",
	"aa",
	"_aa",
	"aa_",
	"a_a",
	"apple",
	"apple1",
	"app1e",

	/* Dot-prefixed basic identifiers */
	".a",
	"._",
	".a_",
	"._a",
	".aa",
	"._aa",
	".aa_",
	".a_a",
	".apple",
	".apple1",
	".app1e",

	/* Dot-prefixed quoted identifiers */
	".'a'",
	".'_'",
	".'a_'",
	".'_a'",
	".'aa'",
	".'_aa'",
	".'aa_'",
	".'a_a'",
	".'apple'",
	".'apple1'",
	".'app1e'",
	".'app-e'",
	".'app e'",
	".' '",
	/* Invalid identifiers */
	".''",
	'.""',

	/* Domain-qualified identifiers */
	"a.a",
	"_._",
	"a_.a_",
	"_a._a",
	"aa.aa",
	"_aa._aa",
	"aa_.aa_",
	"a_a.a_a",
	"apple.apple",
	"apple1.apple1",
	"app1e.app1e",

	/* Domain-qualified quoted identifiers */
	"'a'.'a'",
	"'_'.'_'",
	"'a_'.'a_'",
	"'_a'.'_a'",
	"'aa'.'aa'",
	"'_aa'.'_aa'",
	"'aa_'.'aa_'",
	"'a_a'.'a_a'",
	"'apple'.'apple'",
	"'apple1'.'apple1'",
	"'app1e'.'app1e'",
	"'app-e'.'app-e'",
	"'app e'.'app e'",
	"' '.' '",
	/* Invalid identifiers */
	"''.apple",
	'"".apple',
	"''.'apple'",
	'"".' . "'apple'",
	"'apple'.''",
	'"apple".""',
	"''.''",
	'"".""',

	/* Basic identifiers with unary plus or unary minus */
	"+a",
	"-_",
	"+_a",
	"-a_",
	"+aa",
	"-_aa",
	"+aa_",
	"-a_a",
	"+apple",
	"-apple1",
	"+app1e",

	/* Dot-prefixed basic identifiers */
	"+.a",
	"-._",
	"+.a_",
	"-._a",
	"+.aa",
	"-._aa",
	"+.aa_",
	"-.a_a",
	"+.apple",
	"-.apple1",
	"+.app1e",

	/* Dot-prefixed quoted identifiers */
	"+.'a'",
	"-.'_'",
	"+.'a_'",
	"-.'_a'",
	"+.'aa'",
	"-.'_aa'",
	"+.'aa_'",
	"-.'a_a'",
	"+.'apple'",
	"-.'apple1'",
	"+.'app1e'",
	"-.'app-e'",
	"+.'app e'",
	"-.' '",
	/* Invalid dot-prefixed quoted identifiers */
	"+.''",
	"+.\"\"",

	/* Domain-qualified identifiers */
	"+a.a",
	"-_._",
	"+a_.a_",
	"--_a._a",
	"+aa.aa",
	"-_aa._aa",
	"+aa_.aa_",
	"-a_a.a_a",
	"+apple.apple",
	"-apple1.apple1",
	"+app1e.app1e",

	/* Domain-qualified quoted identifiers */
	"+'a'.'a'",
	"-'_'.'_'",
	"+'a_'.'a_'",
	"-'_a'.'_a'",
	"+'aa'.'aa'",
	"-'_aa'.'_aa'",
	"+'aa_'.'aa_'",
	"-'a_a'.'a_a'",
	"+'apple'.'apple'",
	"-'apple1'.'apple1'",
	"+'app1e'.'app1e'",
	"-'app-e'.'app-e'",
	"+'app e'.'app e'",
	"-' '.' '",
	"+''.''",
	'+"".""',

	/* Identifiers in special domains */
	"m.envvarname",
	'm."env var name"',
	"r.envvarname",
	'r."env var name"',
	"v.repvarname",
	'v."rep var name"',

	/* Domain-qualified quoted identifiers with the special domain losing its meaning */
	'"m".envvarname',
	'"m"."env var name"',
	'"r".envvarname',
	'"r"."env var name"',
	'"v".repvarname',
	'"v"."rep var name"',

	/* Weak reserved words as identifiers */
	".true",
	".false",
	".yes",
	".no",
	"false.false",
	"false.true",
	"false.no",
	"false.yes",
	"true.false",
	"true.true",
	"true.no",
	"true.yes",
	"no.false",
	"no.true",
	"no.yes",
	"no.no",
	"yes.false",
	"yes.true",
	"yes.no",
	"yes.yes",

	/* Reserved words as quoted identifiers */
	".'or'",
	'."and"',

	/* Identifiers with UTF-8 localized characters without quoting */
	"hülye",
	"nagyképű.hólyag",

	/* Simple operators */
	"1+2",
	"1+ 2",
	"1 +2",
	"1 + 2",
	"1+a",
	"1+ a",
	"1 +a",
	"1 + a",
	"1+aa",
	"1+ aa",
	"1 +aa",
	"1 + aa",

	/* Simple operators with unary plus */
	"1+ +2",
	"1+ ++2",
	"1++ + ++2",
	"1+++ ++2",
	"1++ +++2",
	"1+++++2",
	"1+ +2",
	"1 ++2",
	"1 + +2",
	"1 + + 2",
	"1+ +a",
	"1 ++a",
	"1 + +a",
	"1 + + a",
	"1++aa",
	"1+ +aa",
	"1 ++aa",
	"1 + +aa",
	"1 + + aa",

	/* Simple operators with unary minus */
	"1+-2",
	"1+ -2",
	"1 +-2",
	"1 + -2",
	"1 + - 2",
	"1+-a",
	"1+ -a",
	"1 +-a",
	"1 + -a",
	"1 + - a",
	"1+-aa",
	"1+ -aa",
	"1 +-aa",
	"1 + -aa",
	"1 + - aa",

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

	/* Comparison operators */
	"a == b",
	"a = b",
	"a != b",
	"a <> b",
	"a < b",
	"a <= b",
	"a > b",
	"a >= b",

	/* Bitshift operators */
	"a << b",
	"a >> b",

	/* Trinary operator */
	"q.a ? 'true' : 'false'",
	"!q.a ? -1.5 : -3",
	"q.a ? +1.5 : +3",
	"!q.a ? 1 + 2 * 3 : (1 + 2) * 3",

	/* Trinary operator as function call */
	"iif(q.a,'true','false')",
	"iif \n\t (q.a,'true','false')",
	"iif(!q.a, -1.5, -3)",
	"iif(q.a, +1.5, +3)",
	"iif(!q.a, 1 + 2 * 3, (1 + 2) * 3)",

	/* Logical operators */
	"a and b",
	"a or b",
	"a or b and c",
	"(a or b) and c",
	"a && b",
	"a || b",
	"a || b && c",
	"(a || b) && c",

	/* All arithmetic operators */
	"a + b",
	"a - b",
	"a * b",
	"a / b",
	"a % b",
	"a ^ b",
	"a | b",
	"a & b",
	"!a",
	"~a",
	"b2 or !b2",

	/* Function calls and implicit multiplication */
	"myfunc(a,b,c,d,e,f)",
	"myfunc(a)",
	"1 + myfunc(a,b,c,d,e,f) + 3",
	"1 + myfunc(a) + 2",
	"8/2(2+2)",
	"1/2x",
	"1/2e",
	"2e",
	"-2e",
	"2e-1",
	"-2e-1",
	"2e - 1",
	"-2e - 1",
	"-2e-1e",
	"--2e-1e",
	"(2+2).2",
	"(2+2).a",
	"(2+2)a",
	"1/(1+1)(2+2)",

	/* Syntax error - implicit multiplication must not have whitespace */
	"1 / ( 1 + 1 ) ( 2 + 2 )",

	/* Simple expressions with bad syntax */
	"a.b|",
	"c.d;",
	".m.envvarname",
	".v.repvarname",
	".or",
	".and",
	"1++2",
	"1++a",

	/* More complex expressions with bad syntax */
	"1++2 ? yes : false",
	"yes ? 1++2 : 2",
	"yes ? 1 : 1++2",
	"add(1, 2, 3, 4, 5, 6, 7++8)",
	/* Missing parentheses */
	"add(1, 2, 3, 4, 5, (6))",
	"add(1, 2, 3, 4, 5, (6, 7)"
];

$o = new OpenCReport();

foreach ($exprs as &$str) {
	echo "string: " . $str . PHP_EOL;

	$e = $o->expr_parse($str);
	if ($e instanceof OpenCReport\Expr) {
		echo "expr reprinted: "; flush();
		$e->print();
		echo "expr nodes: " . $e->nodes() . PHP_EOL;
	} else {
		echo $o->expr_error() . PHP_EOL;
	}

	echo PHP_EOL;
}
