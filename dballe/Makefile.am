## Process this file with automake to produce Makefile.in

EXTRA_DIST=
CLEANFILES=

AM_CPPFLAGS = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) -I$(top_builddir) $(WREPORT_CFLAGS) $(SQLITE3_CFLAGS) $(LUA_CFLAGS)

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


msg/vars.h msg/vars.gperf msg/msg-extravars.h msg/context-extravars.h: msg/vars.csv msg/mkvars
	$(srcdir)/msg/mkvars msg < $<

msg/vars.cc: msg/vars.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

BUILT_SOURCES += msg/vars.h msg/vars.cc msg/vars.gperf msg/msg-extravars.h msg/context-extravars.h
EXTRA_DIST += msg/mkvars msg/vars.csv msg/vars.h msg/vars.gperf msg/vars.cc msg/vars.cc msg/msg-extravars.h msg/context-extravars.h


#
# Shared library
#

dballeincludedir = $(includedir)/dballe

nobase_dist_dballeinclude_HEADERS = \
	core/aliases.h \
	core/verbose.h \
	core/stlutils.h \
	core/stlutils.tcc \
	core/defs.h \
	core/var.h \
	core/rawmsg.h \
	core/file.h \
	core/arrayfile.h \
	core/aoffile.h \
	core/csv.h \
	core/record.h \
	core/structbuf.h \
	core/matcher.h \
	core/match-wreport.h \
	core/varmatch.h \
	core/trace.h \
	memdb/valuestorage.h \
	memdb/valuestorage.tcc \
	memdb/index.h \
	memdb/match.h \
	memdb/match.tcc \
	memdb/results.h \
	memdb/results.tcc \
	memdb/station.h \
	memdb/valuebase.h \
	memdb/stationvalue.h \
	memdb/levtr.h \
	memdb/value.h \
	memdb/memdb.h \
	memdb/serializer.h \
	msg/codec.h \
	msg/context.h \
	msg/vars.h \
	msg/msg.h \
	msg/msg-extravars.h \
	msg/context-extravars.h \
	msg/msgs.h \
	msg/repinfo.h \
	msg/aof_codec.h \
	msg/wr_codec.h \
	msg/wr_exporters/common.h \
	db/modifiers.h \
	db/db.h \
	db/mem/repinfo.h \
	db/mem/cursor.h \
	db/mem/db.h \
	db/querybuf.h \
	db/sql.h \
	db/sqlite/internals.h \
	db/sqlite/repinfo.h \
	db/sqlite/station.h \
	db/sqlite/v6_levtr.h \
	db/sqlite/v6_data.h \
	db/sqlite/v6_attr.h \
	db/sqlite/v6_run_query.h \
	db/v6/internals.h \
	db/v6/db.h \
	db/v6/cursor.h \
	db/v6/qbuilder.h \
	db/summary.h \
	cmdline/cmdline.h \
	cmdline/conversion.h \
	cmdline/processor.h \
	cmdline/dbadb.h \
	simple/dbapi.h \
	simple/simple.h \
	simple/commonapi.h \
	simple/msgapi.h
if HAVE_ODBC
nobase_dist_dballeinclude_HEADERS += \
	db/odbc/internals.h \
	db/odbc/repinfo.h \
	db/odbc/station.h \
	db/odbc/v6_data.h \
	db/odbc/v6_attr.h \
	db/odbc/v6_levtr.h \
	db/odbc/v6_run_query.h \
	db/v5/attr.h \
	db/v5/context.h \
	db/v5/cursor.h \
	db/v5/data.h \
	db/v5/db.h \
	db/v5/repinfo.h \
	db/v5/station.h
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
	core/structbuf.cc \
	core/matcher.cc \
	core/match-wreport.cc \
	core/varmatch.cc \
	memdb/station.cc \
	memdb/valuebase.cc \
	memdb/stationvalue.cc \
	memdb/levtr.cc \
	memdb/valuestorage.cc \
	memdb/index.cc \
	memdb/match.cc \
	memdb/results.cc \
	memdb/value.cc \
	memdb/memdb.cc \
	memdb/serializer.cc \
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
	msg/wr_importers/ship.cc \
	msg/wr_importers/temp.cc \
	msg/wr_importers/metar.cc \
	msg/wr_importers/flight.cc \
	msg/wr_importers/generic.cc \
	msg/wr_importers/pollution.cc \
	msg/wr_exporters/common.cc \
	msg/wr_exporters/synop.cc \
	msg/wr_exporters/ship.cc \
	msg/wr_exporters/buoy.cc \
	msg/wr_exporters/metar.cc \
	msg/wr_exporters/temp.cc \
	msg/wr_exporters/flight.cc \
	msg/wr_exporters/generic.cc \
	msg/wr_exporters/pollution.cc \
	db/modifiers.cc \
	db/db.cc \
	db/querybuf.cc \
	db/sql.cc \
	db/sqlite/internals.cc \
	db/sqlite/repinfo.cc \
	db/sqlite/station.cc \
	db/sqlite/v6_levtr.cc \
	db/sqlite/v6_data.cc \
	db/sqlite/v6_attr.cc \
	db/sqlite/v6_run_query.cc \
	db/mem/repinfo.cc \
	db/mem/cursor.cc \
	db/mem/db.cc \
	db/v6/internals.cc \
	db/v6/db.cc \
	db/v6/cursor.cc \
	db/v6/qbuilder.cc \
	db/v6/import.cc \
	db/v6/export.cc \
	db/summary.cc \
	cmdline/cmdline.cc \
	cmdline/processor.cc \
	cmdline/conversion.cc \
	cmdline/dbadb.cc \
	simple/simple.cc \
	simple/commonapi.cc \
	simple/msgapi.cc \
	simple/dbapi.cc

