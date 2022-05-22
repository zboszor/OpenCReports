/*
 * Build with:
 * gcc -Wall -Werror -o cairotest cairotest.c $(pkg-config --cflags --libs cairo cairo-pdf pangocairo librsvg-2.0 gdk-pixbuf-2.0 gtk4)
 */

#include <config.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>

const char *loremipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

cairo_status_t ocrpt_write_pdf(void *closure, const unsigned char *data, unsigned int length) {
	/* "closure" is the same as "user_data" for GLIB/GTK */
	size_t len = write(fileno(stdout), data, length);
	assert(len == length);
	return CAIRO_STATUS_SUCCESS; /* or CAIRO_STATUS_WRITE_ERROR */
}

void put_svg_on_page(cairo_t *cr, const char *filename, double x, double y, double w, double h) {
	struct {
		double width, height;
	} dimensions;

	RsvgHandle *rsvg = rsvg_handle_new_from_file(filename, NULL);
	if (!rsvg)
		return;

	cairo_save(cr);

	rsvg_handle_get_intrinsic_size_in_pixels(rsvg, &dimensions.width, &dimensions.height);

	cairo_surface_t *svg = cairo_svg_surface_create(NULL, dimensions.width, dimensions.height);
	cairo_set_source_surface(cr, svg, 0.0, 0.0);

	cairo_translate(cr, x, y);
	cairo_scale(cr, w / dimensions.width, h / dimensions.height);

	RsvgRectangle rect = { .x = 0.0, .y = 0.0, .width = dimensions.width, .height = dimensions.height };
	rsvg_handle_render_document(rsvg, cr, &rect, NULL);

	cairo_paint(cr);

	cairo_surface_destroy(svg);
	g_object_unref(rsvg);

	cairo_restore(cr);
}

void put_png_on_page(cairo_t *cr, const char *filename, double x, double y, double w, double h) {
	struct {
		double width, height;
	} dimensions;
	double scale_x, scale_y;

	cairo_save(cr);

	cairo_surface_t *png = cairo_image_surface_create_from_png(filename);

	dimensions.width = cairo_image_surface_get_width(png);
	dimensions.height = cairo_image_surface_get_height(png);
	scale_x = w / dimensions.width;
	scale_y = h / dimensions.height;

	cairo_translate(cr, x, y);
	cairo_scale(cr, scale_x, scale_y);

	cairo_set_source_surface(cr, png, 0.0, 0.0);
	cairo_paint(cr);

	cairo_surface_destroy(png);

	cairo_restore(cr);
}

static cairo_surface_t *_cairo_new_surface_from_pixbuf(const GdkPixbuf *pixbuf) {
	int width = gdk_pixbuf_get_width(pixbuf);
	int height = gdk_pixbuf_get_height(pixbuf);
	guchar *gdk_pixels = gdk_pixbuf_get_pixels(pixbuf);
	int gdk_rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
	int cairo_stride;
	guchar *cairo_pixels;

	cairo_format_t format;
	cairo_surface_t *surface;
	static const cairo_user_data_key_t key;
	int j;

	format = (n_channels == 3 ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_ARGB32);

	cairo_stride = cairo_format_stride_for_width(format, width);
	cairo_pixels = g_malloc(height * cairo_stride);
	surface = cairo_image_surface_create_for_data((unsigned char *)cairo_pixels, format, width, height, cairo_stride);

	cairo_surface_set_user_data(surface, &key, cairo_pixels, (cairo_destroy_func_t)g_free);

	for (j = height; j; j--) {
		guchar *p = gdk_pixels;
		guchar *q = cairo_pixels;

		if (n_channels == 3) {
			guchar *end = p + 3 * width;

			while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				q[0] = p[2]; q[1] = p[1]; q[2] = p[0];
#else
				q[1] = p[0]; q[2] = p[1]; q[3] = p[2];
#endif
				p += 3; q += 4;
			}
		} else {
			guchar *end = p + 4 * width;
			guint t1,t2,t3;

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

			while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				MULT(q[0], p[2], p[3], t1);
				MULT(q[1], p[1], p[3], t2);
				MULT(q[2], p[0], p[3], t3);
				q[3] = p[3];
#else
				q[0] = p[3];
				MULT(q[1], p[0], p[3], t1);
				MULT(q[2], p[1], p[3], t2);
				MULT(q[3], p[2], p[3], t3);
#endif
				p += 4; q += 4;
			}
#undef MULT
		}

		gdk_pixels += gdk_rowstride;
		cairo_pixels += cairo_stride;
	}

	return surface;
}

