## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) $(LUA_CFLAGS)

#
# Shared library
#

messageincludedir = $(includedir)/dballe/msg

# Include the .cc files that contain template definitions
messageinclude_HEADERS = \
		defs.h \
		formatter.h \
		codec.h \
		codec-opts.h \
		context.h \
		vars.h \
		msg.h \
		msg-extravars.h \
		msgs.h \
		repinfo.h \
		aof_codec.h

#if DO_DBALLE_BUFREX
#messageinclude_HEADERS += \
#		bufrex_codec.h
#endif

noinst_LTLIBRARIES = libdballe-msg.la
libdballe_msg_la_SOURCES = \
		repinfo.cc \
		formatter.cc \
		vars.cc \
		context.cc \
		msg.cc \
		msgs.cc \
		codec.cc \
		aof_codec.cc \
		aof_importers/common.cc \
		aof_importers/synop.cc
##		aof_importers/dribu.cc \
##		aof_importers/flight.cc \
##		aof_importers/temp.cc \
##		aof_importers/pilot.cc

##		codec-opts.cc
##		filter.c  (todo)

#if DO_DBALLE_BUFREX
#libdballe_msg_la_SOURCES += \
#		bufrex_codec.cc \
#		bufrex_importers/generic.c \
#		bufrex_importers/synop.c \
#		bufrex_importers/metar.c \
#		bufrex_importers/pilot.c \
#		bufrex_importers/temp.c \
#		bufrex_importers/flight.c \
#		bufrex_importers/sat.c \
#		bufrex_importers/pollution.c \
#		bufrex_exporters/exporters.c \
#		bufrex_exporters/generic.c \
#		bufrex_exporters/synop.c \
#		bufrex_exporters/metar.c \
#		bufrex_exporters/ship.c \
#		bufrex_exporters/buoy.c \
#		bufrex_exporters/pilot.c \
#		bufrex_exporters/temp.c \
#		bufrex_exporters/flight.c \
#		bufrex_exporters/pollution.c
#endif
#if LUA
#libdballe_msg_la_SOURCES += msg-lua.c
#endif
libdballe_msg_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(LUA_LIBS)

vars.h vars.gperf msg-extravars.h: vars.csv mkvars
	$(srcdir)/mkvars < $<

vars.cc: vars.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = test-utils-msg.h

tut_test_SOURCES = \
	../core/test-utils-core.cc \
	../bufrex/test-utils-bufrex.cc \
	test-utils-msg.cc \
	tests/formatter.cc \
	tests/vars.cc \
	tests/context.cc \
	tests/msg.cc \
	../core/tests/tut-main.cpp
tut_test_LDADD = \
	libdballe-msg.la \
	../bufrex/libdballe-bufrex.la \
	../core/libdballe-core.la \
	$(LUA_LIBS) $(WIBBLE_LIBS)

#	tests/aof_codec.cc \
#	tests/bufrex_codec.cc \
#	tests/bufrex_codec_generic.cc

#if LUA
#tut_test_SOURCES += tests/lua.cc \
#	../core/test-utils-lua.cc
#endif


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

BUILT_SOURCES = vars.h vars.cc vars.gperf msg-extravars.h
EXTRA_DIST = mkvars vars.csv vars.h vars.gperf vars.cc msg-extravars.h aof_importers/common.h bufrex_exporters/exporters.h bufrex_exporters/exporters.dox ltypes.txt mklevels mktranges
