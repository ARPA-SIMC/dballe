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
		msg.h

noinst_LTLIBRARIES = libdballe-bufrex.la
libdballe_bufrex_la_SOURCES = \
		template-aliases.c \
		dtable.c \
		opcode.c \
		subset.c \
		msg.c \
		bufr_decoder.c \
		bufr_encoder.c \
		crex_decoder.c \
		crex_encoder.c \
		bufrex_codec.c
libdballe_bufrex_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

template-aliases.c: template-aliases.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

template-aliases.tex: template-aliases.gperf mktaliases
	if ! $(srcdir)/mktaliases < $< ; then rm $@; /bin/false; fi
	

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = test-utils-bufrex.h

tut_test_SOURCES = \
	tests/bufr_decoder.cc \
	tests/bufr_encoder.cc \
	tests/crex_decoder.cc \
	tests/dtable.cc \
	tests/opcode.cc \
	tests/msg.cc \
	../core/test-utils-core.cc \
	test-utils-bufrex.cc \
	../tut/tut-main.cc
tut_test_LDADD = \
	../core/libdballe-core.la \
	libdballe-bufrex.la

distclean-local:
	rm -f template-aliases.c template-aliases.tex

EXTRA_DIST=template-aliases.gperf mktaliases
