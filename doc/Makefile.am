SGMLFILES = OpenCReports.sgm expressions.sgm jadetex.cfg

all: OpenCReports.pdf

OpenCReports.pdf: $(SGMLFILES)
	SP_ENCODING=XML docbook2pdf -d docbook-utils.dsl OpenCReports.sgm

EXTRA_DIST = $(SGMLFILES)

CLEANFILES = OpenCReports.pdf
