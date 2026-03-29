## doc/meson.build for OpenCReports
##
## Generates HTML and optionally PDF documentation from SGML sources.
## All output lands in the meson build directory's doc/ subdirectory.
## Source files (SGML, XSL, images) are read from the source doc/ directory.
##
## Installation mirrors Makefile.am:
##   doc_DATA      = OpenCReports.html $(PDF)   → $(docdir)/
##   dochtml_DATA  = $(IMAGES) html/*.html       → $(docdir)/html/

xmllint  = find_program('xmllint',  required: true)
xsltproc = find_program('xsltproc', required: true)
fop      = find_program('fop',      required: false)

enable_pdf_docs = fop.found()

# Absolute paths evaluated at configure time.
_src = meson.current_source_dir()   # .../doc  (source tree)
_bld = meson.current_build_dir()    # .../build/doc  (output tree)
_ver = meson.project_version()

# Installation root: $(docdir) = $(datarootdir)/doc/opencreports
docdir     = join_paths(get_option('datadir'), 'doc', 'opencreports')
dochtmldir = join_paths(docdir, 'html')

# ---------------------------------------------------------------------------
# OpenCReports.sgm is generated from OpenCReports.sgm.in by substituting
# @VERSION@.  This mirrors the AC_CONFIG_FILES entry in configure.ac.
# The result lands in the build doc/ directory.
# ---------------------------------------------------------------------------
opencreports_sgm = configure_file(
  input:         'OpenCReports.sgm.in',
  output:        'OpenCReports.sgm',
  configuration: {'VERSION': _ver},
)

# Static SGML include files (all in the source doc/ directory).
sgml_includes = files(
  'introduction.sgm',
  'dsandqueries.sgm',
  'expressions.sgm',
  'functions.sgm',
  'variables.sgm',
  'breaks.sgm',
  'xmldescription.sgm',
  'formatting.sgm',
  'c-api-ref-high.sgm',
  'c-api-ref-low.sgm',
  'c-api-ds-impl.sgm',
  'php-api-reference.sgm',
  'examples.sgm',
  'license.sgm',
  'jadetex.cfg',
)

# Extra XSL files included by the main stylesheets – listed so that meson
# rebuilds the doc targets when any of them changes.
extra_stylesheets = files(
  'stylesheet-common.xsl',
  'stylesheet-html-common.xsl',
  'stylesheet-speedup-common.xsl',
  'stylesheet-speedup-xhtml.xsl',
)

# Images installed to $(docdir)/html/ (basename only, mirroring automake
# _DATA behaviour which strips the leading images/ directory component).
doc_images = files(
  'images/namedcolors.svg',
  'images/example1.png',
  'images/example3.png',
  'images/example4.png',
  'images/example5.png',
  'images/example6.png',
  'images/example7.png',
)

install_data(doc_images, install_dir: dochtmldir)

# ---------------------------------------------------------------------------
# Step 1: xmllint – validate SGML and expand all entities into one XML file.
#
# Equivalent to:
#   $(XMLLINT) --nonet --path . --path $(srcdir) \
#              --output OpenCReports-full.xml --noent --valid OpenCReports.sgm
# ---------------------------------------------------------------------------
full_xml = custom_target('OpenCReports-full.xml',
  input:            [opencreports_sgm, sgml_includes],
  output:           'OpenCReports-full.xml',
  build_by_default: true,
  command: [
    xmllint, '--nonet',
    '--path', _src,
    '--path', _bld,
    '--output', '@OUTPUT@',
    '--noent', '--valid',
    _bld / 'OpenCReports.sgm',
  ],
)

# ---------------------------------------------------------------------------
# Step 2a: xsltproc – chunked (multi-page) HTML → build/doc/html/
#
# stylesheet.xsl contains:
#   <xsl:param name="base.dir" select="'html/'"/>
# so xsltproc writes all pages to  html/  relative to its working directory.
# We must run xsltproc from _bld so the html/ tree lands in build/doc/html/.
#
# All paths are passed as explicit shell positional parameters because
# @INPUT0@ / @OUTDIR@ do not expand inside sh -c strings:
#   $1 = build/doc dir (cd target + --path arg)
#   $2 = xsltproc binary
#   $3 = source/doc dir (--path arg for stylesheet includes)
#   $4 = version string
#   $5 = stylesheet.xsl (absolute source path)
#   $6 = OpenCReports-full.xml (absolute build path)
#   $7 = html-stamp (absolute build path, for touch)
#
# Equivalent to:
#   cd build/doc && xsltproc --nonet --path srcdir --path builddir \
#     --stringparam ocrpt.version VERSION --param website.stylesheet 1 \
#     stylesheet.xsl OpenCReports-full.xml
#   touch build/doc/html-stamp
# ---------------------------------------------------------------------------
html_stamp = custom_target('html-stamp',
  input:            [files('stylesheet.xsl'), full_xml, extra_stylesheets, doc_images],
  output:           'html-stamp',
  build_by_default: true,
  command: [
    'sh', '-c',
    'cd "$1" && "$2" --nonet --path "$3" --path "$1"'
      + ' --stringparam ocrpt.version "$4" --param website.stylesheet 1'
      + ' "$5" "$6" && touch "$7"',
    '_',                            # $0 (argv[0] / script name)
    _bld,                           # $1
    xsltproc.full_path(),           # $2
    _src,                           # $3
    _ver,                           # $4
    _src / 'stylesheet.xsl',        # $5
    _bld / 'OpenCReports-full.xml', # $6
    _bld / 'html-stamp',            # $7
  ],
)

