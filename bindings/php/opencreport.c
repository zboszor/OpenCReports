/*
 * OpenCReports PHP module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.PHP in the toplevel directory.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "zend_interfaces.h"
#include "php_opencreport.h"
#include <opencreport.h>

/* {{{ zend_function_entry */
static const zend_function_entry opencreport_functions[] = {
#ifdef  PHP_FE_END
	PHP_FE_END
#else
	{NULL,NULL,NULL}
#endif
};
/* }}} */

/* {{{ function prototypes */
static PHP_MINIT_FUNCTION(opencreport);
static PHP_MSHUTDOWN_FUNCTION(opencreport);
static PHP_MINFO_FUNCTION(opencreport);
/* }}} */

/* {{{ opencreport_module_entry
 */
zend_module_entry opencreport_module_entry = {
	STANDARD_MODULE_HEADER,
	"opencreport",
	opencreport_functions,
	PHP_MINIT(opencreport),
	PHP_MSHUTDOWN(opencreport),
	NULL,
	NULL,
	PHP_MINFO(opencreport),
	PHP_OCRPT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

ZEND_GET_MODULE(opencreport)

/* {{{ PHP_MINIT_FUNCTION */
static PHP_MINIT_FUNCTION(opencreport)
{
	return SUCCESS;
}

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
static PHP_MSHUTDOWN_FUNCTION(opencreport)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(opencreport)
{
	php_info_print_table_start();

	php_info_print_table_row(2, "OpenCReport", "enabled");
	php_info_print_table_row(2, "OpenCReport module version", PHP_OCRPT_VERSION);
	php_info_print_table_row(2, "OpenCReport library version", ocrpt_version());

	php_info_print_table_end();
}
/* }}} */
