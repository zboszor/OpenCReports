/*
 * OpenCReports output driver common code
 * Copyright (C) 2019-2023 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <unistd.h>
#include <utf8proc.h>

#include "opencreport.h"
#include "ocrpt-private.h"
#include "common-output.h"

static void ocrpt_common_add_new_page(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, unsigned int rows, bool *newpage, double *page_indent, double *page_position, double *old_page_position) {
	common_private_data *priv = o->output_private;

	if (o->precalculate) {
		if (!priv->current_page) {
			if (!priv->pages)
				priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, NULL);
			priv->current_page = priv->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			priv->pages = ocrpt_list_end_append(priv->pages, &priv->last_page, NULL);
			priv->current_page = priv->last_page;
		}

		if (mpfr_cmp(o->totpages->number, o->pageno->number) < 0)
			mpfr_set(o->totpages->number, o->pageno->number, o->rndmode);
	} else {
		if (!priv->current_page) {
			priv->current_page = priv->pages;
		} else {
			mpfr_add_ui(o->pageno->number, o->pageno->number, 1, o->rndmode);
			priv->current_page = priv->current_page->next;
		}
	}
}

static void *ocrpt_common_get_current_page(opencreport *o) {
	common_private_data *priv = o->output_private;

	return priv->current_page;
}

static void ocrpt_common_set_current_page(opencreport *o, void *page) {
	common_private_data *priv = o->output_private;

	priv->current_page = page;
}

static bool ocrpt_common_is_current_page_first(opencreport *o) {
	common_private_data *priv = o->output_private;

	return priv->current_page == priv->pages;
}

void ocrpt_common_set_font_sizes(opencreport *o, const char *font, double wanted_font_size, bool bold, bool italic, double *result_font_size, double *result_font_width) {
	common_private_data *priv = o->output_private;

	if (priv->prepare_set_font_sizes)
		priv->prepare_set_font_sizes(o);

	PangoLayout *layout;
	PangoFontDescription *font_description;

	font_description = pango_font_description_new();

	pango_font_description_set_family(font_description, font);
	pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_font_description_set_style(font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_absolute_size(font_description, wanted_font_size * PANGO_SCALE);

	layout = pango_cairo_create_layout(priv->cr);
	pango_layout_set_font_description(layout, font_description);

	PangoContext *context = pango_layout_get_context(layout);
	PangoLanguage *language = pango_context_get_language(context);
	PangoFontMetrics *metrics = pango_context_get_metrics(context, font_description, language);

	const char *family = pango_font_description_get_family(font_description);
	PangoFontFamily **families;
	int n_families, i;
	bool monospace = true;

	pango_context_list_families(context, &families, &n_families);

	for (i = 0; i < n_families; i++) {
		if (strcmp(family, pango_font_family_get_name(families[i])) == 0) {
			monospace = pango_font_family_is_monospace(families[i]);
			break;
		}
	}

	/* This pointer MUST be freed with g_free() */
	g_free(families);

	int char_width = pango_font_metrics_get_approximate_char_width(metrics);

	if (result_font_size)
		*result_font_size = wanted_font_size;
	if (result_font_width)
		*result_font_width = (monospace ? ((double)char_width / PANGO_SCALE) : wanted_font_size);

	pango_font_metrics_unref(metrics);
	g_object_unref(layout);
	pango_font_description_free(font_description);
}

void ocrpt_common_init(opencreport *o, size_t privsz, size_t datasz, size_t outbufsz) {
	memset(&o->output_functions, 0, sizeof(ocrpt_output_functions));
	o->output_functions.add_new_page = ocrpt_common_add_new_page;
	o->output_functions.get_current_page = ocrpt_common_get_current_page;
	o->output_functions.set_current_page = ocrpt_common_set_current_page;
	o->output_functions.is_current_page_first = ocrpt_common_is_current_page_first;

	o->output_private = ocrpt_mem_malloc(privsz);
	memset(o->output_private, 0, privsz);

	common_private_data *priv = o->output_private;
	priv->data = ocrpt_mem_string_new_with_len(NULL, datasz);

	o->output_buffer = ocrpt_mem_string_new_with_len(NULL, outbufsz);

	cairo_rectangle_t page = { .x = o->paper->width, .y = o->paper->height };
	priv->nullpage_cs = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &page);
	priv->cr = cairo_create(priv->nullpage_cs);
}

void ocrpt_common_finalize(opencreport *o) {
	common_private_data *priv = o->output_private;

	ocrpt_list_free(priv->pages);
	ocrpt_mem_string_free(priv->data, true);

	if (priv->cr)
		cairo_destroy(priv->cr);
	if (priv->nullpage_cs)
		cairo_surface_destroy(priv->nullpage_cs);

	ocrpt_mem_free(priv);

	o->output_private = NULL;
}
