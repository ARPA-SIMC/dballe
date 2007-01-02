## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

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
		aof_codec.h \
		bufrex_codec.h

lib_LTLIBRARIES = libdballe-msg.la
libdballe_msg_la_SOURCES = \
		formatter.c \
		vars.c \
		datum.c \
		level.c \
		msg.c \
		msgs.c \
		marshal.c \
		aof_codec.c \
		aof_importers/common.c \
		aof_importers/synop.c \
		aof_importers/dribu.c \
		aof_importers/flight.c \
		aof_importers/temp.c \
		aof_importers/pilot.c \
		bufrex_codec.c \
		bufrex_importers/generic.c \
		bufrex_importers/synop.c \
		bufrex_importers/metar.c \
		bufrex_importers/pilot.c \
		bufrex_importers/temp.c \
		bufrex_importers/flight.c \
		bufrex_importers/sat.c \
		bufrex_exporters/exporters.c \
		bufrex_exporters/generic.c \
		bufrex_exporters/synop.c \
		bufrex_exporters/metar.c \
		bufrex_exporters/ship.c \
		bufrex_exporters/buoy.c \
		bufrex_exporters/pilot.c \
		bufrex_exporters/temp.c \
		bufrex_exporters/flight.c \
		file.c \
libdballe_msg_la_LIBADD = \
		../core/libdballe-core.la \
		../bufrex/libdballe-bufrex.la
libdballe_msg_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

vars.h vars.c: vars.csv mkvars
	./mkvars < $<

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

tut_test_SOURCES = \
	tests/aof_codec.cc \
	tests/bufrex_codec.cc \
	tests/bufrex_codec_generic.cc \
	tests/formatter.cc \
	tests/msg.cc \
	$(top_srcdir)/extra/test-utils-core.cc \
	$(top_srcdir)/extra/test-utils-bufrex.cc \
	$(top_srcdir)/extra/tut-main.cc
tut_test_LDADD = ../core/libdballe-core.la \
		../bufrex/libdballe-bufrex.la \
		libdballe-msg.la

EXTRA_DIST = msg.dox mkvars vars.csv vars.h vars.c aof_importers/common.h bufrex_exporters/exporters.h bufrex_exporters/exporters.dox
