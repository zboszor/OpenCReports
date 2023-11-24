if ENABLE_DOCS

SGMLFILES = \
	OpenCReports.sgm \
	introduction.sgm \
	xmldescription.sgm \
	breaks.sgm \
	variables.sgm \
	expressions.sgm \
	formatting.sgm \
	c-api-reference.sgm \
	php-api-reference.sgm \
	examples.sgm \
	license.sgm \
	jadetex.cfg

PDF =
if ENABLE_PDF_DOCS
override PDF += OpenCReports-A4.pdf OpenCReports-US.pdf
endif

HTML = OpenCReports.html html

all: $(PDF) $(HTML)

IMAGES = \
	images/namedcolors.svg \
	images/example1.png \
	images/example3.png \
	images/example4.png \
	images/example5.png \
	images/example6.png \
	images/example7.png 

XMLINCLUDE = --path . --path $(srcdir)
XSLTPROCFLAGS = --stringparam ocrpt.version '$(VERSION)'
XSLTPROC_HTML_FLAGS = --param website.stylesheet 1

OpenCReports-full.xml: $(SGMLFILES)
	$(XMLLINT) --nonet $(XMLINCLUDE) --output $@ --noent --valid $<

html-stamp: stylesheet.xsl OpenCReports-full.xml $(IMAGES)
	$(XSLTPROC) --nonet $(XMLINCLUDE) $(XSLTPROCFLAGS) $(XSLTPROC_HTML_FLAGS) $(wordlist 1,2,$^)
	touch $@

html: html-stamp

# single-page HTML
OpenCReports.html: stylesheet-html-nochunk.xsl OpenCReports-full.xml $(IMAGES)
	$(XSLTPROC) --nonet $(XMLINCLUDE) $(XSLTPROCFLAGS) $(XSLTPROC_HTML_FLAGS) -o $@ $(wordlist 1,2,$^)

if ENABLE_PDF_DOCS
XSLTPROC_FO_FLAGS = --stringparam img.src.path '$(srcdir)/'

%-A4.fo: stylesheet-fo.xsl %-full.xml
	$(XSLTPROC) --nonet $(XMLINCLUDE) $(XSLTPROCFLAGS) $(XSLTPROC_FO_FLAGS) --stringparam paper.type A4 -o $@ $^

%-US.fo: stylesheet-fo.xsl %-full.xml
	$(XSLTPROC) --nonet $(XMLINCLUDE) $(XSLTPROCFLAGS) $(XSLTPROC_FO_FLAGS) --stringparam paper.type USletter -o $@ $^

%.pdf: %.fo $(IMAGES)
	$(FOP) -fo $< -pdf $@

endif

EXTRA_DIST = $(SGMLFILES)

CLEANFILES = OpenCReports-A4.pdf OpenCReports-US.pdf html-stamp OpenCReports.html html/*.html

endif
