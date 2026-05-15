/*
 * OpenCReports TXT output driver
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <config.h>

#include <string.h>
#include <math.h>

#include "ocrpt-private.h"
#include "exprutil.h"
#include "formatting.h"
#include "parts.h"
#include "txt-output.h"

/*
 * The TXT driver buffers each output line as an ocrpt_string in priv->lines.
 * draw_text(), draw_image() and draw_barcode() compute a character column from
 * the layout's point coordinates and place their content at that column inside
 * priv->current_line, padding with spaces or overwriting whatever was there
 * before. Lines are finalised (joined with '\n' into o->output_buffer) only in
 * ocrpt_txt_finalize().
 *
 * Overlay support comes from the get_current_page / set_current_page callbacks.
 * Before rendering a fielddetails row the runtime captures the "current page"
 * (a list node into priv->lines). When the same matched key appears later, it
 * calls set_current_page() to roll the writes back onto the earlier line -
 * draw_*() then overwrite the captured line at the right column.
 *
 * The driver assumes character-based positioning. When the report's sizeunit
 * is "char" the layout has already multiplied positions by line->font_width
 * (= one character in points), so dividing by font_width recovers character
 * columns. When the sizeunit is "points", the same font_width - the default
 * character size set for the line - is used as the divisor, which is the
 * best approximation TXT can offer.
 */

static double ocrpt_txt_char_width(opencreport *o, ocrpt_line *line) {
	if (line && line->font_width > 0.0)
		return line->font_width;
	if (o && o->font_width > 0.0)
		return o->font_width;
	return OCRPT_DEFAULT_FONT_SIZE;
}


static txt_line *ocrpt_txt_new_line(void) {
	txt_line *tl = ocrpt_mem_malloc(sizeof(txt_line));
	tl->str = ocrpt_mem_string_new_with_len(NULL, 64);
	tl->started = false;
	tl->cursor = 0;
	tl->layout_pos = 0.0;
	return tl;
}

/*
 * Make priv->current_line point to a valid txt_line and mark it started.
 * If the current line is unset (fresh after end_data_row) a new empty
 * line is appended.
 */
static ocrpt_string *ocrpt_txt_current(txt_private_data *priv) {
	if (!priv->current_line) {
		txt_line *tl = ocrpt_txt_new_line();
		priv->lines = ocrpt_list_end_append(priv->lines, &priv->last_line, tl);
		priv->current_line = priv->last_line;
	}
	txt_line *tl = (txt_line *)priv->current_line->data;
	tl->started = true;
	return tl->str;
}

/*
 * Return the byte offset in 'line' that corresponds to character column
 * 'char_col'. If the line has fewer characters than that, pad it with spaces
 * so its character count reaches 'char_col' and return the resulting end
 * byte offset. This keeps the rest of the placement code addressable in
 * character columns even though the underlying storage is UTF-8 bytes.
 */
static int32_t ocrpt_txt_byte_at_char(ocrpt_string *line, int32_t char_col) {
	int32_t chars = 0;
	int32_t i = 0;
	while (i < (int32_t)line->len && chars < char_col) {
		unsigned char c = (unsigned char)line->str[i];
		if (c < 0x80)
			i += 1;
		else if ((c & 0xE0) == 0xC0)
			i += 2;
		else if ((c & 0xF0) == 0xE0)
			i += 3;
		else if ((c & 0xF8) == 0xF0)
			i += 4;
		else
			i += 1;
		chars++;
	}
	if (chars < char_col) {
		int32_t pad = char_col - chars;
		size_t need = line->len + pad + 1;
		if (line->allocated_len < need)
			ocrpt_mem_string_resize(line, need + 16);
		memset(line->str + line->len, ' ', pad);
		line->len += pad;
		line->str[line->len] = '\0';
		i = line->len;
	}
	return i;
}

/*
 * Replace 'nchars' characters starting at character column 'char_col' in
 * 'line' with 'bytes' bytes from 'str'. The byte sequence may have a
 * different byte length than the characters it replaces; the buffer is
 * adjusted accordingly.
 */