void put_pixbuf_on_page(cairo_t *cr, const char *filename, double x, double y, double w, double h) {
	struct {
		double width, height;
	} dimensions;
	double scale_x, scale_y;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

	if (!pixbuf)
		return;

	cairo_save(cr);

	dimensions.width = gdk_pixbuf_get_width(pixbuf);
	dimensions.height = gdk_pixbuf_get_height(pixbuf);

	cairo_surface_t *surface = _cairo_new_surface_from_pixbuf(pixbuf);

	scale_x = w / dimensions.width;
	scale_y = h / dimensions.height;

	cairo_translate(cr, x, y);
	cairo_scale(cr, scale_x, scale_y);

	cairo_set_source_surface(cr, surface, 0.0, 0.0);
	cairo_paint(cr);

	cairo_surface_destroy(surface);

	cairo_restore(cr);

	g_object_unref(pixbuf);
}

void draw_rect(cairo_t *cr, double lw, double x, double y, double w, double h) {
	cairo_set_line_width(cr, lw);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_rectangle(cr, x, y, w, h);
	cairo_stroke(cr);
}

void draw_text_with_bounding_box(cairo_t *cr, const char *font, const char *text, bool wrap, PangoAlignment align, double x, double y, double width, double *height) {
	PangoLayout *layout;
	PangoFontDescription *font_description;

	font_description = pango_font_description_new();

	pango_font_description_set_family(font_description, font);
	pango_font_description_set_weight(font_description, PANGO_WEIGHT_NORMAL);
	pango_font_description_set_absolute_size(font_description, 12.0 * PANGO_SCALE);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, font_description);

	pango_layout_set_alignment(layout, align);

	if (wrap) {
		pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
	} else {
		PangoAttrList *alist = pango_layout_get_attributes(layout);
		if (!alist)
			alist = pango_attr_list_new();
		PangoAttribute *nohyph = pango_attr_insert_hyphens_new(FALSE);
		pango_attr_list_insert(alist, nohyph);
		pango_layout_set_attributes(layout, alist);
		pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
	}

	pango_layout_set_width(layout, width * PANGO_SCALE);

	pango_layout_set_text(layout, text, -1);

	PangoRectangle ink_rect, logical_rect;

	if (wrap) {
		pango_layout_get_extents(layout, &ink_rect, &logical_rect);
	} else {
		PangoLayoutLine *pline = pango_layout_get_line(layout, 0);

		pango_layout_line_get_extents(pline, &ink_rect, &logical_rect);

		char *line = malloc(pline->length + 1);
		memcpy(line, text, pline->length);
		line[pline->length] = 0;
		pango_layout_set_text(layout, line, -1);
		free(line);
	}

	fprintf(stderr, "ink_rect x %d w %d logical_rect x %d y %d width %d height %d\n", ink_rect.x, ink_rect.width, logical_rect.x, logical_rect.y, logical_rect.width, logical_rect.height);

	double h = (double)logical_rect.height / PANGO_SCALE;

	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	cairo_rectangle(cr, x, y, width, h);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	if (align != PANGO_ALIGN_LEFT) {
		double xdiff = (double)(ink_rect.width - logical_rect.width) / PANGO_SCALE;
		cairo_move_to(cr, x - xdiff, y);
#if 0
	} else if (align == PANGO_ALIGN_CENTER) {
		double xdiff = (double)(ink_rect.width - logical_rect.width) / PANGO_SCALE / 2;
		cairo_move_to(cr, x - xdiff, y);
#endif
	} else
		cairo_move_to(cr, x, y);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
	pango_font_description_free(font_description);

	*height = h;
}

