#include <stdio.h>
#include <opencreport.h>

int main(int argc, char **argv) {
    opencreport *o = ocrpt_init();
    ocrpt_datasource *ds = ocrpt_datasource_add_postgresql(o, "pgsql", NULL, NULL, "ocrpttest", "ocrpt", NULL);
    ocrpt_query *q1 = ocrpt_query_add_postgresql(ds, "q1", "select * from data;");
    ocrpt_query *q2 = ocrpt_query_add_postgresql(ds, "q2", "select * from more_data;");
    ocrpt_query *q3 = ocrpt_query_add_postgresql(ds, "q3", "select * from moar_data;");

    ocrpt_expr *match = ocrpt_expr_parse(o, "q1.id = q2.boss_id", NULL);
    ocrpt_query_add_follower_n_to_1(q1, q2, match);

    ocrpt_expr *match2 = ocrpt_expr_parse(o, "q2.id = q3.sk_id", NULL);
    ocrpt_query_add_follower_n_to_1(q2, q3, match2);

    if (!ocrpt_parse_xml(o, "example7.xml")) {
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
