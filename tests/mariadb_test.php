<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$ds = $o->datasource_add_mariadb("mariadb", NULL, NULL, "ocrpttest", "ocrpt", NULL, NULL);

echo "Connecting to MariaDB database was " . ($ds instanceof OpenCReport\Datasource ? "" : "NOT ") . "successful" . PHP_EOL;
