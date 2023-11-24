<?php
$o = new OpenCReport();
$ds = $o->datasource_add_postgresql("pgsql", NULL, NULL, "ocrpttest", "ocrpt", NULL);
$q1 = $ds->query_add("q1", "select * from flintstones4;");
$q2 = $ds->query_add("q2", "select * from flintstones5;");

$q1->add_follower($q2);

if (!$o->parse_xml("example5.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

$o->execute();
$o->spool();