static void ocrpt_txt_place(ocrpt_string *line, int32_t char_col, const char *str, int32_t bytes, int32_t nchars) {
	if (bytes <= 0 || nchars <= 0)
		return;
	int32_t b1 = ocrpt_txt_byte_at_char(line, char_col);
	int32_t b2 = ocrpt_txt_byte_at_char(line, char_col + nchars);
	int32_t replaced = b2 - b1;
	int32_t delta = bytes - replaced;
	if (delta > 0) {
		size_t need = line->len + delta + 1;
		if (line->allocated_len < need)
			ocrpt_mem_string_resize(line, need + 16);
		memmove(line->str + b2 + delta, line->str + b2, line->len - b2);
		line->len += delta;
	} else if (delta < 0) {
		memmove(line->str + b2 + delta, line->str + b2, line->len - b2);
		line->len += delta;
	}
	memcpy(line->str + b1, str, bytes);
	line->str[line->len] = '\0';
}

/*
 * Place 'count' spaces at character column 'char_col' in 'line', either
 * appending (if the line is shorter) or overwriting the next 'count'
 * characters in place. Spaces are single-byte so byte/char counts coincide.
 */
static void ocrpt_txt_place_spaces(ocrpt_string *line, int32_t char_col, int32_t count) {
	if (count <= 0)
		return;
	int32_t b1 = ocrpt_txt_byte_at_char(line, char_col);
	int32_t b2 = ocrpt_txt_byte_at_char(line, char_col + count);
	int32_t replaced = b2 - b1;
	int32_t delta = count - replaced;
	if (delta > 0) {
		size_t need = line->len + delta + 1;
		if (line->allocated_len < need)
			ocrpt_mem_string_resize(line, need + 16);
		memmove(line->str + b2 + delta, line->str + b2, line->len - b2);
		line->len += delta;
	} else if (delta < 0) {
		memmove(line->str + b2 + delta, line->str + b2, line->len - b2);
		line->len += delta;
	}
	memset(line->str + b1, ' ', count);
	line->str[line->len] = '\0';
}

static void ocrpt_txt_start_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, double page_indent, double y) {
	txt_private_data *priv = o->output_private;
	/*
	 * Make sure there's a line we will write into. If set_current_page()
	 * just pointed us at an earlier line (overlay), we reuse that line so
	 * the upcoming draw_*() calls overwrite it in place.
	 */
	ocrpt_txt_current(priv);
	/*
	 * Reset the per-row tracking: each new data row starts laying out
	 * from its leftmost element. Overlay rows reset too - they overwrite
	 * the captured line starting from col 0 of the new row's layout.
	 */
	txt_line *tl_reset = (txt_line *)priv->current_line->data;
	tl_reset->cursor = 0;
	tl_reset->layout_pos = 0.0;
}

static void ocrpt_txt_end_data_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l) {
	txt_private_data *priv = o->output_private;
	/*
	 * Drop the current line handle. The next start_data_row() (or
	 * get_current_page() prior to it) decides which line to use.
	 */
	priv->current_line = NULL;
}

static void ocrpt_txt_start_part_column(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd) {
	txt_private_data *priv = o->output_private;
	/*
	 * Close the overlay save/restore window when we cross into a new
	 * <pd> column. This is the only place that runs between the runtime's
	 * end-of-column get_current_page and its rollback set_current_page
	 * in api.c ocrpt_execute_parts(), so we can use it to keep the
	 * rollback from being honoured as if it were an overlay redirect.
	 */
	priv->overlay_active = false;
}

static void ocrpt_txt_end_part_column(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd) {
	txt_private_data *priv = o->output_private;
	priv->overlay_active = false;
}

static void ocrpt_txt_end_part_row(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr) {
	txt_private_data *priv = o->output_private;
	/*
	 * Same protection for the set_current_page(pr->end_page) call at
	 * api.c:ocrpt_execute_parts() that fires after the last column of
	 * a part row.
	 */
	priv->overlay_active = false;
}

