<?php

$o = new OpenCReport();
$o->add_search_path(getcwd());

if (!$o->parse_xml("layout_xlsx_test.xml")) {
	echo "XML parse error" . PHP_EOL;
	return 0;
}

$o->set_output_format(OpenCReport::OUTPUT_PDF);
$o->execute();
$o->spool();
