<?php
$o = new OpenCReport();

if (!$o->parse_xml("example1.xml")) {
    echo "XML parse error" . PHP_EOL;
    exit(0);
}

$o->execute();
$o->spool();
