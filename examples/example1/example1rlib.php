<?php
$r = rlib_init();

rlib_add_report($r, "layout_ocrpt_fixed_part_test.xml");

if (!rlib_add_report($r, "example1.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

rlib_execute($r);
rlib_spool($r);
