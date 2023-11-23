<?php
$o = new OpenCReport();
$ds = $o->datasource_add_postgresql("pgsql", NULL, NULL, "ocrpttest", "ocrpt", NULL);

$ds->query_add("q", "select * from flintstones2;");

if (!$o->parse_xml("example3.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

$o->execute();
$o->spool();
