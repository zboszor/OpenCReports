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
#include <stdio.h>
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
	PHP_OPENCREPORT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

ZEND_GET_MODULE(opencreport)

/* Handlers */
static zend_object_handlers opencreport_object_handlers;

/* Class entries */
zend_class_entry *opencreport_sc_entry;

static zend_object *opencreport_object_new(zend_class_entry *class_type) /* {{{ */
{
	php_opencreport_object *intern;

	/* Allocate memory for it */
	intern = zend_object_alloc(sizeof(php_opencreport_object), class_type);

	zend_object_std_init(&intern->zo, class_type);
	object_properties_init(&intern->zo, class_type);

	intern->zo.handlers = &opencreport_object_handlers;

	return &intern->zo;
}
/* }}} */

static void opencreport_object_free(zend_object *object) /* {{{ */
{
	php_opencreport_object *oo = php_opencreport_from_obj(object);

	if (!oo)
		return;

	if (oo->o) {
		ocrpt_free(oo->o);
		oo->o = NULL;
	}

	zend_object_std_dtor(&oo->zo);
}
/* }}} */

PHP_METHOD(opencreport, __construct) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);

	fprintf(stderr, "opencreport __construct called\n");
	oo->o = ocrpt_init();
	fprintf(stderr, "opencreport __construct initialized opencreport\n");
}

PHP_METHOD(opencreport, __destruct) {
	zval *object = ZEND_THIS;
	php_opencreport_object *oo = Z_OPENCREPORT_P(object);
	fprintf(stderr, "opencreport __destruct called\n");
	ocrpt_free(oo->o);
	oo->o = NULL;
	fprintf(stderr, "opencreport __destruct deinitialized opencreport\n");
}

ZEND_BEGIN_ARG_INFO(arginfo_opencreport_void, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry opencreport_class_methods[] = {
	PHP_ME(opencreport, __construct, arginfo_opencreport_void, ZEND_ACC_PUBLIC)
	PHP_ME(opencreport, __destruct, arginfo_opencreport_void, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/* {{{ PHP_MINIT_FUNCTION */
static PHP_MINIT_FUNCTION(opencreport)
{
	zend_class_entry ce;

	memcpy(&opencreport_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));

	INIT_CLASS_ENTRY(ce, "OpenCReport", opencreport_class_methods);
	ce.create_object = opencreport_object_new;
	opencreport_object_handlers.offset = XtOffsetOf(php_opencreport_object, zo);
	opencreport_object_handlers.clone_obj = NULL;
	opencreport_object_handlers.free_obj = opencreport_object_free;
	opencreport_sc_entry = zend_register_internal_class(&ce);
#if PHP_VERSION_ID >= 80100
	opencreport_sc_entry->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
#else
	opencreport_sc_entry->serialize = zend_class_serialize_deny;
	opencreport_sc_entry->unserialize = zend_class_unserialize_deny;
#endif

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
	php_info_print_table_row(2, "OpenCReport module version", PHP_OPENCREPORT_VERSION);
	php_info_print_table_row(2, "OpenCReport library version", ocrpt_version());

	php_info_print_table_end();
}
/* }}} */
