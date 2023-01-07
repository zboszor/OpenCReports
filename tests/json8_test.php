<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$JSONFILE = "jsonquery8.json";

$o = new OpenCReport();
$ds = $o->datasource_add_json("json");
$q = $ds->query_add("a", $JSONFILE);
if (is_null($q)) {
	echo $JSONFILE . " parsing failed (good, it's intentional)" . PHP_EOL;
	exit(0);
}

echo $JSONFILE . " parsing succeded (bad, there's errors in it)" . PHP_EOL;
