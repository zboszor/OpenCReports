/*
 * OpenCReports TXT output driver
 * Copyright (C) 2019-2026 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _OCRPT_TXT_H_
#define _OCRPT_TXT_H_

#include <stdint.h>

#include "opencreport.h"
#include "common-output.h"

/*
 * One entry per buffered output line. 'started' is true for lines that
 * were claimed by start_data_row (or otherwise written to). Lines that
 * were only ever created as placeholders by get_current_page() - captured
 * for the matched/overlay path but never written into - stay started=false
 * and are dropped at finalize time.
 *
 * 'cursor' is the rightmost character column reached by the previous
 * placement on the current data row, in TXT cells.
 *
 * 'layout_pos' is the corresponding cumulative position as the layout
 * code sees it, in points (i.e. sum of previous elements' width_computed,
 * plus any indent). The two can drift: Pango may exclude trailing
 * whitespace from logical_rect.width, and image/barcode elements take
 * fewer TXT cells than their image_text_width in points implies. Driving
 * placement off cursor (plus any explicit indent computed from the gap
 * between elem->start and layout_pos) keeps subsequent elements packed
 * against the actual rendered content rather than the layout's
 * point-coordinate phantom of it.
 */
struct txt_line {
	ocrpt_string *str;
	bool started;
	int32_t cursor;
	double layout_pos;
};
typedef struct txt_line txt_line;

struct txt_private_data {
	common_private_data base;
	/*
	 * Collected output lines. Each list entry is a txt_line *. They are
	 * concatenated into o->output_buffer in ocrpt_txt_finalize().
	 * Storing them as a list lets matched-and-overlay data rows write
	 * back over previously emitted lines at the column position set by
	 * the line element layout (including indent).
	 */
	ocrpt_list *lines;
	ocrpt_list *last_line;
	ocrpt_list *current_line;
	ocrpt_string *bgimagepfx;
	ocrpt_string *spc_padding;
	/*
	 * True iff the most recent driver callback was get_current_page(),
	 * which marks the start of the runtime's matched-row overlay save/
	 * restore window. set_current_page() honours its redirect only while
	 * this is true so that multi-column rollback (api.c calls
	 * set_current_page(pr->start_page) when crossing to a new <pd>) is
	 * ignored - TXT lays columns out serially, not side by side.
	 */
	bool overlay_active;
};
typedef struct txt_private_data txt_private_data;

void ocrpt_txt_init(opencreport *o);

#endif
