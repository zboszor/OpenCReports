/*
 * OpenCReports main module
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "opencreport.h"

static ocrpt_color compat_color_names[] = {
	{ "Aqua",		"#00ffff" },
	{ "Black",		"#000000" },
	{ "Blue",		"#0000ff" },
	{ "BobKratz",	"#ffc59f" },
	{ "Everton",	"#d3d3d3" },
	{ "Fuchsia",	"#ff00ff" },
	{ "Gray",		"#808080" },
	{ "Green",		"#008000" },
	{ "Lime",		"#00ff00" },
	{ "Maroon",		"#800000" },
	{ "Navy",		"#000080" },
	{ "Olive",		"#808000" },
	{ "Purple",		"#800080" },
	{ "Red",		"#ff0000" },
	{ "Silver",		"#c0c0c0" },
	{ "Teal",		"#008080" },
	{ "White",		"#ffffff" },
	{ "Yellow",		"#ffff00" },
};

static int32_t compat_color_names_n = sizeof(compat_color_names) / sizeof(ocrpt_color);

static int colorsortcmp(const void *a, const void *b) {
	return strcasecmp(((ocrpt_color *)a)->name, ((ocrpt_color *)b)->name);
}

static int colorfindcmp(const void *key, const void *a) {
	return strcasecmp(key, ((ocrpt_color *)a)->name);
}

/* We assume cname is 6 digits long. Callers should make sure. */
static void ocrpt_parse_html_color(const char *cname, ocrpt_color *color) {
	char *endptr = NULL;
	long html = strtol(cname, &endptr, 16);

	if (*endptr == 0) {
		color->r = (double)((html >> 16) & 0xff) / 255.0;
		color->g = (double)((html >>  8) & 0xff) / 255.0;
		color->b = (double)((html      ) & 0xff) / 255.0;
	} else {
		color->r = 0.0;
		color->g = 0.0;
		color->b = 0.0;
	}
}

void ocrpt_init_color(void) {
	qsort(&compat_color_names, compat_color_names_n, sizeof(ocrpt_color), colorsortcmp);

	for (int32_t i = 0; i < compat_color_names_n; i++)
		ocrpt_parse_html_color(compat_color_names[i].html + 1, &compat_color_names[i]);
}

DLL_EXPORT_SYM void ocrpt_get_color(opencreport *o, const char *cname, ocrpt_color *color, bool bgcolor) {
	if (!cname || *cname == 0)
		cname = bgcolor ? "White" : "Black";

	if (*cname == '#')
		ocrpt_parse_html_color(cname + 1, color);
	else if (strncasecmp(cname, "0x", 2) == 0)
		ocrpt_parse_html_color(cname + 2, color);
	else {
		ocrpt_color *c;

		while (!(c = bsearch(cname, &compat_color_names, compat_color_names_n, sizeof(ocrpt_color), colorfindcmp)))
			cname = bgcolor ? "White" : "Black";

		*color = *c;
	}
}
