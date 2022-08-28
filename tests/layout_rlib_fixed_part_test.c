/*
 * OpenCReports test
 *
 * This is a copy of the "fixed_part" example from RLIB
 * ported to C code. The XML part is identical.
 *
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
#include "test_common.h"

char *yields[4][3] = {
	{ "item", "portions", "eqv" },
	{ "Hamburger", "1", "100" },
	{ "Chicken", "1", "150" },
	{ "Pasta", "1", "190" }
};

char *coupons[5][5] = {
	{ "name", "actual_count", "actual_amount", "computed_count", "computed_amount" },
	{ "DOLLAR OFF", "0", "0", "0", "0" },
	{ "FREE", "0", "0", "3", "-10.23" },
	{ "SENIOR 10%", "0", "0", "0", "0" },
	{ "15%", "0", "0", "0", "0" }
};

char *deposits[3][4] = {
	{ "time", "bag_id", "manager_id", "amount" },
	{ "12:00", "DEPOSIT 1", "101", "2000.00" },
	{ "4:00", "DEPOSIT 2", "102", "1000.00" }
};

char *petty_cash[3][3] = {
	{ "time", "name", "amount" },
	{ "1:00", "Sugar", "20.00" },
	{ "2:00", "Pants", "40.00" }
};

char *misc_income[2][3] = {
	{ "time", "name", "amount" },
	{ "1:00", "Birthday Party", "20.00" }
};

char *inv_transfer[3][4] = {
	{ "qty", "name", "from", "to" },
	{ "100", "Buns", "1121", "4452" },
	{ "400", "Pattys", "1121", "4499" }
};

char *inventory[3][11] = {
	{ "num", "name", "amount", "unit", "open", "usage", "received", "transfer_in", "transfer_out", "waste", "pysical_count" },
	{ "1", "BUN, REG/PREMIUM", ".50", "DOZEN", "176", "0", "0", "0", "0", "0", "0" },
	{ "2", "BUN, KAISER", ".70", "DOZEN", "176", "0", "0", "0", "0", "0", "0" }
};

void part_iter_cb(opencreport *o, ocrpt_part *p, void *data) {
	yields[3][0] = "MIKEROTH";
	yields[3][1] = "CALL";
	yields[3][2] = "BACK";
}

void precalc_done_cb(opencreport *o, void *data) {
	yields[3][0] = "Pasta";
	yields[3][1] = "1";
	yields[3][2] = "190";
}

int main(void) {
	opencreport *o = ocrpt_init();
	ocrpt_datasource *ds = ocrpt_datasource_add_array(o, "local_array");

	/* For m.sillypants in the report */
	setenv("sillypants", "5", 1);

	ocrpt_query_add_array(o, ds, "yields", (const char **)yields, 3, 3, NULL);
	ocrpt_query_add_array(o, ds, "coupons", (const char **)coupons, 4, 5, NULL);
	ocrpt_query_add_array(o, ds, "deposits", (const char **)deposits, 2, 4, NULL);
	ocrpt_query_add_array(o, ds, "petty_cash", (const char **)petty_cash, 2, 3, NULL);
	ocrpt_query_add_array(o, ds, "misc_income", (const char **)misc_income, 1, 3, NULL);
	ocrpt_query_add_array(o, ds, "inv_transfer", (const char **)inv_transfer, 2, 4, NULL);
	ocrpt_query_add_array(o, ds, "inventory", (const char **)inventory, 2, 11, NULL);

	if (!ocrpt_parse_xml(o, "layout_rlib_fixed_part_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	ocrpt_add_part_iteration_cb(o, part_iter_cb, NULL);
	ocrpt_add_precalculation_done_cb(o, precalc_done_cb, NULL);

	ocrpt_set_output_format(o, OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
