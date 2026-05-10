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
if (!$o->parse_xml("layout_xml53_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$srcdir = getenv("abs_srcdir");
if (is_bool($srcdir))
	$srcdir = getcwd();
$srcdir = OpenCReport::canonicalize_path($srcdir);
$o->add_search_path($srcdir . "/images/images");

$o->set_output_format(OpenCReport::OUTPUT_PDF);
$o->execute();
$o->spool();
