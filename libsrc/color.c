/*
 * OpenCReports color handling
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "opencreport.h"
#include "color.h"

static ocrpt_named_color compat_color_names[] = {
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

static int32_t compat_color_names_n = sizeof(compat_color_names) / sizeof(ocrpt_named_color);

static int colorsortcmp(const void *a, const void *b) {
	return strcasecmp(((ocrpt_named_color *)a)->name, ((ocrpt_named_color *)b)->name);
}

static int colorfindcmp(const void *key, const void *a) {
	return strcasecmp(key, ((ocrpt_named_color *)a)->name);
}

/*
 * We assume cname is 6 or 8 digits long.
 * For colors with alpha, ARGB notation is used.
 * Callers should make sure.
 */
static void ocrpt_parse_html_color(const char *cname, ocrpt_named_color *color) {
	char *endptr = NULL;
	long html = strtol(cname, &endptr, 16);
	size_t len = endptr - cname;

	if (*endptr == 0) {
		color->c.r = (double)((html >> 16) & 0xff) / 255.0;
		color->c.g = (double)((html >>  8) & 0xff) / 255.0;
		color->c.b = (double)((html      ) & 0xff) / 255.0;
		/* For RGB patterns, make sure the color is opaque. */
		if (len <= 6)
			html |= 0xff000000;
		color->c.alpha = (double)((html >> 24) & 0xff) / 255.0;
	} else {
		color->c.r = 0.0;
		color->c.g = 0.0;
		color->c.b = 0.0;
		color->c.alpha = 1.0;
	}
}

void ocrpt_init_color(void) {
	qsort(&compat_color_names, compat_color_names_n, sizeof(ocrpt_named_color), colorsortcmp);

	for (int32_t i = 0; i < compat_color_names_n; i++)
		ocrpt_parse_html_color(compat_color_names[i].html + 1, &compat_color_names[i]);
}

DLL_EXPORT_SYM void ocrpt_get_color(const char *cname, ocrpt_color *color, bool bgcolor) {
	if (!color)
		return;

	ocrpt_named_color nc;

	if (!cname || *cname == 0)
		cname = bgcolor ? "White" : "Black";

	if (*cname == '#') {
		ocrpt_parse_html_color(cname + 1, &nc);
		*color = nc.c;
	} else if (strncasecmp(cname, "0x", 2) == 0) {
		ocrpt_parse_html_color(cname + 2, &nc);
		*color = nc.c;
	} else if (strcasecmp(cname, "transparent") == 0) {
		/* Transparent white */
		color->r = color->g = color->b = 1.0;
		color->alpha = 0.0;
	}  else {
		ocrpt_named_color *c = bsearch(cname, &compat_color_names, compat_color_names_n, sizeof(ocrpt_named_color), colorfindcmp);

		if (c)
			*color = (*c).c;
		else {
			/* Fall back to opaque black or white */
			color->r = color->g = color->b = (bgcolor ? 1.0 : 0.0);
			color->alpha = 1.0;
		}
	}
}
