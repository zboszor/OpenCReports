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

function part_iter_cb() {
	global $yields;

	$yields[3][0] = "MIKEROTH";
	$yields[3][1] = "CALL";
	$yields[3][2] = "BACK";

	global $r;
	rlib_query_refresh($r);
}

function precalc_done_cb() {
	global $yields;

	$yields[3][0] = "Pasta";
	$yields[3][1] = "1";
	$yields[3][2] = "190";

	global $r;
	rlib_query_refresh($r);
}

$r = rlib_init();
rlib_add_datasource_array($r, "local_array");

/* For m.sillypants in the report */
$sillypants = "5";

rlib_add_query_as($r, "local_array", "yields", "yields");
rlib_add_query_as($r, "local_array", "coupons", "coupons");
rlib_add_query_as($r, "local_array", "deposits", "deposits");
rlib_add_query_as($r, "local_array", "petty_cash", "petty_cash");
rlib_add_query_as($r, "local_array", "misc_income", "misc_income");
rlib_add_query_as($r, "local_array", "inv_transfer", "inv_transfer");
rlib_add_query_as($r, "local_array", "inventory", "inventory");

rlib_add_report($r, "layout_ocrpt_fixed_part_test.xml");

rlib_signal_connect($r, "part_iteration", "part_iter_cb");
rlib_signal_connect($r, "precalculation_done", "precalc_done_cb");

rlib_set_output_format_from_text($r, "pdf");

rlib_execute($r);
rlib_spool($r);
