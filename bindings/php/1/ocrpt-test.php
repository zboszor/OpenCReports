<?php

$o = new OpenCReport();

if (!$o->parse_xml("report.xml")) {
    printf("XML parse error\n");
    exit(1);
}

$o->set_output_format(OCRPT_OUTPUT_PDF);
$o->execute();
$o->spool();