int main(void) {
	cairo_rectangle_t page = { .x = 0.0, .y = 0.0, .width = 595.276, .height = 841.89 };
	cairo_surface_t *rec = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &page);
	cairo_t *cr = cairo_create(rec);

	double x, y;
	double width = 150.0;
	double height;

	x = 200.0;

	y = 50.0;
	draw_text_with_bounding_box(cr, "Z003", loremipsum, false, PANGO_ALIGN_LEFT, x, y, width, &height);
	fprintf(stderr, "after draw_text_with_bounding_box w %lf h %lf\n", width, height);
	//draw_rect(cr, 0.1, x, y, width, height);

	y += height + 10.0;
	draw_text_with_bounding_box(cr, "Z003", loremipsum, true, PANGO_ALIGN_LEFT, x, y, width, &height);
	fprintf(stderr, "after draw_text_with_bounding_box w %lf h %lf\n", width, height);
	//draw_rect(cr, 0.1, x, y, width, height);

	y += height + 10.0;
	draw_text_with_bounding_box(cr, "Z003", loremipsum, false, PANGO_ALIGN_RIGHT, x, y, width, &height);
	fprintf(stderr, "after draw_text_with_bounding_box w %lf h %lf\n", width, height);
	//draw_rect(cr, 0.1, x, y, width, height);

	y += height + 10.0;
	draw_text_with_bounding_box(cr, "Z003", loremipsum, true, PANGO_ALIGN_RIGHT, x, y, width, &height);
	fprintf(stderr, "after draw_text_with_bounding_box w %lf h %lf\n", width, height);
	//draw_rect(cr, 0.1, x, y, width, height);

	y += height + 10.0;
	draw_text_with_bounding_box(cr, "Z003", loremipsum, false, PANGO_ALIGN_CENTER, x, y, width, &height);
	fprintf(stderr, "after draw_text_with_bounding_box w %lf h %lf\n", width, height);
	//draw_rect(cr, 0.1, x, y, width, height);

	y += height + 10.0;
	draw_text_with_bounding_box(cr, "Z003", loremipsum, true, PANGO_ALIGN_CENTER, x, y, width, &height);
	fprintf(stderr, "after draw_text_with_bounding_box w %lf h %lf\n", width, height);
	//draw_rect(cr, 0.1, x, y, width, height);

	put_svg_on_page(cr, "img/matplotlib.svg", 100.0, 100.0, 40.0, 40.0);
	//put_svg_on_page(cr, "math-formula.svg", 100.0, 100.0, 40.0, 40.0);
	draw_rect(cr, 1.0, 100.0, 100.0, 40.0, 40.0);
	put_svg_on_page(cr, "img/matplotlib.svg", 100.0, 140.0, 60.0, 40.0);
	//put_svg_on_page(cr, "math-formula.svg", 100.0, 140.0, 60.0, 40.0);
	draw_rect(cr, 1.0, 100.0, 140.0, 60.0, 40.0);

	put_png_on_page(cr, "img/math-formula.png", 100.0, 200.0, 40.0, 40.0);
	draw_rect(cr, 1.0, 100.0, 200.0, 40.0, 40.0);
	put_png_on_page(cr, "img/math-formula.png", 100.0, 240.0, 60.0, 40.0);
	draw_rect(cr, 1.0, 100.0, 240.0, 60.0, 40.0);

	put_pixbuf_on_page(cr, "img/math-formula.jpg", 100.0, 300.0, 40.0, 40.0);
	draw_rect(cr, 1.0, 100.0, 300.0, 40.0, 40.0);
	put_pixbuf_on_page(cr, "img/math-formula.jpg", 100.0, 340.0, 60.0, 40.0);
	draw_rect(cr, 1.0, 100.0, 340.0, 60.0, 40.0);

	put_pixbuf_on_page(cr, "img/math-formula.gif", 100.0, 400.0, 40.0, 40.0);
	draw_rect(cr, 1.0, 100.0, 400.0, 40.0, 40.0);
	put_pixbuf_on_page(cr, "img/math-formula.gif", 100.0, 440.0, 60.0, 40.0);
	draw_rect(cr, 1.0, 100.0, 440.0, 60.0, 40.0);

	cairo_destroy(cr);

	cairo_surface_t *pdf = cairo_pdf_surface_create_for_stream(ocrpt_write_pdf, NULL, 595.276, 841.89);
	cairo_pdf_surface_restrict_to_version(pdf, CAIRO_PDF_VERSION_1_5);
	cairo_pdf_surface_set_metadata(pdf, CAIRO_PDF_METADATA_TITLE, "OpenCReports report");
	cairo_pdf_surface_set_metadata(pdf, CAIRO_PDF_METADATA_AUTHOR, "OpenCReports");
	cairo_pdf_surface_set_metadata(pdf, CAIRO_PDF_METADATA_CREATOR, "OpenCReports");
	cairo_pdf_surface_set_metadata(pdf, CAIRO_PDF_METADATA_CREATE_DATE, "2020-01-01T00:00");

	for (int i = 0; i < 3; i++) {
		cr = cairo_create(pdf);
		cairo_set_source_surface(cr, rec, 0.0, 0.0);
		cairo_paint(cr);
		cairo_show_page(cr);
		cairo_destroy(cr);
	}

	cairo_surface_destroy(rec);
	cairo_surface_destroy(pdf);

	return 0;
}
