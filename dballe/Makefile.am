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
	transaction.h \
	var.h \
	core/error.h \
	core/aliases.h \
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
	sql/fwd.h \
	sql/sql.h \
	sql/querybuf.h \
	sql/sqlite.h \
	db/defs.h \
	db/db.h \
	db/trace.h \
	db/v6/internals.h \
	db/v6/repinfo.h \
	db/v6/station.h \
	db/v6/levtr.h \
	db/v6/datav6.h \
	db/v6/attrv6.h \
	db/v6/driver.h \
	db/v6/sqlite/repinfo.h \
	db/v6/sqlite/station.h \
	db/v6/sqlite/levtr.h \
	db/v6/sqlite/datav6.h \
	db/v6/sqlite/attrv6.h \
	db/v6/sqlite/driver.h \
	db/v6/db.h \
	db/v6/cursor.h \
	db/v6/qbuilder.h \
	db/v7/transaction.h \
	db/v7/state.h \
	db/v7/internals.h \
	db/v7/repinfo.h \
	db/v7/station.h \
	db/v7/levtr.h \
	db/v7/data.h \
	db/v7/driver.h \
	db/v7/sqlite/repinfo.h \
	db/v7/sqlite/station.h \
	db/v7/sqlite/levtr.h \
	db/v7/sqlite/data.h \
	db/v7/sqlite/driver.h \
	db/v7/db.h \
	db/v7/cursor.h \
	db/v7/qbuilder.h \
	db/summary.h \
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
	db/v6/postgresql/repinfo.h \
	db/v6/postgresql/station.h \
	db/v6/postgresql/levtr.h \
	db/v6/postgresql/datav6.h \
	db/v6/postgresql/attrv6.h \
	db/v6/postgresql/driver.h \
	db/v7/postgresql/repinfo.h \
	db/v7/postgresql/station.h \
	db/v7/postgresql/levtr.h \
	db/v7/postgresql/data.h \
	db/v7/postgresql/driver.h
endif
if HAVE_MYSQL
nobase_dist_dballeinclude_HEADERS += \
	sql/mysql.h \
	db/v6/mysql/repinfo.h \
	db/v6/mysql/station.h \
	db/v6/mysql/levtr.h \
	db/v6/mysql/datav6.h \
	db/v6/mysql/attrv6.h \
	db/v6/mysql/driver.h \
	db/v7/mysql/repinfo.h \
	db/v7/mysql/station.h \
	db/v7/mysql/levtr.h \
	db/v7/mysql/data.h \
	db/v7/mysql/driver.h
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
	db/v6/repinfo.cc \
	db/v6/station.cc \
	db/v6/levtr.cc \
	db/v6/datav6.cc \
	db/v6/attrv6.cc \
	db/v6/driver.cc \
	db/v6/sqlite/repinfo.cc \
	db/v6/sqlite/station.cc \
	db/v6/sqlite/levtr.cc \
	db/v6/sqlite/datav6.cc \
	db/v6/sqlite/attrv6.cc \
	db/v6/sqlite/driver.cc \
	db/v6/db.cc \
	db/v6/cursor.cc \
	db/v6/qbuilder.cc \
	db/v6/import.cc \
	db/v6/export.cc \
	db/v7/transaction.cc \
	db/v7/state.cc \
	db/v7/repinfo.cc \
	db/v7/station.cc \
	db/v7/levtr.cc \
	db/v7/data.cc \
	db/v7/driver.cc \
	db/v7/sqlite/repinfo.cc \
	db/v7/sqlite/station.cc \
	db/v7/sqlite/levtr.cc \
	db/v7/sqlite/data.cc \
	db/v7/sqlite/driver.cc \
	db/v7/db.cc \
	db/v7/cursor.cc \
	db/v7/qbuilder.cc \
	db/v7/import.cc \
	db/v7/export.cc \
	db/summary.cc \
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
	db/v6/postgresql/repinfo.cc \
	db/v6/postgresql/station.cc \
	db/v6/postgresql/levtr.cc \
	db/v6/postgresql/datav6.cc \
	db/v6/postgresql/attrv6.cc \
	db/v6/postgresql/driver.cc \
	db/v7/postgresql/repinfo.cc \
	db/v7/postgresql/station.cc \
	db/v7/postgresql/levtr.cc \
	db/v7/postgresql/data.cc \
	db/v7/postgresql/driver.cc
endif
if HAVE_MYSQL
libdballe_la_SOURCES += \
	sql/mysql.cc \
	db/v6/mysql/driver.cc \
	db/v6/mysql/repinfo.cc \
	db/v6/mysql/station.cc \
	db/v6/mysql/levtr.cc \
	db/v6/mysql/datav6.cc \
	db/v6/mysql/attrv6.cc \
	db/v7/mysql/repinfo.cc \
	db/v7/mysql/station.cc \
	db/v7/mysql/levtr.cc \
	db/v7/mysql/data.cc \
	db/v7/mysql/driver.cc
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
	db/v6/station-test.cc \
	db/v6/levtr-test.cc \
	db/v6/datav6-test.cc \
	db/v6/attrv6-test.cc \
	db/v7/repinfo-test.cc \
	db/v7/station-test.cc \
	db/v7/levtr-test.cc \
	db/v7/data-test.cc \
	db/db-basic-test.cc \
	db/db-misc-test.cc \
	db/db-query-station-test.cc \
	db/db-query-data-test.cc \
	db/db-query-summary-test.cc \
	db/db-import-test.cc \
	db/db-export-test.cc \
	db/summary-test.cc \
	simple/commonapi-test.cc \
	simple/msgapi-test.cc \
	simple/dbapi-test.cc \
	cmdline/processor-test.cc \
	cmdline/dbadb-test.cc
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

test_dballe_LDADD = \
	libdballe.la \
	$(common_libs)

#
# Benchmark
#

noinst_PROGRAMS += bench-run

dist_noinst_HEADERS += \
	core/benchmark.h \
	db/benchmark.h

bench_run_SOURCES = \
	core/benchmark.cc \
	db/db-query-parse-bench.cc \
	db/db-import-bench.cc \
	db/db-query-bench.cc \
	bench-main.cc
bench_run_LDADD = \
	libdballe.la \
	$(common_libs)


#
# Profile
#

noinst_PROGRAMS += profile-run

profile_run_SOURCES = \
	profile-main.cc
profile_run_LDADD = \
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
