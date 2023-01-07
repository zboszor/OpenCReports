<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$ds = $o->datasource_add_postgresql2("pgsql", "dbname=ocrpttest user=ocrpt");

echo "Connecting to PostgreSQL database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;
