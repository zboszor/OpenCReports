#include <stdio.h>
#include <opencreport.h>

int main(int argc, char **argv) {
    opencreport *o = ocrpt_init();

    if (!ocrpt_parse_xml(o, "example1.xml")) {
        printf("XML parse error\n");
        ocrpt_free(o);
        return 0;
    }

    ocrpt_set_output_format(o, OCRPT_OUTPUT_PDF);
    ocrpt_execute(o);
    ocrpt_spool(o);
    ocrpt_free(o);

    return 0;
}
