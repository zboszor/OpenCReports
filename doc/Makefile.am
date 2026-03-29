if ENABLE_DOCS

SGMLFILES = \
	$(builddir)/OpenCReports.sgm \
	$(srcdir)/introduction.sgm \
	$(srcdir)/dsandqueries.sgm \
	$(srcdir)/expressions.sgm \
	$(srcdir)/functions.sgm \
	$(srcdir)/variables.sgm \
	$(srcdir)/breaks.sgm \
	$(srcdir)/xmldescription.sgm \
	$(srcdir)/formatting.sgm \
	$(srcdir)/c-api-ref-high.sgm \
	$(srcdir)/c-api-ref-low.sgm \
	$(srcdir)/c-api-ds-impl.sgm \
	$(srcdir)/php-api-reference.sgm \
	$(srcdir)/examples.sgm \
	$(srcdir)/license.sgm \
	$(srcdir)/jadetex.cfg

PDF =
if ENABLE_PDF_DOCS
override PDF += OpenCReports-A4.pdf OpenCReports-US.pdf
endif

HTML = OpenCReports.html html

all: $(PDF) $(HTML)

html/*.html: html

IMAGES = \
	$(srcdir)/images/namedcolors.svg \
	$(srcdir)/images/example1.png \
	$(srcdir)/images/example3.png \
	$(srcdir)/images/example4.png \
	$(srcdir)/images/example5.png \
	$(srcdir)/images/example6.png \
	$(srcdir)/images/example7.png

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

doc_DATA = OpenCReports.html $(PDF)

dochtmldir = $(docdir)/html
dochtml_DATA = $(IMAGES) html/*.html

EXTRA_DIST = $(SGMLFILES)

CLEANFILES = *.fo *.pdf html-stamp *.html html/*.html

endif