static void *ocrpt_txt_get_current_page(opencreport *o) {
	txt_private_data *priv = o->output_private;

	/*
	 * Return NULL until at least one line has been emitted. This keeps
	 * the runtime's "flush previous page" newpage block in
	 * ocrpt_layout_output() from firing on the very first call, where it
	 * would otherwise render the page header and page footer before any
	 * data rows. Once a line exists, a NULL current_line just means we
	 * are between data rows: lazily allocate a placeholder so the captured
	 * handle survives a subsequent set_current_page() redirect (used by
	 * the matched-row / overlay path). Placeholders are marked
	 * started=false so finalize can drop them if start_data_row is never
	 * called for the row that would have populated them.
	 *
	 * Also mark the overlay save/restore window open - only the
	 * set_current_page calls in api.c:ocrpt_execute_one_report() are
	 * sandwiched between two get_current_page calls; the multi-column
	 * rollback at api.c:ocrpt_execute_parts() fires set_current_page
	 * out of nowhere and is therefore filtered out by set_current_page()
	 * below.
	 */
	if (!priv->lines && !priv->current_line)
		return NULL;

	if (!priv->current_line) {
		txt_line *tl = ocrpt_txt_new_line();
		priv->lines = ocrpt_list_end_append(priv->lines, &priv->last_line, tl);
		priv->current_line = priv->last_line;
	}
	priv->overlay_active = true;
	return priv->current_line;
}

static void ocrpt_txt_set_current_page(opencreport *o, void *page) {
	txt_private_data *priv = o->output_private;
	if (page == NULL) {
		/*
		 * The runtime resets the page state by calling set_current_page(NULL)
		 * at the start of each pass (precalc and render). Mirror that into
		 * common's page tracking so add_new_page() sees a clean state.
		 * Without this, pageno would be incremented in the render pass
		 * because the precalc pass left priv->current_page pointing at the
		 * first page.
		 */
		priv->current_line = NULL;
		priv->base.current_page = NULL;
		priv->overlay_active = false;
		return;
	}
	/*
	 * Honour the redirect only while the overlay window is open. This
	 * filters the multi-column rollback in api.c:ocrpt_execute_parts():
	 * the text output serialises columns linearly, so column 2 must
	 * append after column 1 rather than overwriting it.
	 */
	if (priv->overlay_active)
		priv->current_line = (ocrpt_list *)page;
}

