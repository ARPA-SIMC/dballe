## Process this file with automake to produce Makefile.in

EXTRA_DIST =
CLEANFILES =
noinst_DATA =

AM_CPPFLAGS = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) -I$(top_builddir) $(WREPORT_CFLAGS) $(LIBPQ_CFLAGS) $(SQLITE3_CFLAGS) $(MYSQL_CFLAGS) $(LUA_CFLAGS)
if FILE_OFFSET_BITS_64
AM_CPPFLAGS += -D_FILE_OFFSET_BITS=64
endif

common_libs = $(WREPORT_LIBS) $(LIBPQ_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS) $(LUA_LIBS)

#
# Autobuilt files
#

core/record_keyword.cc: core/record_keyword.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

core/aliases.cc: core/aliases.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

BUILT_SOURCES = core/record_keyword.cc core/aliases.cc
EXTRA_DIST += core/aliases.gperf core/aliases.cc core/record_keyword.gperf core/record_keyword.cc


msg/vars.h msg/vars.gperf msg/msg-extravars.h msg/context-extravars.h: msg/vars.csv msg/mkvars
	$(srcdir)/msg/mkvars msg < $<

msg/vars.cc: msg/vars.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

BUILT_SOURCES += msg/vars.h msg/vars.cc msg/vars.gperf msg/msg-extravars.h msg/context-extravars.h
EXTRA_DIST += msg/mkvars msg/vars.csv msg/vars.h msg/vars.gperf msg/vars.cc msg/vars.cc msg/msg-extravars.h msg/context-extravars.h

noinst_PROGRAMS =
EXTRA_PROGRAMS =


#
# Shared library
#

dballeincludedir = $(includedir)/dballe

nobase_dist_dballeinclude_HEADERS = \
	file.h \
	message.h \
	types.h \
	record.h \
	query.h \
	var.h \
	core/error.h \
	core/aliases.h \
	core/verbose.h \
	core/stlutils.h \
	core/stlutils.tcc \
	core/defs.h \
	core/var.h \
	core/values.h \
	core/file.h \
	core/arrayfile.h \
	core/aoffile.h \
	core/csv.h \
	core/record.h \
	core/query.h \
	core/structbuf.h \
	core/matcher.h \
	core/match-wreport.h \
	core/varmatch.h \
	core/trace.h \
	core/json.h \
	core/ostream.h \
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
	msg/bulletin.h \
	msg/context.h \
	msg/vars.h \
	msg/msg.h \
	msg/msg-extravars.h \
	msg/context-extravars.h \
	msg/aof_codec.h \
	msg/wr_codec.h \
	msg/wr_exporters/common.h \
	sql/sql.h \
	sql/querybuf.h \
	sql/sqlite.h \
	db/defs.h \
	db/db.h \
	db/trace.h \
	db/mem/repinfo.h \
	db/mem/cursor.h \
	db/mem/db.h \
	db/sql/internals.h \
	db/sql/repinfo.h \
	db/sql/station.h \
	db/sql/levtr.h \
	db/sql/datav6.h \
	db/sql/attrv6.h \
	db/sql/driver.h \
	db/sqlite/repinfo.h \
	db/sqlite/station.h \
	db/sqlite/levtr.h \
	db/sqlite/datav6.h \
	db/sqlite/attrv6.h \
	db/sqlite/driver.h \
	db/v6/db.h \
	db/v6/cursor.h \
	db/v6/qbuilder.h \
	db/summary.h \
	db/ostream.h \
	cmdline/cmdline.h \
	cmdline/conversion.h \
	cmdline/processor.h \
	cmdline/dbadb.h \
	simple/dbapi.h \
	simple/simple.h \
	simple/commonapi.h \
	simple/msgapi.h
if HAVE_LIBPQ
nobase_dist_dballeinclude_HEADERS += \
	sql/postgresql.h \
	db/postgresql/repinfo.h \
	db/postgresql/station.h \
	db/postgresql/levtr.h \
	db/postgresql/datav6.h \
	db/postgresql/attrv6.h \
	db/postgresql/driver.h
