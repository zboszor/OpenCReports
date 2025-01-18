<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

if (!$o->parse_xml("pgsqlquery3.xml")) {
	echo "XML parse error" . PHP_EOL;
	exit(0);
}

$ds = $o->datasource_get("pgsql");

echo "Connecting to PostgreSQL database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;