static void ocrpt_txt_draw_text(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *l, ocrpt_text *le, bool last, double page_width, double page_indent, double y) {
	txt_private_data *priv = o->output_private;
	ocrpt_string *line = ocrpt_txt_current(priv);
	double cw = ocrpt_txt_char_width(o, l);
#if PANGO_VERSION_CHECK(1,50,0)
	int startpos = le->pline ? pango_layout_line_get_start_index(le->pline) : 0;
	int length = le->pline ? pango_layout_line_get_length(le->pline) : 0;
#else
	int startpos = le->pline ? le->pline->start_index : 0;
	int length = le->pline ? le->pline->length : 0;
#endif
	int32_t l2, bl2;
	int32_t field_width;
	/*
	 * The text driver doesn't honour page-level margins or column indents,
	 * matching the historical behaviour. Only the element's own start
	 * offset (which already accounts for line-element indent) is used.
	 */
	txt_line *cur_tl = (txt_line *)priv->current_line->data;
	/*
	 * Position by cursor + any explicit indent the layout introduced.
	 * indent_pts is the gap between this element's layout start and
	 * where the previous element ended (in points). Drift caused by
	 * Pango trailing-whitespace measurement or image render-vs-layout
	 * width mismatches is < 1 cell on either side and is rounded away,
	 * leaving real indent values intact.
	 */
	double indent_pts = le->start - cur_tl->layout_pos;
	int32_t indent_chars = (indent_pts > 0.0) ? (int32_t)round(indent_pts / cw) : 0;
	int32_t col_start = cur_tl->cursor + indent_chars;

	if (le->width) {
		field_width = (int32_t)ceil(le->width_computed / cw);
		if (field_width <= 0)
			field_width++;

		ocrpt_utf8forward(le->result_str->str + startpos, le->memo ? field_width : length, &l2, length, &bl2);
	} else {
		ocrpt_utf8forward(le->result_str->str + startpos, length, &l2, length, &bl2);
		field_width = ((le->p_align == PANGO_ALIGN_LEFT || ocrpt_list_length(l->elements) > 1) ? l2 : (int32_t)(page_width / cw));
	}

	if (field_width >= l2) {
		/*
		 * Designated field width is wider than the text.
		 * Pad with spaces.
		 */
		int32_t nspc = field_width - l2;
		int32_t nspc1 = nspc >> 1, nspc2 = nspc - nspc1;

		switch (le->p_align) {
		case PANGO_ALIGN_LEFT:
			ocrpt_txt_place(line, col_start, le->result_str->str + startpos, bl2, l2);
			ocrpt_txt_place_spaces(line, col_start + l2, nspc);
			break;
		case PANGO_ALIGN_RIGHT:
			ocrpt_txt_place_spaces(line, col_start, nspc);
			ocrpt_txt_place(line, col_start + nspc, le->result_str->str + startpos, bl2, l2);
			break;
		case PANGO_ALIGN_CENTER:
			ocrpt_txt_place_spaces(line, col_start, nspc1);
			ocrpt_txt_place(line, col_start + nspc1, le->result_str->str + startpos, bl2, l2);
			ocrpt_txt_place_spaces(line, col_start + nspc1 + l2, nspc2);
			break;
		}
	} else {
		/*
		 * Designated field width is narrower. Truncate the text.
		 */
		int32_t trnc = l2 - field_width;
		int32_t trnc1 = trnc >> 1;
		int bl3, l3;

		switch (le->p_align) {
		case PANGO_ALIGN_LEFT:
			ocrpt_utf8forward(le->result_str->str + startpos, field_width, &l2, length, &bl2);
			ocrpt_txt_place(line, col_start, le->result_str->str + startpos, bl2, l2);
			break;
		case PANGO_ALIGN_RIGHT:
			ocrpt_utf8backward(le->result_str->str + startpos, field_width, &l2, length, &bl2);
			ocrpt_txt_place(line, col_start, le->result_str->str + startpos + bl2, le->result_str->len - startpos - bl2, l2);
			break;
		case PANGO_ALIGN_CENTER:
			ocrpt_utf8forward(le->result_str->str + startpos, trnc1, &l3, length, &bl2);
			ocrpt_utf8forward(le->result_str->str + startpos + bl2, field_width, &l2, length - bl2, &bl3);
			ocrpt_txt_place(line, col_start, le->result_str->str + startpos + bl2, bl3, l2);
			break;
		}
	}

	/*
	 * Advance both the cell cursor and the layout-point shadow so the
	 * next element can compute its indent from this element's end.
	 */
	int32_t end_col = col_start + (field_width > l2 ? field_width : l2);
	if (end_col > cur_tl->cursor)
		cur_tl->cursor = end_col;
	cur_tl->layout_pos = le->start + le->width_computed;

	if (l->current_line + 1 < le->lines) {
		le->pline = pango_layout_get_line(le->layout, l->current_line + 1);
		pango_layout_line_get_extents(le->pline, NULL, &le->p_rect);
	} else
		le->pline = NULL;
}