endif
if HAVE_MYSQL
nobase_dist_dballeinclude_HEADERS += \
	sql/mysql.h \
	db/mysql/repinfo.h \
	db/mysql/station.h \
	db/mysql/levtr.h \
	db/mysql/datav6.h \
	db/mysql/attrv6.h \
	db/mysql/driver.h
endif
if HAVE_ODBC
nobase_dist_dballeinclude_HEADERS += \
	sql/odbc.h \
	db/odbc/repinfo.h \
	db/odbc/station.h \
	db/odbc/levtr.h \
	db/odbc/datav6.h \
	db/odbc/attrv6.h \
	db/odbc/driver.h
endif

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = \
	file.cc \
	message.cc \
	types.cc \
	record.cc \
	query.cc \
	var.cc \
	core/error.cc \
	core/verbose.cc \
	core/aliases.cc \
	core/defs.cc \
	core/var.cc \
	core/values.cc \
	core/file.cc \
	core/arrayfile.cc \
	core/aoffile.cc \
	core/csv.cc \
	core/record_keyword.cc \
	core/record.cc \
	core/query.cc \
	core/structbuf.cc \
	core/matcher.cc \
	core/match-wreport.cc \
	core/varmatch.cc \
	core/json.cc \
	core/ostream.cc \
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
	msg/bulletin.cc \
	msg/vars.cc \
	msg/context.cc \
	msg/msg.cc \
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
	sql/sql.cc \
	sql/querybuf.cc \
	sql/sqlite.cc \
	db/db.cc \
	db/trace.cc \
	db/sql/repinfo.cc \
	db/sql/station.cc \
	db/sql/levtr.cc \
	db/sql/datav6.cc \
	db/sql/attrv6.cc \
	db/sql/driver.cc \
	db/sqlite/repinfo.cc \
	db/sqlite/station.cc \
	db/sqlite/levtr.cc \
	db/sqlite/datav6.cc \
	db/sqlite/attrv6.cc \
	db/sqlite/driver.cc \
	db/mem/repinfo.cc \
	db/mem/cursor.cc \
	db/mem/db.cc \
	db/v6/db.cc \
	db/v6/cursor.cc \
	db/v6/qbuilder.cc \
	db/v6/import.cc \
	db/v6/export.cc \
	db/summary.cc \
	db/ostream.cc \
	cmdline/cmdline.cc \
	cmdline/processor.cc \
	cmdline/conversion.cc \
	cmdline/dbadb.cc \
	simple/simple.cc \
	simple/commonapi.cc \
	simple/msgapi.cc \
	simple/dbapi.cc
if HAVE_LIBPQ
libdballe_la_SOURCES += \
	sql/postgresql.cc \
	db/postgresql/repinfo.cc \
	db/postgresql/station.cc \
	db/postgresql/levtr.cc \
	db/postgresql/datav6.cc \
	db/postgresql/attrv6.cc \
	db/postgresql/driver.cc
endif
if HAVE_MYSQL
libdballe_la_SOURCES += \
	sql/mysql.cc \
	db/mysql/driver.cc \
	db/mysql/repinfo.cc \
	db/mysql/station.cc \
	db/mysql/levtr.cc \
	db/mysql/datav6.cc \
	db/mysql/attrv6.cc
endif
if HAVE_ODBC
libdballe_la_SOURCES += \
	sql/odbc.cc \
	db/odbc/repinfo.cc \
	db/odbc/station.cc \
	db/odbc/levtr.cc \
	db/odbc/datav6.cc \
	db/odbc/attrv6.cc \
	db/odbc/driver.cc
endif

libdballe_la_CPPFLAGS = $(AM_CPPFLAGS) -Werror
libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(common_libs)

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

check_PROGRAMS = test-dballe
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
#TESTS = $(check_PROGRAMS)

