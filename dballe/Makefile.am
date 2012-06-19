## Process this file with automake to produce Makefile.in

EXTRA_DIST=
CLEANFILES=

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) -I$(top_builddir) $(WREPORT_CFLAGS) $(LUA_CFLAGS)

#
# Autobuilt files
#

core/record_keyword.cc: core/record_keyword.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

core/aliases.cc: core/aliases.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

core/aliases-reverse.cc: core/aliases.gperf core/mkaliasdoc
	$(srcdir)/core/mkaliasdoc revsolver < $< > $@

BUILT_SOURCES = core/record_keyword.cc core/aliases.cc core/aliases-reverse.cc
EXTRA_DIST += core/aliases.gperf core/aliases.cc core/record_keyword.gperf core/record_keyword.cc


msg/vars.h msg/vars.gperf msg/msg-extravars.h: msg/vars.csv msg/mkvars
	$(srcdir)/msg/mkvars msg < $<

msg/vars.cc: msg/vars.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

BUILT_SOURCES += msg/vars.h msg/vars.cc msg/vars.gperf msg/msg-extravars.h
EXTRA_DIST += msg/mkvars msg/vars.csv msg/vars.h msg/vars.gperf msg/vars.cc msg/vars.cc msg/msg-extravars.h


#
# Shared library
#

dballeincludedir = $(includedir)/dballe

nobase_dist_dballeinclude_HEADERS = \
	core/aliases.h \
	core/verbose.h \
	core/defs.h \
	core/var.h \
	core/rawmsg.h \
	core/file.h \
	core/arrayfile.h \
	core/aoffile.h \
	core/csv.h \
	core/record.h \
	core/matcher.h \
	core/match-wreport.h \
	msg/codec.h \
	msg/context.h \
	msg/vars.h \
	msg/msg.h \
	msg/msg-extravars.h \
	msg/msgs.h \
	msg/repinfo.h \
	msg/aof_codec.h \
	msg/wr_codec.h \
	cmdline/cmdline.h \
	cmdline/conversion.h \
	cmdline/processor.h
if DO_DBALLE_DB
nobase_dist_dballeinclude_HEADERS += \
	db/odbcworkarounds.h \
	db/querybuf.h \
	db/internals.h \
	db/repinfo.h \
	db/station.h \
	db/context.h \
	db/data.h \
	db/attr.h \
	db/cursor.h \
	db/db.h \
	cmdline/dbadb.h
endif

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = \
	core/verbose.cc \
	core/aliases.cc \
	core/aliases-reverse.cc \
	core/defs.cc \
	core/var.cc \
	core/file.cc \
	core/arrayfile.cc \
	core/aoffile.cc \
	core/csv.cc \
	core/record_keyword.cc \
	core/record.cc \
	core/matcher.cc \
	core/match-wreport.cc \
	msg/repinfo.cc \
	msg/vars.cc \
	msg/context.cc \
	msg/msg.cc \
	msg/msgs.cc \
	msg/msg-lua.cc \
	msg/codec.cc \
	msg/aof_codec.cc \
	msg/aof_importers/common.cc \
	msg/aof_importers/synop.cc \
	msg/aof_importers/dribu.cc \
	msg/aof_importers/flight.cc \
	msg/aof_importers/temp.cc \
	msg/aof_importers/pilot.cc \
	msg/wr_codec.cc \
	msg/wr_importers/base.cc \
	msg/wr_importers/synop.cc \
	msg/wr_importers/temp.cc \
	msg/wr_importers/metar.cc \
	msg/wr_importers/flight.cc \
	msg/wr_importers/generic.cc \
	msg/wr_importers/pollution.cc \
	msg/wr_exporters/synop.cc \
	msg/wr_exporters/ship.cc \
	msg/wr_exporters/buoy.cc \
	msg/wr_exporters/metar.cc \
	msg/wr_exporters/temp.cc \
	msg/wr_exporters/flight.cc \
	msg/wr_exporters/generic.cc \
	msg/wr_exporters/pollution.cc \
	cmdline/cmdline.cc \
	cmdline/processor.cc \
	cmdline/conversion.cc
if DO_DBALLE_DB
libdballe_la_SOURCES += \
	db/querybuf.cc \
	db/internals.cc \
	db/repinfo.cc \
	db/station.cc \
	db/context.cc \
	db/data.cc \
	db/attr.cc \
	db/cursor.cc \
	db/db.cc \
	db/import.cc \
	db/export.cc \
	cmdline/dbadb.cc
endif

libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(WREPORT_LIBS) $(LUA_LIBS)
#libdballe_la_LDFLAGS = -lstdc++ -version-info @LIBDBALLE_VERSION_INFO@

