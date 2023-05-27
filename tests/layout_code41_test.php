<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$LOREMIPSUM = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

$array = [
	[ "id", "name", "image", "age", "adult" ],
	[ "1", "Fred Flintstone", "images/images/matplotlib.svg", "31", "yes" ],
	[ "2", "Wilma Flintstone " . $LOREMIPSUM, "images/images/matplotlib.svg", "28", "yes" ],
	[ "3", "Pebbles Flintstone", NULL, "5e-1", "no" ],
	[ "4", "Betty Rubble " . $LOREMIPSUM, "images/images/matplotlib.svg", "27", "yes" ],
	[ "5", "Barney Rubble", "images/images/matplotlib.svg", "29", "yes" ],
	[ "6", "Mr. George Slate", "images/images/matplotlib.svg", "53", "yes" ],
	[ "7", "Joe Rockhead", "images/images/matplotlib.svg", "33", "yes" ],
	[ "8", "Sam Slagheap", "images/images/matplotlib.svg", "37", "yes" ],
	[ "9", "The Great Gazoo " . $LOREMIPSUM, "images/images/matplotlib.svg", "1200", "yes" ],
];

/* Intentionally wider than $array - last element is ignored */
$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$srcdir = getenv("abs_srcdir");
if (is_bool($srcdir))
	$srcdir = getcwd();
$srcdir = OpenCReport::canonicalize_path($srcdir);

$o = new OpenCReport();

$o->add_search_path($srcdir);

$ds = $o->datasource_add_array("myarray");
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
$t->set_value_expr("printf('Page header: %d / %d', r.pageno, r.totpages)");
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
$t->set_value_expr("printf('Page footer: %d / %d', r.pageno, r.totpages)");
$t->set_alignment("'right'");

$hl = $out->add_hline();
$hl->set_size("2");
$hl->set_color("'black'");

/* End of page header / footer */

$pr = $p->row_new();

$pd = $pr->column_new();
$pd->set_detail_columns(2);
$pd->set_column_padding(0.2);
$pd->set_border_width(2);
$pd->set_border_color("bobkratz");

$r = $pd->report_new();
$r->set_main_query($q);
$r->set_font_size(12.0);

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
$t->set_value_string("Name");
$t->set_width("20");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_string("Image");
$t->set_width("8");
$t->set_alignment("'center'");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_string("Age");
$t->set_width("6");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_string("Adult");
$t->set_width("5");
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
$t->set_width("20");
$t->set_alignment("'justified'");
$t->set_memo(true);

$t = $l->add_text();
$t->set_width("1");

$im = $l->add_image();
$im->set_value("image");
$im->set_text_width("8");
$im->set_alignment("'center'");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("age");
$t->set_width("6");
$t->set_format("'%.2d'");
$t->set_alignment("'right'");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("adult ? 'yes' : 'no'");
$t->set_width("5");
$t->set_alignment("'center'");

/* End of field headers / details */

$o->set_output_format(OpenCReport::OUTPUT_PDF);
$o->execute();
$o->spool();
