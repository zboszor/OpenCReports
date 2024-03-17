<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2024 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$o = new OpenCReport();

$OCRPTENV = "This is a test string";

$r = $o->env_get("OCRPTENV");
echo "OCRPTENV is set. Value is: ";
$r->print();

unset($OCRPTENV);
$r = $o->env_get("OCRPTENV");
echo "OCRPTENV is unset. Value is: ";
$r->print();
