/*
 * OpenCReports test
 * Copyright (C) 2019-2025 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencreport.h>
#include "test_common.h"

const char *array[1][5] = {
	{ "id", "name", "property", "age", "adult" },
};

const int32_t coltypes[5] = {
	OCRPT_RESULT_NUMBER, OCRPT_RESULT_STRING, OCRPT_RESULT_STRING, OCRPT_RESULT_NUMBER, OCRPT_RESULT_NUMBER
};

int main(int argc, char **argv) {
	opencreport *o = ocrpt_init();

	if (!ocrpt_parse_xml(o, "layout_xml_test.xml")) {
		printf("XML parse error\n");
		ocrpt_free(o);
		return 0;
	}

	/*
	 * Modify all horizontal lines in <NoData> to use a custom color.
	 */
	ocrpt_report *r = get_first_report(o);
	ocrpt_output *output = ocrpt_layout_report_nodata(r);
	void *iter = NULL;
	ocrpt_output_element *elem;

	while ((elem = ocrpt_output_iterate_elements(output, &iter))) {
		if (ocrpt_output_element_is_hline(elem)) {
			ocrpt_hline *hline = (ocrpt_hline *)elem;

			ocrpt_hline_set_color(hline, "'green'");
		} else if (ocrpt_output_element_is_line(elem)) {
			/* Modify all text entries in the line to a custom string. */
			ocrpt_line *line = (ocrpt_line *)elem;
			void *iter1 = NULL;
			ocrpt_line_element *lelem;

			while ((lelem = ocrpt_line_iterate_elements(line, &iter1))) {
				if (ocrpt_line_element_is_text(lelem)) {
					ocrpt_text *text = (ocrpt_text *)lelem;

					ocrpt_text_set_value_string(text, "xxx");
				}
			}
		}
	}

	ocrpt_set_output_format(o, argc >= 2 ? atoi(argv[1]) : OCRPT_OUTPUT_PDF);

	ocrpt_execute(o);

	ocrpt_spool(o);

	ocrpt_free(o);

	return 0;
}
