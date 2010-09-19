## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) $(WREPORT_CFLAGS) $(LUA_CFLAGS)
EXTRA_DIST =

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
		defs.cc \
		repinfo.cc \
		formatter.cc \
		vars.cc \
		context.cc \
		msg.cc \
		msgs.cc \
		msg-lua.cc \
		codec.cc \
		aof_codec.cc \
		aof_importers/common.cc \
		aof_importers/synop.cc \
		aof_importers/dribu.cc \
		aof_importers/flight.cc \
		aof_importers/temp.cc \
		aof_importers/pilot.cc \
		wr_codec.cc \
		wr_importers/base.cc \
		wr_importers/synop.cc \
		wr_exporters/base.cc \
		wr_exporters/synop.cc

##		codec-opts.cc
##		filter.c  (todo)

EXTRA_DIST += wr_importers/base.h wr_exporters/base.h aof_importers/common.h

#if DO_DBALLE_BUFREX
#libdballe_msg_la_SOURCES += \
#		wr_importers/generic.cc \
#		wr_importers/metar.cc \
#		wr_importers/pilot.cc \
#		wr_importers/temp.cc \
#		wr_importers/flight.cc \
#		wr_importers/sat.cc \
#		wr_importers/pollution.cc \
#		bufrex_exporters/exporters.cc \
#		bufrex_exporters/generic.cc \
#		bufrex_exporters/metar.cc \
#		bufrex_exporters/ship.cc \
#		bufrex_exporters/buoy.cc \
#		bufrex_exporters/pilot.cc \
#		bufrex_exporters/temp.cc \
#		bufrex_exporters/flight.cc \
#		bufrex_exporters/pollution.cc
#endif
libdballe_msg_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(WREPORT_LIBS) $(LUA_LIBS)

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

noinst_HEADERS = test-utils-msg.h test-utils-lua.h

tut_test_SOURCES = \
	../core/test-utils-core.cc \
	test-utils-msg.cc \
	tests/formatter.cc \
	tests/vars.cc \
	tests/context.cc \
	tests/msg.cc \
	tests/aof_codec.cc \
	tests/wr_import.cc \
	tests/wr_export.cc \
	tests/wr_codec.cc \
	../core/tests/tut-main.cpp
tut_test_LDADD = \
	libdballe-msg.la \
	../core/libdballe-core.la \
	$(WREPORT_LIBS) $(LUA_LIBS) $(WIBBLE_LIBS)

#	tests/bufrex_codec_generic.cc

if LUA
tut_test_SOURCES += tests/lua.cc \
	test-utils-lua.cc
endif


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
EXTRA_DIST += mkvars vars.csv vars.h vars.gperf vars.cc msg-extravars.h bufrex_exporters/exporters.dox ltypes.txt mklevels mktranges
