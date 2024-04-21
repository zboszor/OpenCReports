<?php
/*
 * OpenCReports test
 *
 * This is a copy of the "fixed_part" example from RLIB
 * ported to C code. The XML part is identical.
 *
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$yields = [
	[ "item", "portions", "eqv" ],
	[ "Hamburger", "1", "100" ],
	[ "Chicken", "1", "150" ],
	[ "Pasta", "1", "190" ]
];

$coupons = [
	[ "name", "actual_count", "actual_amount", "computed_count", "computed_amount" ],
	[ "DOLLAR OFF", "0", "0", "0", "0" ],
	[ "FREE", "0", "0", "3", "-10.23" ],
	[ "SENIOR 10%", "0", "0", "0", "0" ],
	[ "15%", "0", "0", "0", "0" ]
];

$deposits = [
	[ "time", "bag_id", "manager_id", "amount" ],
	[ "12:00", "DEPOSIT 1", "101", "2000.00" ],
	[ "4:00", "DEPOSIT 2", "102", "1000.00" ]
];

$petty_cash = [
	[ "time", "name", "amount" ],
	[ "1:00", "Sugar", "20.00" ],
	[ "2:00", "Pants", "40.00" ]
];

$misc_income = [
	[ "time", "name", "amount" ],
	[ "1:00", "Birthday Party", "20.00" ]
];

$inv_transfer = [
	[ "qty", "name", "from", "to" ],
	[ "100", "Buns", "1121", "4452" ],
	[ "400", "Pattys", "1121", "4499" ]
];

$inventory = [
	[ "num", "name", "amount", "unit", "open", "usage", "received", "transfer_in", "transfer_out", "waste", "pysical_count" ],
	[ "1", "BUN, REG/PREMIUM", ".50", "DOZEN", "176", "0", "0", "0", "0", "0", "0" ],
	[ "2", "BUN, KAISER", ".70", "DOZEN", "176", "0", "0", "0", "0", "0", "0" ]
];

function part_iter_cb(OpenCReport $o, OpenCReport\Part $p) {
	global $yields;

	$yields[3][0] = "MIKEROTH";
	$yields[3][1] = "CALL";
	$yields[3][2] = "BACK";

	$o->query_refresh();
}

function precalc_done_cb(OpenCReport $o) {
	global $yields;

	$yields[3][0] = "Pasta";
	$yields[3][1] = "1";
	$yields[3][2] = "190";

	$o->query_refresh();
}

$o = new OpenCReport();
$ds = $o->datasource_add("local_array", "array");

/* For m.sillypants in the report */
$sillypants = "5";

$ds->query_add("yields", "yields");
$ds->query_add("coupons", "coupons");
$ds->query_add("deposits", "deposits");
$ds->query_add("petty_cash", "petty_cash");
$ds->query_add("misc_income", "misc_income");
$ds->query_add("inv_transfer", "inv_transfer");
$ds->query_add("inventory", "inventory");

if (!$o->parse_xml("layout_ocrpt_fixed_part_test.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

$p = $o->part_get_first();
$p->add_iteration_cb("part_iter_cb");
$o->add_precalculation_done_cb("precalc_done_cb");

$o->set_output_format(OpenCReport::OUTPUT_PDF);

$o->execute();
$o->spool();
