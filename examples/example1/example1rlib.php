<?php
$r = rlib_init();

if (!rlib_add_report($r, "example1.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

rlib_execute($r);
rlib_spool($r);
