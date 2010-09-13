## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir)

#
# Shared library
#

bufrexincludedir = $(includedir)/dballe/bufrex

# Include the .cc files that contain template definitions
bufrexinclude_HEADERS = \
		dtable.h \
		opcode.h \
		subset.h \
		msg.h \
		codec.h

noinst_LTLIBRARIES = libdballe-bufrex.la
libdballe_bufrex_la_SOURCES = \
		template-aliases.cc \
		opcode.cc \
		dtable.cc \
		subset.cc \
		msg.cc \
		bufr_decoder.cc \
		bufr_encoder.cc \
		crex_decoder.cc \
		crex_encoder.cc \
		codec.cc
libdballe_bufrex_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

template-aliases.cc: template-aliases.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

template-aliases.tex: template-aliases.gperf mktaliases
	if ! $(srcdir)/mktaliases < $< ; then rm $@; /bin/false; fi
	
distclean-local:
	rm -f template-aliases.tex

BUILT_SOURCES=template-aliases.cc template-aliases.tex
EXTRA_DIST=template-aliases.gperf mktaliases

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = test-utils-bufrex.h

tut_test_SOURCES = \
	../core/test-utils-core.cc \
	test-utils-bufrex.cc \
	tests/opcode.cc \
	tests/dtable.cc \
	tests/msg.cc \
	tests/bufr_decoder.cc \
	tests/bufr_encoder.cc \
	tests/crex_decoder.cc \
	../core/tests/tut-main.cpp
tut_test_LDADD = \
	../core/libdballe-core.la \
	libdballe-bufrex.la \
	$(WIBBLE_LIBS)
