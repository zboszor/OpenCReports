/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#include "php_opencreport.h"

/* {{{ PHP_MINIT_FUNCTION */
static PHP_MINIT_FUNCTION(opencreport)
{
	if (!opencreport_init())
		return FAILURE;

	register_opencreport_ce();
	register_opencreport_ds_ce();
	register_opencreport_query_ce();
	register_opencreport_query_result_ce();
	register_opencreport_expr_ce();
	register_opencreport_result_ce();
	register_opencreport_part_ce();
	register_opencreport_row_ce();
	register_opencreport_col_ce();
	register_opencreport_report_ce();
	register_opencreport_variable_ce();
	register_opencreport_break_ce();
	register_opencreport_output_ce();
	register_opencreport_line_ce();
	register_opencreport_hline_ce();
	register_opencreport_image_ce();
	register_opencreport_text_ce();
	register_opencreport_barcode_ce();
	register_opencreport_output_element_ce();
	register_opencreport_line_element_ce();

	return SUCCESS;
}

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(opencreport) {
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION
 */
static PHP_RINIT_FUNCTION(opencreport) {
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(opencreport) {
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(opencreport)
{
	php_info_print_table_start();

	php_info_print_table_row(2, "OpenCReport", "enabled");
	php_info_print_table_row(2, "OpenCReport module version", PHP_OPENCREPORT_VERSION);
	php_info_print_table_row(2, "OpenCReport library version", ocrpt_version());

	php_info_print_table_end();
}
/* }}} */

/* {{{ opencreport_module_entry
 */
zend_module_entry opencreport_module_entry = {
	STANDARD_MODULE_HEADER,
	"opencreport",
	opencreport_functions,
	PHP_MINIT(opencreport),
	PHP_MSHUTDOWN(opencreport),
	PHP_RINIT(opencreport),
	PHP_RSHUTDOWN(opencreport),
	PHP_MINFO(opencreport),
	PHP_OPENCREPORT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

ZEND_GET_MODULE(opencreport)
