<?php
$r = rlib_init();

rlib_add_datasource_postgres($r, "pgsql", "dbname=ocrpttest user=ocrpt");
rlib_add_query_as($r, "pgsql", "select * from flintstones4;", "q1");
rlib_add_query_as($r, "pgsql", "select * from flintstones5;", "q2");

rlib_add_resultset_follower($r, "q1", "q2");

if (!rlib_add_report($r, "example5.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

rlib_execute($r);
rlib_spool($r);
