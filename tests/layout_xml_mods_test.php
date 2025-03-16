<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

require_once 'test_common.php';

$array = [
	[ "id", "name", "property", "age", "adult" ]
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

if (!$o->parse_xml("layout_xml_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$r = get_first_report($o);
$out = $r->nodata();

for ($e = $out->get_first_element(); $e instanceof OpenCReport\OutputElement; $e = $e->get_next()) {
	if ($e->is_hline()) {
		$hline = $e->get_hline();
		$hline->set_color("'green'");
	} else if ($e->is_line()) {
		$l = $e->get_line();
		for ($le = $l->get_first_element(); $le instanceof OpenCReport\LineElement; $le = $le->get_next()) {
			if ($le->is_text()) {
				$t = $le->get_text();
				$t->set_value_string("xxx");
			}
		}
	}
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);
$o->execute();
$o->spool();
