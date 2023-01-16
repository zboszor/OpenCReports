<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$srcdir = getenv("abs_srcdir");
if (is_bool($srcdir))
	$srcdir = getcwd();
$srcdir = OpenCReport::canonicalize_path($srcdir);

$o->add_search_path($srcdir);

$files = [
	"images/images/matplotlib.svg",
	"images2/images/matplotlib.svg",
	"images2/images2/matplotlib.svg",
	"images/images/doesnotexist.png",
];

foreach ($files as &$f) {
	$file = $o->find_file($f);
	echo "file '" . $f . "' found canonically '" . (!is_null($file) ? substr($file, strlen($srcdir) + 1) : "") . "'" . PHP_EOL;
}
