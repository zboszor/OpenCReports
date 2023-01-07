<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

function print_query_columns(&$q) {
	$qr = $q->get_result();
	echo "Query columns:" . PHP_EOL;
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
	for ($pri = $p->row_iter_start(), $i = 0; !$pri->finished(); $pri->next(), $i++) {
		$pr = $pri->get_row();

		printf("row %d reports:", i);
		$j = 0;
		for ($pdi = $pr->column_iter_start(); !$pdi->finished(); $pdi->next()) {
			$pd = $pdi->get_column();

			for ($ri = $pd->report_iter_start(); !$ri->finished(); $ri->next(), $j++)
				echo " " . $j;
		}
		echo PHP_EOL;
	}
}

function get_first_report(OpenCReport &$o): OpenCReport\Report {
	$pi = $o->part_iter_start();
	$p = $pi->get_part();

	$pri = $p->row_iter_start();
	$pr = $pri->get_row();

	$pdi = $pd->column_iter_start();
	$pd = $pdi->get_column();

	$ri = $pd->report_iter_start();
	$r = $ri->get_report();

	return $r;
}

function create_exprs(OpenCReport &$o, bool $resolve = true) {
	global $id;
	global $name;
	global $age;
	global $adult;

	$id = $o->expr_parse("id");
	$id->print();

	$name = $o->expr_parse("name");
	$name->print();

	$age = $o->expr_parse("age * 2");
	$age->print();

	$adult = $o->expr_parse("a.adult");

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
