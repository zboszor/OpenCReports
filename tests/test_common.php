<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

function print_query_columns(&$q, $name = NULL) {
	$qr = $q->get_result();
	echo "Query columns" . (is_null($name) ? "" : " (" . $name . ")") . ":" . PHP_EOL;
	for ($i = 0; $i < $qr->columns(); $i++)
		echo $i . ": '" . $qr->column_name($i) . "'" . PHP_EOL;
}

function print_result_row(string $name, OpenCReport\QueryResult &$qr) {
	echo "Query: '" . $name . "':" . PHP_EOL;
	for ($i = 0; $i < $qr->columns(); $i++) {
		$r = $qr->column_result($i);
		$isnull = $r->is_null();
		$s = $r->get_string();

		echo "\tCol #" . $i . ": '" . $qr->column_name($i) . "': string value: " . (($isnull || is_null($s)) ? "NULL" : $s);
		if (!$isnull && $r->is_number()) {
			echo " (converted to number: " . $r->get_number("%.6RF"). ")"; flush();
			//echo " (converted to number: " . $r->get_number() . ")"; flush();
		}
		echo PHP_EOL;
		unset($r);
	}
}

function print_part_reports(string $name, OpenCReport\Part &$p) {
	echo "part " . $name . ":" . PHP_EOL;
	for ($row = $p->row_get_first(), $i = 0; isset($row); $row = $row->get_next(), $i++) {
		echo "row ". $i . " reports:"; flush();
		$j = 0;
		for ($col = $row->column_get_first(); isset($col); $col = $col->get_next()) {
			for ($rpt = $col->report_get_first(); isset($rpt); $rpt = $rpt->get_next(), $j++)
				echo " " . $j; flush();
		}
		echo PHP_EOL;
	}
}

function get_first_report(OpenCReport &$o): OpenCReport\Report {
	return $o->part_get_first()->row_get_first()->column_get_first()->report_get_first();
}

function create_expr(OpenCReport &$o, &$e, $str, $print = true) {
	$e = $o->expr_parse($str);
	if ($print)
		$e->print();
}

function create_exprs(OpenCReport &$o, bool $resolve = true) {
	global $id;
	global $name;
	global $age;
	global $adult;

	$id = 0;
	create_expr($o, $id, "id");

	$name = 0;
	create_expr($o, $name, "name");

	$age = 0;
	create_expr($o, $age, "age * 2");

	$adult = 0;
	create_expr($o, $adult, "a.adult", false);

	if ($resolve) {
		$id->resolve();
		$name->resolve();
		$age->resolve();
		$adult->resolve();
	}
}

function resolve_exprs() {
	global $id;
	global $name;
	global $age;
	global $adult;

	$id->resolve();
	$name->resolve();
	$age->resolve();
	$adult->resolve();
}

function create_exprs_with_val(OpenCReport &$o, bool $resolve = true) {
	global $id;
	global $name;
	global $age;
	global $adult;

	$id = $o->expr_parse("id");
	$id->print();

	$name = $o->expr_parse("name");
	$name->print();

	$age = $o->expr_parse("val(age) * 2");
	$age->print();

	$adult = $o->expr_parse("val(a.adult)");

	if ($resolve) {
		$id->resolve();
		$name->resolve();
		$age->resolve();
		$adult->resolve();
	}
}

function eval_print_expr(&$e) {
	echo "Expression: "; flush();
	$e->print();
	$r = $e->eval();
	echo "Evaluated: "; flush();
	$r->print();
}

function free_exprs() {
	global $id;
	global $name;
	global $age;
	global $adult;

	$id->free();
	$name->free();
	$age->free();
	$adult->free();
}
