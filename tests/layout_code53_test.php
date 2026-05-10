<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$main_array = [
	[ "dummy" ],
	[ "1" ],
];

$main_coltypes = [
	OpenCReport::RESULT_NUMBER
];

$gen_array = [
	[ "element_type", "value",          "bgcolor", "color", "font_name", "bold", "italic", "bctype",  "imgtype", "width", "height", "align",    "suppress" ],
	[ "text",         "1",              null,      null,    "Courier",   null,   "0",      null,      null,      "6",     "2",      "left",     null       ],
	[ "text",         "2",              null,      null,    null,        "1",    "1",      null,      null,      "6",     "2",      "right",    null       ],
	[ "barcode",      "123456789012",   "red",     "black", null,        null,   null,     "ean-13",  null,      null,    "50",     "center",   null       ],
	[ "text",         "4",              null,      null,    "Courier",   "0",    null,     null,      null,      "6",     "2",      "left",     "1"        ],
	[ "text",         "5",              null,      null,    null,        null,   "0",      null,      null,      "6",     "2",      "right",    null       ],
	[ "image",        "matplotlib.svg", "green",   null,    null,        null,   null,     null,      "svg",     "20",    "50",     "center",   null       ],
	[ "text",         "7",              null,      null,    "Courier",   "1",    "1",      null,      null,      "6",     "2",      "left",     null       ],
	[ "text",         "8",              null,      null,    null,        "0",    null,     null,      null,      "6",     "2",      "right",    "1"        ],
	[ "barcode",      "123456789012",   null,      "black", null,        null,   null,     "ean-13",  null,      null,    "50",     "center",   null       ],
	[ "text",         "10",             null,      null,    "Courier",   null,   "0",      null,      null,      "6",     "2",      "left",     null       ],
	[ "text",         "11",             null,      null,    null,        "1",    "1",      null,      null,      "6",     "2",      "right",    null       ],
	[ "image",        "matplotlib.svg", null,      null,    null,        null,   null,     null,      "svg",     "20",    "50",     "center",   "1"        ],
];

$gen_coltypes = [
	OpenCReport::RESULT_STRING, /* element_type */
	OpenCReport::RESULT_STRING, /* value        */
	OpenCReport::RESULT_STRING, /* bgcolor      */
	OpenCReport::RESULT_STRING, /* color        */
	OpenCReport::RESULT_STRING, /* font_name    */
	OpenCReport::RESULT_NUMBER, /* bold         */
	OpenCReport::RESULT_NUMBER, /* italic       */
	OpenCReport::RESULT_STRING, /* bctype       */
	OpenCReport::RESULT_STRING, /* imgtype      */
	OpenCReport::RESULT_NUMBER, /* width        */
	OpenCReport::RESULT_NUMBER, /* height       */
	OpenCReport::RESULT_STRING, /* align        */
	OpenCReport::RESULT_NUMBER, /* suppress     */
];

$o = new OpenCReport();

$ds = $o->datasource_add("myarray", "array");
$qa = $ds->query_add("a", "main_array", "main_coltypes");
$h  = $ds->query_add("h", "gen_array", "gen_coltypes");

$p = $o->part_new();
$p->set_paper("'A4'");
$p->set_orientation("'landscape'");

$pr = $p->row_new();

$pd = $pr->column_new();

$r = $pd->report_new();
$r->set_main_query($qa);

/* Construct field details */

$out = $r->field_details();

$gl = $out->add_genline();
$gl->set_query($h);
$gl->set_element_type("h.element_type");
$gl->set_line_font_name("'Times New Roman'");
$gl->set_line_font_size("14");
$gl->set_line_bold("yes");
$gl->set_line_italic("yes");
$gl->set_value("h.value");
$gl->set_bgcolor("h.bgcolor");
$gl->set_color("h.color");
$gl->set_font_name("h.font_name");
$gl->set_bold("h.bold");
$gl->set_italic("h.italic");
$gl->set_barcode_type("h.bctype");
$gl->set_image_type("h.imgtype");
$gl->set_width("h.width");
$gl->set_height("h.height");
$gl->set_alignment("h.align");
$gl->set_suppress("h.suppress");

$srcdir = getenv("abs_srcdir");
if (is_bool($srcdir))
	$srcdir = getcwd();
$srcdir = OpenCReport::canonicalize_path($srcdir);
$o->add_search_path($srcdir . "/images/images");

$o->set_output_format(OpenCReport::OUTPUT_PDF);
$o->execute();
$o->spool();
