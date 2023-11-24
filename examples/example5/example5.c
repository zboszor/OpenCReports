#include <stdio.h>
#include <opencreport.h>

int main(int argc, char **argv) {
    opencreport *o = ocrpt_init();
    ocrpt_datasource *ds = ocrpt_datasource_add_postgresql(o, "pgsql", NULL, NULL, "ocrpttest", "ocrpt", NULL);
    ocrpt_query *q1 = ocrpt_query_add_postgresql(ds, "q1", "select * from flintstones4;");
    ocrpt_query *q2 = ocrpt_query_add_postgresql(ds, "q2", "select * from flintstones5;");

    ocrpt_query_add_follower(q1, q2);

    if (!ocrpt_parse_xml(o, "example5.xml")) {
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