check-local:
	for test in $(check_PROGRAMS); do \
		$(TESTS_ENVIRONMENT) $$test ; \
	done

#CXXFLAGS += -O0

dist_noinst_HEADERS = \
	core/tests.h \
	memdb/tests.h \
	msg/tests.h \
	msg/tests-lua.h \
	db/tests.h

test_dballe_SOURCES = \
	tests-main.cc \
	core/tests.cc \
	file-test.cc \
	message-test.cc \
	types-test.cc \
	record-test.cc \
	query-test.cc \
	var-test.cc \
	core/aliases-test.cc \
	core/stlutils-test.cc \
	core/defs-test.cc \
	core/var-test.cc \
	core/values-test.cc \
	core/file-test.cc \
	core/record-test.cc \
	core/query-test.cc \
	core/structbuf-test.cc \
	core/csv-test.cc \
	core/matcher-test.cc \
	core/match-wreport-test.cc \
	core/varmatch-test.cc \
	core/json-test.cc \
	memdb/tests.cc \
	memdb/valuestorage-test.cc \
	memdb/index-test.cc \
	memdb/match-test.cc \
	memdb/results-test.cc \
	memdb/station-test.cc \
	memdb/valuebase-test.cc \
	memdb/stationvalue-test.cc \
	memdb/levtr-test.cc \
	memdb/value-test.cc \
	memdb/memdb-test.cc \
	memdb/serializer-test.cc \
	msg/tests.cc \
	msg/bulletin-test.cc \
	msg/vars-test.cc \
	msg/context-test.cc \
	msg/codec-test.cc \
	msg/msg-test.cc \
	msg/wr_codec_generic-test.cc \
	msg/wr_codec-test.cc \
	msg/wr_import-test.cc \
	msg/wr_export-test.cc \
	msg/aof_codec-test.cc \
	sql/querybuf-test.cc \
	sql/sqlite-test.cc \
	db/tests.cc \
	db/trace-test.cc \
	db/sql/repinfo-test.cc \
	db/sql/station-test.cc \
	db/sql/levtr-test.cc \
	db/sql/datav6-test.cc \
	db/sql/attrv6-test.cc \
	db/db-basic-test.cc \
	db/db-misc-test.cc \
	db/db-query-station-test.cc \
	db/db-query-data-test.cc \
	db/db-query-summary-test.cc \
	db/db-import-test.cc \
	db/db-export-test.cc \
	db/mem/repinfo-test.cc \
	db/mem/cursor-test.cc \
	db/summary-test.cc \
	simple/msgapi-test.cc \
	cmdline/dbadb-test.cc \
	simple/dbapi-test.cc
if LUA
test_dballe_SOURCES += \
	msg/lua-test.cc \
	msg/tests-lua.cc
endif
if HAVE_LIBPQ
test_dballe_SOURCES += \
	sql/postgresql-test.cc
endif
if HAVE_MYSQL
test_dballe_SOURCES += \
	sql/mysql-test.cc
endif
if HAVE_ODBC
test_dballe_SOURCES += \
	sql/odbc-test.cc
endif

test_dballe_LDADD = \
	libdballe.la \
	$(common_libs)

#
# Benchmark
#

noinst_PROGRAMS += bench-run

dist_noinst_HEADERS += \
	db/bench.h

bench_run_SOURCES = \
	db/db-query-parse-bench.cc \
	db/db-import-bench.cc \
	db/db-query-bench.cc \
	benchmark/bench-main.cc
bench_run_LDADD = \
	libdballe.la \
	$(common_libs)


#
# Documentation
#

noinst_PROGRAMS += dump-core-info

dump_core_info_SOURCES = core/dump-core-info.cc
dump_core_info_LDADD = \
	libdballe.la \
	$(common_libs)

# toplevel

EXTRA_DIST += msg/wr_exporters/exporters.dox \
	msg/ltypes.txt 
