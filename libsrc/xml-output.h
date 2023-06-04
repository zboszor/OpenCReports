/*
 * OpenCReports XML output driver
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_XML_H_
#define _OCRPT_XML_H_

#include <libxml/tree.h>
#include "opencreport.h"
#include "common-output.h"

struct xml_private_data {
	common_private_data base;
	xmlDocPtr doc;
	xmlNodePtr toplevel;
	xmlNodePtr part;
	xmlNodePtr parttbl;
	xmlNodePtr ph, pho;
	xmlNodePtr pf, pfo;
	xmlNodePtr tr;
	xmlNodePtr td;
	xmlNodePtr r;
	xmlNodePtr rh, rho;
	xmlNodePtr rf, rfo;
	xmlNodePtr fh, fho;
	xmlNodePtr fd, fdo;
	xmlNodePtr nd, ndo;
	xmlNodePtr bh, bho;
	xmlNodePtr bf, bfo;
	ocrpt_list *rl;
	ocrpt_list *rl_last;
	xmlNodePtr line;
};
typedef struct xml_private_data xml_private_data;

void ocrpt_xml_init(opencreport *o);

#endif
