<?php
/*
 * OpenCReports test
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

$OCRPTENV = "This is a test string";

$r = OpenCReport::env_get("OCRPTENV");
echo "OCRPTENV is set. Value is: ";
$r->print();

unset($OCRPTENV);
$r = OpenCReport::env_get("OCRPTENV");
echo "OCRPTENV is unset. Value is: ";
$r->print();
