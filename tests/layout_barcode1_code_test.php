<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "bc", "type" ],
	[ "1",  "123456789010", "ean-13" ],
	[ "2",  "123456789011", "ean-13" ],
	[ "3",  "123456789012", "ean-13" ],
	[ "4",  "123456789013", "ean-13" ],
	[ "5",  "123456789010A", "code39" ],
	[ "6",  "123456789011B", "code39" ],
	[ "7",  "123456789012C", "code39" ],
	[ "8",  "123456789013D", "code39" ],
	[ "9",  "123456789010Aa", "code128b" ],
	[ "10", "123456789011Ba", "code128b" ],
	[ "11", "123456789012Ca", "code128b" ],
	[ "12", "123456789013Da", "code128b" ],
	[ "13", "123456789010", "code128c" ],
	[ "14", "123456789011", "code128c" ],
	[ "15", "123456789012", "code128c" ],
	[ "16", "123456789013", "code128c" ],
	[ "17", "123456789010Aa", "code128" ],
	[ "18", "123456789011Ba", "code128" ],
	[ "19", "123456789012Ca", "code128" ],
	[ "20", "123456789013Da", "code128" ],
];

$o = new OpenCReport();
$ds = $o->datasource_add("array", "array");
$q = $ds->query_add("data", "array");

$p = $o->part_new();
$pr = $p->row_new();
$pd = $pr->column_new();
$r = $pd->report_new();

$fh = $r->field_header();

$l = $fh->add_line();

$t = $l->add_text();
$t->set_value_string("ID");
$t->set_width("4");
$t->set_alignment("'right'");

$t = $l->add_text();
$t->set_width("1");
$t->set_bgcolor("'red'");

$t = $l->add_text();
$t->set_value_string("String");
$t->set_width("20");

$t = $l->add_text();
$t->set_width("1");
$t->set_bgcolor("'red'");

$t = $l->add_text();
$t->set_value_string("Barcode");
$t->set_width( "20");
$t->set_alignment("'center'");

$t = $l->add_text();
$t->set_width("1");
$t->set_bgcolor("'red'");

$hl = $fh->add_hline();
$hl->set_color("'red'");

$fd = $r->field_details();

$l = $fd->add_line();

$t = $l->add_text();
$t->set_value_expr("id");
$t->set_width("4");
$t->set_alignment("'right'");

$t = $l->add_text();
$t->set_width("1");
$t->set_bgcolor("'red'");

$t = $l->add_text();
$t->set_value_expr("bc");
$t->set_width("20");

$t = $l->add_text();
$t->set_width("1");
$t->set_bgcolor("'red'");

$bc = $l->add_barcode();
$bc->set_value("bc");
$bc->set_type("type");
$bc->set_width("20");

$t = $l->add_text();
$t->set_width("1");
$t->set_bgcolor("'red'");

$hl = $fd->add_hline();
$hl->set_color("'red'");

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
