## Makefile.am for OpenCReports

ACLOCAL_AMFLAGS = -I m4

pkgconfig_DATA = opencreports.pc

if ENABLE_DOCS
DOCS = doc
endif

if ENABLE_TESTS
TESTS = tests
else
clean-local:
	$(MAKE) -C tests clean
endif

SUBDIRS = $(DOCS) include libsrc $(TESTS)
