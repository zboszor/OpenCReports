<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$exprs = [
	/* Legacy string formats */

	"format('apples and oranges', '')",
	"format('apples and oranges', nulls())",

	"format('apples and oranges', '%s')",
	"format('apples and oranges', 'You have some %s')",
	"format('apples and oranges', 'You have some %s and pears')",
	"format('apples and oranges', 'You have some: %s')",
	"format('apples and oranges', '% 10s')",
	"format('apples and oranges', 'You have some % 10s pears')",
	"format('apples', '% 10s')",
	"format('árvíztűrő tükörfúrógép', '%9s')",
	"format('árvíztűrő tükörfúrógép', '% 9s')",
	"format('hülye', '% 9s')",

	/* Legacy number formats */

	"format(12345.56789, '%d')",
	"format(-12345.56789, '%d')",

	"format(12345.56789, '%6.3d')",
	"format(-12345.56789, '%6.3d')",

	"format(12345.56789, '%16.3d')",
	"format(-12345.56789, '%16.3d')",

	"format(12345.56789, '% 16.3d')",
	"format(-12345.56789, '% 16.3d')",

	"format(12345.56789, '%-16.3d')",
	"format(-12345.56789, '%-16.3d')",

	"format(6, 'You have % 10d oranges')",

	/* Legacy datetime formats */

	"format(stodt('2022-05-08'), '')", /* default formatting */
	"format(stodt('2022-05-08'), '%Y-%m-%d')",

	"dtosf(stodt('2022-05-08'), '')", /* default formatting */
	"dtosf(stodt('2022-05-08'), '%Y-%m-%d')",

	/* New style string formats */

	"format('apples and oranges', 'You have some !&% 10s pears')",
	"format('apples and oranges', 'You have some !&{% 10s} pears')",

	/* New style number formats */

	"format(12345.56789, '!#%16.3d')",
	"format(-12345.56789, '!#%16.3d')",

	"format(12345.56789, '!#{%16.3d}')",
	"format(-12345.56789, '!#{%16.3d}')",

	"format(6, 'You have !#% 10d oranges')",
	"format(6, 'You have !#{% 10d} oranges')",

	/* printf(), yay!!! */

	"printf('You had %d %s on !@{%Y-%m-%d} and %d %s on !@{%Y-%m-%d} in your pocket.', 6, 'apples', stodt('2022-05-01'), 2, 'oranges', stodt('2022-05-02'))",

	/*
	 * NULL data is treated as empty string,
	 * regardless of the optional width specifier.
	 * Literals from the format string are printed as is.
	 */
	"printf('You had %d %s on !@{%Y-%m-%d} and %d %s on !@{%Y-%m-%d} in your pocket.', nulln(), nulls(), nulldt(), nulln(), nulls(), nulldt())",

	/*
	 * This will throw a "format error".
	 * Data has a type first, then the data may be NULL.
	 */
	"printf('You had %d %s on !@{%Y-%m-%d} and %d %s on !@{%Y-%m-%d} in your pocket.', nulln(), nulls(), nulldt(), nulln(), nulls(), nulls())",

	/* Invalid formats */

	/* Numeric format for a string */
	"format('apples and oranges', '%6.3d')",

	/* String format for a number */
	"format(-12345.56789, '% 10s')",
];

/* Locale aware formats */
$exprs1 = [
	/* Legacy number format with legacy thousand separator flag */
	"format(12345.56789, '%$16.3d')",
	"format(-12345.56789, '%$16.3d')",

	/* Legacy number format with standard thousand separator flag */
	"format(12345.56789, '%''16.3d')",
	"format(-12345.56789, '%''16.3d')",

	/* New style number format */

	"format(12345.56789, '!#%''16.3d')",
	"format(-12345.56789, '!#%''16.3d')",

	"format(12345.56789, '!#{%''16.3d}')",
	"format(-12345.56789, '!#{%''16.3d}')",

	/* New style monetary format */

	"format(12345.56789, '!\$%=*#60n')",
	"format(-12345.56789, '!\$%=*#60n')",

	"format(12345.56789, '!\${%=*#60n}')",
	"format(-12345.56789, '!\${%=*#60n}')",

	/* Legacy datetime format */

	"format(stodt('2022-05-08'), '')",
	"format(stodt('2022-05-08'), '%A')",

	"dtosf(stodt('2022-05-08'), '')",
	"dtosf(stodt('2022-05-08'), '%A')",

	/* New style datetime format */

	"format(stodt('2022-05-08'), '!@%Y-%m-%d')",
	"format(stodt('2022-05-08'), '!@%A')",

	"format(stodt('2022-05-08'), '!@{%Y-%m-%d}')",
	"format(stodt('2022-05-08'), '!@{%A}')",

	"dtosf(stodt('2022-05-08'), '!@%Y-%m-%d')",
	"dtosf(stodt('2022-05-08'), '!@%A')",

	"dtosf(stodt('2022-05-08'), '!@{%Y-%m-%d}')",
	"dtosf(stodt('2022-05-08'), '!@{%A}')",
];

$locales = [
	"C",
	"en_US.UTF-8",
	"en_GB.UTF-8",
	"fr_FR.UTF-8",
	"de_DE.UTF-8",
	"hu_HU.UTF-8",
];

foreach ($exprs as &$str) {
	echo "string: " . $str . PHP_EOL;
	unset($e);
	unset($err);
	[ $e, $err ] = $o->expr_parse($str);
	if ($e instanceof OpenCReport\Expr) {
		echo "expr reprinted: "; flush();
		$e->print(); flush();
		echo "expr nodes: " . $e->nodes() . PHP_EOL;

		$e->optimize();
		echo "expr optimized: "; flush();
		$e->print(); flush();
		echo "expr nodes: " . $e->nodes() . PHP_EOL;

		$r = $e->eval();
		$r->print(); flush();
	} else {
		echo $err . PHP_EOL;
	}


	echo PHP_EOL;
}

unset($o);

foreach ($locales as &$loc) {
	$o = new OpenCReport();
	$o->set_locale($loc);

	echo PHP_EOL . "Print formats in " . $loc . PHP_EOL . PHP_EOL;

	foreach ($exprs1 as &$str) {
		echo "string: " . $str . PHP_EOL;
		unset($e);
		unset($err);
		[ $e, $err ] = $o->expr_parse($str);
		if ($e instanceof OpenCReport\Expr) {
			echo "expr reprinted: "; flush();
			$e->print(); flush();
			echo "expr nodes: " . $e->nodes() . PHP_EOL;

			$e->optimize();
			echo "expr optimized: "; flush();
			$e->print();
			echo "expr nodes: " . $e->nodes() . PHP_EOL;

			$r = $e->eval();
			$r->print(); flush();
		} else {
			echo $err;
		}

		echo PHP_EOL;
	}

	unset($o);
}
