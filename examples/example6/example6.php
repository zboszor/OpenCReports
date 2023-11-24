<?php
$o = new OpenCReport();
$ds = $o->datasource_add_postgresql("pgsql", NULL, NULL, "ocrpttest", "ocrpt", NULL);
$q1 = $ds->query_add("q1", "select * from data;");
$q2 = $ds->query_add("q2", "select * from more_data;");
$q3 = $ds->query_add("q3", "select * from moar_data;");

$match1 = $o->expr_parse("q1.id = q2.boss_id");
$q1->add_follower_n_to_1($q2, $match1);

$match2 = $o->expr_parse("q2.id = q3.sk_id");
$q2->add_follower_n_to_1($q3, $match2);

if (!$o->parse_xml("example6.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

$o->execute();
$o->spool();
