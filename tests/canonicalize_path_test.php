<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$srcdir = getenv("abs_srcdir");
if (is_bool($srcdir))
	$srcdir = getcwd();
$csrcdir = OpenCReport::canonicalize_path($srcdir);
$slen = strlen($csrcdir);

$builddir = getenv("abs_srcdir");
if (is_bool($builddir))
	$builddir = getcwd();
$cbuilddir = OpenCReport::canonicalize_path($builddir);
$blen = strlen($cbuilddir);

/* Assume that the srcdir doesn't contain a symlink along its path */
if (strcmp($srcdir, $csrcdir) == 0)
	echo "srcdir equals canonical srcdir" . PHP_EOL;

$cstr_a = explode("/", $csrcdir);

$slashes = 2;
$s1 = "";
foreach ($cstr_a as &$s) {
	if ($s == "")
		continue;

	$s1 .= str_repeat("/", $slashes) . $s;
	$slashes++;
}

$s2 = $s1;

$s1 .= "/../tests/images/images";
$s2 .= "/../tests/images2/images2";

$s1c = OpenCReport::canonicalize_path($s1);
$s2c = OpenCReport::canonicalize_path($s2);

if (strcmp($s1c, $s2c) == 0) {
	echo "Two subdirs relative to abs_srcdir are equal after canonicalization:" . PHP_EOL;
	echo "\t" . substr($s1c, $slen + 1) . PHP_EOL;
	echo "\t" . substr($s2c, $slen + 1) . PHP_EOL;
}


/* "images/images5" is a bad recursive symlink, canonicalization returns it as is */
$s1 = $csrcdir . "/images/images5";
$s1c = OpenCReport::canonicalize_path($s1);
echo "Bad recursive symlink, returned as is: ", substr($s1c, $slen + 1) . PHP_EOL;

$s1c = OpenCReport::canonicalize_path("images2/images2");
echo "Canonicalized path of 'images2/images2' relative to abs_builddir: '" . substr($s1c, $slen + 1) . "'" . PHP_EOL;
