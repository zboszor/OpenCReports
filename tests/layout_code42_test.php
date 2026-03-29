<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$array = [
	[ "id", "name", "property", "age", "male", "adult" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "2", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "3", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "4", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "5", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "6", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "7", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "8", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "9", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "10", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "11", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "12", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "13", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "14", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "15", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "16", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "17", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "18", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "19", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "20", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "21", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "22", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "23", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "24", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "25", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "26", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "27", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "28", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "29", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "30", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "31", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "32", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "33", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "34", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "35", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "36", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "37", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "38", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "39", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "40", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "41", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "42", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "43", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "44", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "45", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "46", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "47", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "48", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "49", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "50", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "51", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "52", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "53", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "54", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "55", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "56", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "57", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "58", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "59", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "60", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "1", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "2", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "3", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "4", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "5", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "6", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "7", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "8", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "9", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "10", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "11", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "12", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "13", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "14", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "15", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "16", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "17", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "18", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "19", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "20", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "21", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "22", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "23", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "24", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "25", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "26", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "27", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "28", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "29", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "30", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "31", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "32", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "33", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "34", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "35", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "36", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "37", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "38", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "39", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "40", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "41", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "42", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "43", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "44", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "45", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "46", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "47", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "48", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "49", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "50", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "51", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "52", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "53", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "54", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
	[ "55", "Fred Flintstone", "strong", "31", "yes", "yes" ],
	[ "56", "Barney Rubble", "small", "29", "yes", "yes" ],
	[ "57", "Bamm-Bamm Rubble", "energetic", "2", "yes", "no" ],
	[ "58", "Wilma Flintstone", "charming", "28", "no", "yes" ],
	[ "59", "Betty Rubble", "beautiful", "27", "no", "yes" ],
	[ "60", "Pebbles Flintstone", "young", "5e-1", "no", "no" ],
];

$coltypes = [
	OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_STRING, OpenCReport::RESULT_STRING, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER, OpenCReport::RESULT_NUMBER
];

$o = new OpenCReport();

$ds = $o->datasource_add("myarray", "array");
$q = $ds->query_add("a", "array", "coltypes");

$p = $o->part_new();
$p->set_paper("A4");
$p->set_font_size("10.0");

/* Construct page header */

$out = $p->page_header();

$hl = $out->add_hline();
$hl->set_size("1");
$hl->set_color("'red'");

$l = $out->add_line();

$t = $l->add_text();
$t->set_value_string("Page header");

$hl = $out->add_hline();
$hl->set_size("1");
$hl->set_color("'red'");

/* Construct page footer */

$out = $p->page_footer();

$hl = $out->add_hline();
$hl->set_size("1");
$hl->set_color("'green'");

$l = $out->add_line();

$t = $l->add_text();
$t->set_value_string("Page footer");

$hl = $out->add_hline();
$hl->set_size("1");
$hl->set_color("'green'");

/* End of page header / footer */

$pr = $p->row_new();

$pd = $pr->column_new();

$r = $pd->report_new();
$r->set_main_query($q);
$r->set_font_size(12.0);

/* Construct field header */

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
$t->set_value_string("Property");
$t->set_width("10");

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
$t->set_alignment("'right'");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("name");
$t->set_width("20");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("property");
$t->set_width("10");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("age");
$t->set_width("6");
$t->set_alignment("'right'");

$t = $l->add_text();
$t->set_width("1");

$t = $l->add_text();
$t->set_value_expr("adult ? 'yes' : 'no'");
$t->set_width("5");
$t->set_alignment("'center'");

/* Construct field footer */

$out = $r->field_footer();

$hl = $out->add_hline();
$hl->set_size("1");
$hl->set_color("'black'");

$l = $out->add_line();

$t = $l->add_text();
$t->set_value_string("This is a field footer before the page ends.");

$hl = $out->add_hline();
$hl->set_size("1");
$hl->set_color("'black'");

/* End of field header / details / footer */

$o->set_output_format(OpenCReport::OUTPUT_PDF);
$o->execute();
$o->spool();
