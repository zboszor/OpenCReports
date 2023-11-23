<?php
$r = rlib_init();

rlib_add_datasource_postgres($r, "pgsql", "dbname=ocrpttest user=ocrpt");
rlib_add_query_as($r, "pgsql", "select * from flintstones;", "q");

if (!rlib_add_report($r, "example2.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

rlib_execute($r);
rlib_spool($r);