if HAVE_ODBC
libdballe_la_SOURCES += \
	db/odbc/internals.cc \
	db/odbc/repinfo.cc \
	db/odbc/station.cc \
	db/odbc/v6_data.cc \
	db/odbc/v6_attr.cc \
	db/odbc/v6_levtr.cc \
	db/odbc/v6_run_query.cc \
	db/v5/repinfo.cc \
	db/v5/station.cc \
	db/v5/context.cc \
	db/v5/data.cc \
	db/v5/attr.cc \
	db/v5/cursor.cc \
	db/v5/db.cc \
	db/v5/import.cc \
	db/v5/export.cc
endif

libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(WREPORT_LIBS) $(SQLITE3_LIBS) $(LUA_LIBS)
#libdballe_la_LDFLAGS = -lstdc++ -version-info @LIBDBALLE_VERSION_INFO@

# filter.cc and sat.cc are here for now until I manage to turn them into a working system
EXTRA_DIST += \
	core/vasprintf.h \
	core/byteswap.h \
	msg/wr_importers/base.h \
	msg/aof_importers/common.h \
	msg/filter.cc \
	msg/wr_importers/sat.cc \
	msg/tests/wr_export.h

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
#TESTS = $(check_PROGRAMS)

check-local:
	for test in $(check_PROGRAMS); do \
		$(TESTS_ENVIRONMENT) $$test ; \
	done

#CXXFLAGS += -O0

dist_noinst_HEADERS = \
	core/test-utils-core.h \
	memdb/tests.h \
	msg/test-utils-msg.h \
	msg/test-utils-lua.h \
	db/test-utils-db.h

tut_test_SOURCES = \
	core/tests/tut-main.cpp \
	core/aliases-tut.cc \
	core/stlutils-tut.cc \
	core/defs-tut.cc \
	core/var-tut.cc \
	core/rawmsg-tut.cc \
	core/file-tut.cc \
	core/record-tut.cc \
	core/structbuf-tut.cc \
	core/csv-tut.cc \
	core/matcher-tut.cc \
	core/match-wreport-tut.cc \
	core/varmatch-tut.cc \
	core/test-utils-core.cc \
	memdb/tests.cc \
	memdb/valuestorage-tut.cc \
	memdb/index-tut.cc \
	memdb/match-tut.cc \
	memdb/results-tut.cc \
	memdb/station-tut.cc \
	memdb/valuebase-tut.cc \
	memdb/stationvalue-tut.cc \
	memdb/levtr-tut.cc \
	memdb/value-tut.cc \
	memdb/memdb-tut.cc \
	memdb/serializer-tut.cc \
	msg/test-utils-msg.cc \
	msg/tests/vars.cc \
	msg/tests/context.cc \
	msg/codec-tut.cc \
	msg/msg-tut.cc \
	msg/msgs-tut.cc \
	msg/tests/aof_codec.cc \
	msg/tests/wr_import.cc \
	msg/tests/wr_export.cc \
	msg/tests/wr_export2.cc \
	msg/tests/wr_codec.cc \
	msg/tests/wr_codec_generic.cc \
	db/test-utils-db.cc \
	db/modifiers-tut.cc \
	db/querybuf-tut.cc \
	db/sqlite/internals-tut.cc \
	db/db-basic-tut.cc \
	db/db-misc-tut.cc \
	db/db-station-query-tut.cc \
	db/db-data-query-tut.cc \
	db/db-summary-query-tut.cc \
	db/db-import-tut.cc \
	db/db-export-tut.cc \
	db/mem/repinfo-tut.cc \
	db/mem/cursor-tut.cc \
	db/mem/db-tut.cc \
	db/v6/tests/lev_tr.cc \
	db/v6/tests/data.cc \
	db/v6/tests/attr.cc \
	db/v6/tests/db.cc \
	db/summary-tut.cc \
	simple/msgapi-tut.cc \
	cmdline/dbadb-tut.cc \
	simple/dbapi-tut.cc
if LUA
tut_test_SOURCES += \
	msg/tests/lua.cc \
	msg/test-utils-lua.cc
endif
if HAVE_ODBC
tut_test_SOURCES += \
	db/odbc/internals-tut.cc \
	db/v5/tests/repinfo.cc \
	db/v5/tests/station.cc \
	db/v5/tests/context.cc \
	db/v5/tests/data.cc \
	db/v5/tests/attr.cc \
	db/v5/tests/db.cc
endif

tut_test_LDADD = \
	libdballe.la \
	$(WREPORT_LIBS) \
	$(WREPORT_TEST_LIBS) \
	$(SQLITE3_LIBS) \
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
	$(SQLITE3_LIBS) \
	$(LUA_LIBS)

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


