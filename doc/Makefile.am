SGMLFILES = \
	OpenCReports.sgm \
	introduction.sgm \
	xmldescription.sgm \
	breaks.sgm \
	variables.sgm \
	expressions.sgm \
	c-api-reference.sgm \
	jadetex.cfg

if ENABLE_PDF_DOCS
PDF = OpenCReports.pdf
endif

if ENABLE_HTML_DOCS
HTML = html/book1.html
endif

all: $(PDF) $(HTML)

if ENABLE_PDF_DOCS
OpenCReports.pdf: $(SGMLFILES)
	SP_ENCODING=XML $(DOCBOOK2PDF) -d docbook-utils.dsl OpenCReports.sgm
endif

if ENABLE_HTML_DOCS
html/book1.html: $(SGMLFILES)
	SP_ENCODING=XML $(DOCBOOK2HTML) -o html OpenCReports.sgm
endif

EXTRA_DIST = $(SGMLFILES)

CLEANFILES = OpenCReports.pdf