static void ocrpt_txt_draw_image(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_image *img, bool last, double page_width, double page_indent, double x, double y, double w, double h) {
	txt_private_data *priv = o->output_private;

	if (line) {
		ocrpt_string *cur = ocrpt_txt_current(priv);
		txt_line *cur_tl = (txt_line *)priv->current_line->data;
		double cw = ocrpt_txt_char_width(o, line);
		double indent_pts = img->start - cur_tl->layout_pos;
		int32_t indent_chars = (indent_pts > 0.0) ? (int32_t)round(indent_pts / cw) : 0;
		int32_t col_start = cur_tl->cursor + indent_chars;
		/*
		 * For the image's TXT width, follow the legacy convention of
		 * dividing by the line's font size expression (the value the
		 * user wrote, not the resolved metric - TXT forces every line
		 * down to a 12pt Courier face, so line->fontsz wouldn't reflect
		 * an explicit line_font_size="14" on a genline). The expected
		 * outputs were captured under this rule.
		 */
		double size;
		if (line && EXPR_VALID_NUMERIC(line->font_size))
			size = mpfr_get_d(EXPR_NUMERIC(line->font_size), EXPR_RNDMODE(line->font_size));
		else if (r && r->font_size_expr)
			size = r->font_size;
		else if (p && p->font_size_expr)
			size = p->font_size;
		else
			size = OCRPT_DEFAULT_FONT_SIZE;
		int32_t nspc = (int32_t)round(w / size);
		if (nspc < 1)
			nspc = 1;
		/*
		 * In-line image: claim its space with blanks so subsequent
		 * line elements line up at their own column positions.
		 */
		ocrpt_txt_place_spaces(cur, col_start, nspc);
		if (col_start + nspc > cur_tl->cursor)
			cur_tl->cursor = col_start + nspc;
		cur_tl->layout_pos = img->start + img->image_text_width;
	} else {
		/*
		 * Background image: keep the historical bgimagepfx state for
		 * any caller that may inspect it.
		 */
		double size;
		uint32_t nspc, i;

		if (r && r->font_size_expr)
			size = r->font_size;
		else if (p && p->font_size_expr)
			size = p->font_size;
		else
			size = OCRPT_DEFAULT_FONT_SIZE;

		nspc = (uint32_t)round(w / size);

		if ((nspc + 1) > priv->bgimagepfx->allocated_len)
			ocrpt_mem_string_resize(priv->bgimagepfx, nspc + 1);

		for (i = 0; i < nspc; i++)
			priv->bgimagepfx->str[i] = ' ';
		priv->bgimagepfx->str[nspc] = 0;
		priv->bgimagepfx->len = nspc;
	}
}

static void ocrpt_txt_draw_barcode(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output, ocrpt_line *line, ocrpt_barcode *bc, bool last, double page_width, double page_indent, double x, double y, double w, double h) {
	txt_private_data *priv = o->output_private;

	if (line) {
		ocrpt_string *cur = ocrpt_txt_current(priv);
		txt_line *cur_tl = (txt_line *)priv->current_line->data;
		double cw = ocrpt_txt_char_width(o, line);
		double indent_pts = bc->start - cur_tl->layout_pos;
		int32_t indent_chars = (indent_pts > 0.0) ? (int32_t)round(indent_pts / cw) : 0;
		int32_t col_start = cur_tl->cursor + indent_chars;
		/*
		 * Barcodes use the line's character width as the divisor (legacy
		 * expected-output convention - distinct from images, which use
		 * the line's font size).
		 */
		int32_t nspc = (int32_t)round(w / cw);
		if (nspc < 1)
			nspc = 1;
		ocrpt_txt_place_spaces(cur, col_start, nspc);
		if (col_start + nspc > cur_tl->cursor)
			cur_tl->cursor = col_start + nspc;
		cur_tl->layout_pos = bc->start + bc->barcode_width;
	} else {
		double size;
		uint32_t nspc, i;

		if (r && r->font_size_expr)
			size = r->font_width;
		else if (p && p->font_size_expr)
			size = p->font_width;
		else
			size = OCRPT_DEFAULT_FONT_SIZE;

		nspc = (uint32_t)round(w / size);

		if ((nspc + 1) > priv->bgimagepfx->allocated_len)
			ocrpt_mem_string_resize(priv->bgimagepfx, nspc + 1);

		for (i = 0; i < nspc; i++)
			priv->bgimagepfx->str[i] = ' ';
		priv->bgimagepfx->str[nspc] = 0;
		priv->bgimagepfx->len = nspc;
	}
}

