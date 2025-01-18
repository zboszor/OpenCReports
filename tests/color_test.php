<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$colornames = [ "Black", "Red", "bobkratz" ];

foreach ($colornames as &$color) {
	$c = $o->get_color($color);

	printf("%s: %.4lf %.4lf %.4lf\n", $color, $c["r"], $c["g"], $c["b"]);
}
