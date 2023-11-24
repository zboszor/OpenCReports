<?php
$r = rlib_init();

rlib_add_datasource_postgres($r, "pgsql", "dbname=ocrpttest user=ocrpt");
rlib_add_query_as($r, "pgsql", "select * from data order by id;", "q1");
rlib_add_query_as($r, "pgsql", "select * from more_data order by id;", "q2");
rlib_add_query_as($r, "pgsql", "select * from moar_data order by sk_id;", "q3");

rlib_add_resultset_follower_n_to_1($r, "q1", "id", "q2", "boss_id");
rlib_add_resultset_follower_n_to_1($r, "q2", "id", "q3", "sk_id");

if (!rlib_add_report($r, "example6.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

rlib_execute($r);
rlib_spool($r);
