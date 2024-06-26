#include <stdio.h>
#include <opencreport.h>

int main(int argc, char **argv) {
	const ocrpt_paper *p;
	void *p_iter;
	opencreport *o = ocrpt_init();

	p = ocrpt_get_paper_by_name("A4");
	printf("A4 paper size: %s w %.3lf h %.3lf\n", p->name, p->width, p->height);

	p = ocrpt_get_system_paper();
	printf("system paper size: %s w %.3lf h %.3lf\n", p->name, p->width, p->height);

	p = ocrpt_get_paper(o);
	printf("default paper size: %s w %.3lf h %.3lf\n", p->name, p->width, p->height);

	ocrpt_set_paper_by_name(o, "B5");
	p = ocrpt_get_paper(o);
	printf("set paper size: %s w %.3lf h %.3lf\n", p->name, p->width, p->height);

	printf("supported paper sizes:\n");
	for (p_iter = NULL, p = ocrpt_paper_next(o, &p_iter); p; p = ocrpt_paper_next(o, &p_iter)) {
		printf("\tpaper size: %s w %.3lf h %.3lf\n", p->name, p->width, p->height);
	}

	ocrpt_free(o);

	return 0;
}
