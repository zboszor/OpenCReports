<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "name", "age", "adult" ],
	[ "1", "Fred Flintstone", "31", "yes" ],
	[ "2", "Wilma Flintstone", "28", "yes" ],
	[ "3", "Pebbles Flintstone", "5e-1", "no" ],
	[ "4", "Betty Rubble", "27", "yes" ],
	[ "5", "Barney Rubble", "29", "yes" ],
	[ "6", "Mr. George Slate", "53", "yes" ],
	[ "7", "Joe Rockhead", "33", "yes" ],
	[ "8", "Sam Slagheap", "37", "yes" ],
	[ "9", "The Great Gazoo", "1200", "yes" ],
];

$coltypes = [ OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER ];

$o = new OpenCReport();

$abs_builddir = getenv("abs_builddir");
if (is_bool($abs_builddir))
	$abs_builddir = getcwd();
$localedir = OpenCReport::canonicalize_path($abs_builddir) . "/locale";

$o->bindtextdomain("translate_test", $localedir);
$o->set_locale("hu_HU");

$ds = $o->datasource_add("myarray", "array");
$q = $ds->query_add("a", "array", "coltypes");

$p = $o->part_new();
$p->set_paper("A4");
$p->set_orientation("landscape");
$p->set_font_size("10.0");

/* Construct page header */
$out = $p->page_header();

$hl = $out->add_hline();
$hl->set_size("2");
$hl->set_color("'black'");

$l = $out->add_line();
$l->set_font_size("14");

$t = $l->add_text();
$t->set_value_expr("printf(translate('Page header: %d / %d'), r.pageno, r.totpages)");
$t->set_alignment("'right'");

$hl = $out->add_hline();
$hl->set_size("2");
$hl->set_color("'black'");

/* Construct page footer */
$out = $p->page_footer();

$hl = $out->add_hline();
$hl->set_size("2");
$hl->set_color("'black'");

$l = $out->add_line();
$l->set_font_size("14");

$t = $l->add_text();
$t->set_value_expr("printf(translate('Page footer: %d / %d'), r.pageno, r.totpages)");
$t->set_alignment("'right'");

$hl = $out->add_hline();
$hl->set_size("2");
$hl->set_color("'black'");

/* End of page header / footer */

$pr = $p->row_new();

$pd = $pr->column_new();
$pd->set_border_width("2");
$pd->set_border_color("bobkratz");

$r = $pd->report_new();
$r->set_main_query($q);
$r->set_font_size("12.0");

/* Construct field headers */

$out = $r->field_header();

$hl = $out->add_hline();
$hl->set_size("1");
$hl->set_color("'black'");

$l = $out->add_line();
$l->set_bgcolor("'0xe5e5e5'");

$t = $l->add_text();
$t->set_value_string("ID");
$t->set_width("4");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_string("Greetings");
$t->set_translate("yes");
$t->set_width("50");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_string("Age");
$t->set_translate("yes");
$t->set_width("7");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_string("Adult");
$t->set_translate("yes");
$t->set_width("7");
$t->set_alignment("'center'");

$hl = $out->add_hline();
$hl->set_size("1");
$hl->set_color("'black'");

/* Construct field details */

$out = $r->field_details();

$l = $out->add_line();
$l->set_bgcolor("iif(r.detailcnt%2,'0xe5e5e5','white')");

$t = $l->add_text();
$t->set_value_expr("id");
$t->set_width("4");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("name");
$t->set_format("'Happy birthday, %s!'");
$t->set_translate("yes");
$t->set_width("50");
$t->set_alignment("'justified'");
$t->set_memo("yes");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("age");
$t->set_width("7");
$t->set_format("'%.2d'");
$t->set_alignment("'right'");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("adult ? 'yes' : 'no'");
$t->set_translate("yes");
$t->set_width("7");
$t->set_alignment("'center'");

/* End of field headers / details */

$o->set_output_format(OpenCReport::OUTPUT_PDF);
$o->execute();
$o->spool();