# Install html/*.html to $(docdir)/html/.
# meson install always runs the build step first, so html_stamp (and therefore
# build/doc/html/) is guaranteed to exist when the install script runs.
meson.add_install_script(
  'sh', '-c',
  'install -d "$MESON_INSTALL_DESTDIR_PREFIX/' + dochtmldir + '"'
    + ' && for f in "' + _bld + '"/html/*.html;'
    + ' do install -m 644 "$f" "$MESON_INSTALL_DESTDIR_PREFIX/' + dochtmldir + '/"; done',
)

# ---------------------------------------------------------------------------
# Step 2b: xsltproc – single-page HTML → build/doc/OpenCReports.html
#
# Installed to $(docdir)/ (doc_DATA in Makefile.am).
#
# Equivalent to:
#   xsltproc --nonet --path srcdir --path builddir \
#     --stringparam ocrpt.version VERSION --param website.stylesheet 1 \
#     -o OpenCReports.html stylesheet-html-nochunk.xsl OpenCReports-full.xml
# ---------------------------------------------------------------------------
single_html = custom_target('OpenCReports.html',
  input:            [files('stylesheet-html-nochunk.xsl'), full_xml,
                     extra_stylesheets, doc_images],
  output:           'OpenCReports.html',
  build_by_default: true,
  install:          true,
  install_dir:      docdir,
  command: [
    xsltproc, '--nonet',
    '--path', _src,
    '--path', _bld,
    '--stringparam', 'ocrpt.version', _ver,
    '--param', 'website.stylesheet', '1',
    '-o', '@OUTPUT@',
    _src / 'stylesheet-html-nochunk.xsl',
    _bld / 'OpenCReports-full.xml',
  ],
)

# ---------------------------------------------------------------------------
# Steps 3–4: FO intermediate files + PDF via FOP (only when fop is found).
# PDFs are installed to $(docdir)/ (doc_DATA in Makefile.am).
# ---------------------------------------------------------------------------
if enable_pdf_docs
  # Step 3a: xsltproc → FO (A4)
  fo_a4 = custom_target('OpenCReports-A4.fo',
    input:            [files('stylesheet-fo.xsl'), full_xml, extra_stylesheets, doc_images],
    output:           'OpenCReports-A4.fo',
    build_by_default: true,
    command: [
      xsltproc, '--nonet',
      '--path', _src,
      '--path', _bld,
      '--stringparam', 'ocrpt.version',  _ver,
      '--stringparam', 'img.src.path',   _src + '/',
      '--stringparam', 'paper.type',     'A4',
      '-o', '@OUTPUT@',
      _src / 'stylesheet-fo.xsl',
      _bld / 'OpenCReports-full.xml',
    ],
  )

  # Step 3b: xsltproc → FO (US letter)
  fo_us = custom_target('OpenCReports-US.fo',
    input:            [files('stylesheet-fo.xsl'), full_xml, extra_stylesheets, doc_images],
    output:           'OpenCReports-US.fo',
    build_by_default: true,
    command: [
      xsltproc, '--nonet',
      '--path', _src,
      '--path', _bld,
      '--stringparam', 'ocrpt.version',  _ver,
      '--stringparam', 'img.src.path',   _src + '/',
      '--stringparam', 'paper.type',     'USletter',
      '-o', '@OUTPUT@',
      _src / 'stylesheet-fo.xsl',
      _bld / 'OpenCReports-full.xml',
    ],
  )

  # Step 4a: FOP → PDF (A4), installed to $(docdir)/
  custom_target('OpenCReports-A4.pdf',
    input:            [fo_a4, doc_images],
    output:           'OpenCReports-A4.pdf',
    build_by_default: true,
    install:          true,
    install_dir:      docdir,
    command: [fop, '-fo', _bld / 'OpenCReports-A4.fo', '-pdf', '@OUTPUT@'],
  )

  # Step 4b: FOP → PDF (US letter), installed to $(docdir)/
  custom_target('OpenCReports-US.pdf',
    input:            [fo_us, doc_images],
    output:           'OpenCReports-US.pdf',
    build_by_default: true,
    install:          true,
    install_dir:      docdir,
    command: [fop, '-fo', _bld / 'OpenCReports-US.fo', '-pdf', '@OUTPUT@'],
  )
endif
