## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir)

#
# Shared library
#

messageincludedir = $(includedir)/dballe/msg

# Include the .cc files that contain template definitions
messageinclude_HEADERS = \
		formatter.h \
		datum.h \
		level.h \
		vars.h \
		msg.h \
		msgs.h \
		marshal.h \
		file.h \
		filter.h \
		repinfo.h \
		aof_codec.h

if DO_DBALLE_BUFREX
messageinclude_HEADERS += \
		bufrex_codec.h
endif

noinst_LTLIBRARIES = libdballe-msg.la
libdballe_msg_la_SOURCES = \
		formatter.c \
		vars.c \
		datum.c \
		level.c \
		msg.c \
		msgs.c \
		marshal.c \
		filter.c \
		repinfo.c \
		aof_codec.c \
		aof_importers/common.c \
		aof_importers/synop.c \
		aof_importers/dribu.c \
		aof_importers/flight.c \
		aof_importers/temp.c \
		aof_importers/pilot.c \
		file.c
if DO_DBALLE_BUFREX
libdballe_msg_la_SOURCES += \
		bufrex_codec.c \
		bufrex_importers/generic.c \
		bufrex_importers/synop.c \
		bufrex_importers/metar.c \
		bufrex_importers/pilot.c \
		bufrex_importers/temp.c \
		bufrex_importers/flight.c \
		bufrex_importers/sat.c \
		bufrex_importers/pollution.c \
		bufrex_exporters/exporters.c \
		bufrex_exporters/generic.c \
		bufrex_exporters/synop.c \
		bufrex_exporters/metar.c \
		bufrex_exporters/ship.c \
		bufrex_exporters/buoy.c \
		bufrex_exporters/pilot.c \
		bufrex_exporters/temp.c \
		bufrex_exporters/flight.c \
		bufrex_exporters/pollution.c
endif
libdballe_msg_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

vars.h vars.c: vars.csv mkvars
	$(srcdir)/mkvars < $<

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = test-utils-msg.h

tut_test_SOURCES = \
	tests/aof_codec.cc \
	tests/bufrex_codec.cc \
	tests/bufrex_codec_generic.cc \
	tests/formatter.cc \
	tests/msg.cc \
	../core/test-utils-core.cc \
	../bufrex/test-utils-bufrex.cc \
	test-utils-msg.cc \
	../tut/tut-main.cc
tut_test_LDADD = \
	libdballe-msg.la \
	../bufrex/libdballe-bufrex.la \
	../core/libdballe-core.la

#
# Documentation bits
#

noinst_DATA = ltypes.dox ltypes.tex tranges.dox tranges.tex

ltypes.dox: ltypes.txt mklevels
	$(srcdir)/mklevels dox < $< > $@

ltypes.tex: ltypes.txt mklevels
	$(srcdir)/mklevels tex < $< > $@

tranges.dox: mktranges
	$(srcdir)/mktranges dox < $< > $@

tranges.tex: mktranges
	$(srcdir)/mktranges tex < $< > $@

distclean-local:
	rm -f ltypes.dox ltypes.tex tranges.dox tranges.tex

EXTRA_DIST = mkvars vars.csv vars.h vars.c aof_importers/common.h bufrex_exporters/exporters.h bufrex_exporters/exporters.dox ltypes.txt mklevels mktranges