# filter.cc and sat.cc are here for now until I manage to turn them into a working system
EXTRA_DIST += \
	core/vasprintf.h \
	core/byteswap.h \
	msg/wr_importers/base.h \
	msg/aof_importers/common.h \
	msg/filter.cc \
	msg/wr_importers/sat.cc

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

dist_noinst_HEADERS = \
	core/test-utils-core.h \
	msg/test-utils-msg.h \
	msg/test-utils-lua.h \
	db/test-utils-db.h

tut_test_SOURCES = \
	core/tests/tut-main.cpp \
	core/tests/aliases.cc \
	core/tests/defs.cc \
	core/tests/rawmsg.cc \
	core/tests/file.cc \
	core/tests/record.cc \
	core/csv-tut.cc \
	core/matcher-tut.cc \
	core/match-wreport-tut.cc \
	core/test-utils-core.cc \
	msg/test-utils-msg.cc \
	msg/tests/vars.cc \
	msg/tests/context.cc \
	msg/msg-tut.cc \
	msg/msgs-tut.cc \
	msg/tests/aof_codec.cc \
	msg/tests/wr_import.cc \
	msg/tests/wr_export.cc \
	msg/tests/wr_codec.cc \
	msg/tests/wr_codec_generic.cc
if LUA
tut_test_SOURCES += \
	msg/tests/lua.cc \
	msg/test-utils-lua.cc
endif
if DO_DBALLE_DB
tut_test_SOURCES += \
	db/test-utils-db.cc \
	db/tests/querybuf.cc \
	db/tests/internals.cc \
	db/tests/repinfo.cc \
	db/tests/station.cc \
	db/tests/context.cc \
	db/tests/data.cc \
	db/tests/attr.cc \
	db/tests/db.cc \
	db/tests/db_import.cc \
	db/tests/db_export.cc \
	cmdline/dbadb-tut.cc
endif

tut_test_LDADD = \
	libdballe.la \
	$(WREPORT_LIBS) \
	$(LUA_LIBS) \
	$(WIBBLE_LIBS)


#
# Documentation
#

noinst_PROGRAMS = dump-core-info

dump_core_info_SOURCES = core/dump-core-info.cc
dump_core_info_LDADD = \
	libdballe.la \
	$(WREPORT_LIBS) \
	$(LUA_LIBS) \
	$(WIBBLE_LIBS)

# core

noinst_DATA = core/record_keyword.dox core/record_keyword.rst core/record_keyword.tex core/aliases.dox core/aliases.tex

core/record_keyword.rst: core/record_keyword.gperf core/mkkeydoc dump-core-info
	./dump-core-info keywords | $(srcdir)/core/mkkeydoc rst > $@

core/record_keyword.dox: core/record_keyword.gperf core/mkkeydoc dump-core-info
	./dump-core-info keywords | $(srcdir)/core/mkkeydoc dox > $@

core/record_keyword.tex: core/record_keyword.gperf core/mkkeydoc dump-core-info
	./dump-core-info keywords | $(srcdir)/core/mkkeydoc tex > $@

core/aliases.dox: core/aliases.gperf core/mkaliasdoc
	$(srcdir)/core/mkaliasdoc dox < $< > $@

core/aliases.tex: core/aliases.gperf core/mkaliasdoc
	$(srcdir)/core/mkaliasdoc tex < $< > $@

EXTRA_DIST += core/mkaliasdoc core/mkkeydoc

CLEANFILES += \
	core/aliases.dox \
	core/aliases.tex \
	core/record_keyword.dox \
	core/record_keyword.dox \
	core/record_keyword.rst \
	core/record_keyword.tex


# msg

noinst_DATA += msg/ltypes.dox msg/ltypes.tex msg/tranges.dox msg/tranges.tex

msg/ltypes.dox: msg/ltypes.txt msg/mklevels
	$(srcdir)/msg/mklevels dox < $< > $@

msg/ltypes.tex: msg/ltypes.txt msg/mklevels
	$(srcdir)/msg/mklevels tex < $< > $@

msg/tranges.dox: msg/mktranges
	$(srcdir)/msg/mktranges dox < $< > $@

msg/tranges.tex: msg/mktranges
	$(srcdir)/msg/mktranges tex < $< > $@

EXTRA_DIST += msg/wr_exporters/exporters.dox msg/ltypes.txt msg/mklevels msg/mktranges

CLEANFILES += msg/ltypes.dox msg/ltypes.tex msg/tranges.dox msg/tranges.tex


# toplevel

EXTRA_DIST += mainpage.dox dballe.dox \
	main-core.dox \
	main-db.dox \
	main-msg.dox