static void ocrpt_txt_draw_imageend(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output) {
	txt_private_data *priv = o->output_private;

	priv->bgimagepfx->len = 0;
}

static void ocrpt_txt_end_output(opencreport *o, ocrpt_part *p, ocrpt_part_row *pr, ocrpt_part_column *pd, ocrpt_report *r, ocrpt_break *br, ocrpt_output *output) {
	txt_private_data *priv = o->output_private;

	priv->bgimagepfx->len = 0;
}

static void ocrpt_txt_finalize(opencreport *o) {
	txt_private_data *priv = o->output_private;

	/*
	 * Flush collected lines into o->output_buffer, joined with '\n'.
	 * Placeholder lines (created by get_current_page() for the overlay
	 * path but never claimed by a start_data_row) are skipped so they
	 * don't leave gaps. Intentionally-empty rows (e.g. <Line/> elements
	 * inside break headers / footers) keep their blank line because
	 * start_data_row marked them started.
	 */
	for (ocrpt_list *l = priv->lines; l; l = l->next) {
		txt_line *tl = (txt_line *)l->data;
		if (tl->started) {
			ocrpt_mem_string_append_len(o->output_buffer, tl->str->str, tl->str->len);
			ocrpt_mem_string_append(o->output_buffer, "\n");
		}
		ocrpt_mem_string_free(tl->str, true);
		ocrpt_mem_free(tl);
	}
	ocrpt_list_free(priv->lines);
	priv->lines = NULL;
	priv->last_line = NULL;
	priv->current_line = NULL;

	ocrpt_mem_string_free(priv->bgimagepfx, true);
	ocrpt_mem_string_free(priv->spc_padding, true);

	ocrpt_common_finalize(o);

	ocrpt_string **content_type = ocrpt_mem_malloc(4 * sizeof(ocrpt_string *));
	content_type[0] = ocrpt_mem_string_new_printf("Content-Type: text/plain; charset=utf-8\n");
	content_type[1] = ocrpt_mem_string_new_printf("Content-Length: %zu", o->output_buffer->len);
	content_type[2] = ocrpt_mem_string_new_printf("Content-Disposition: attachment; filename=%s", o->csv_filename ? o->csv_filename : "report.txt");
	content_type[3] = NULL;
	o->content_type = (const ocrpt_string **)content_type;
}

void ocrpt_txt_init(opencreport *o) {
	ocrpt_common_init(o, sizeof(txt_private_data), 4096, 65536);
	o->output_functions.set_font_sizes = ocrpt_common_set_font_sizes;
	o->output_functions.get_text_sizes = ocrpt_common_get_text_sizes;
	o->output_functions.start_data_row = ocrpt_txt_start_data_row;
	o->output_functions.end_data_row = ocrpt_txt_end_data_row;
	o->output_functions.start_part_column = ocrpt_txt_start_part_column;
	o->output_functions.end_part_column = ocrpt_txt_end_part_column;
	o->output_functions.end_part_row = ocrpt_txt_end_part_row;
	o->output_functions.end_output = ocrpt_txt_end_output;
	o->output_functions.draw_image = ocrpt_txt_draw_image;
	o->output_functions.draw_barcode = ocrpt_txt_draw_barcode;
	o->output_functions.draw_imageend = ocrpt_txt_draw_imageend;
	o->output_functions.draw_text = ocrpt_txt_draw_text;
	o->output_functions.get_current_page = ocrpt_txt_get_current_page;
	o->output_functions.set_current_page = ocrpt_txt_set_current_page;
	o->output_functions.finalize = ocrpt_txt_finalize;
	o->output_functions.support_fontdesc = true;

	txt_private_data *priv = o->output_private;
	priv->bgimagepfx = ocrpt_mem_string_new_with_len(NULL, 64);
	priv->spc_padding = ocrpt_mem_string_new_with_len(NULL, 64);
}
