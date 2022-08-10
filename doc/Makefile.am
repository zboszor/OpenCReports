if ENABLE_DOCS

SGMLFILES = \
	OpenCReports.sgm \
	introduction.sgm \
	breaks.sgm \
	variables.sgm \
	expressions.sgm \
	c-api-reference.sgm \
	jadetex.cfg

all: OpenCReports.pdf

OpenCReports.pdf: $(SGMLFILES)
	SP_ENCODING=XML $(DOCBOOK2PDF) -d docbook-utils.dsl OpenCReports.sgm

EXTRA_DIST = $(SGMLFILES)

CLEANFILES = OpenCReports.pdf

endif
