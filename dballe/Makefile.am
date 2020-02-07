## Process this file with automake to produce Makefile.in

EXTRA_DIST =
CLEANFILES =
noinst_DATA =

AM_CPPFLAGS = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) -I$(top_builddir) $(WREPORT_CFLAGS) $(LIBPQ_CFLAGS) $(SQLITE3_CFLAGS) $(MYSQL_CFLAGS) $(XAPIAN_CFLAGS)
if FILE_OFFSET_BITS_64
AM_CPPFLAGS += -D_FILE_OFFSET_BITS=64
endif

common_libs = $(WREPORT_LIBS) $(LIBPQ_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS) $(POPT_LIBS) $(XAPIAN_LIBS)

#
# Autobuilt files
#

core/query-access.cc: core/query-access.in.cc mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

core/data-access.cc: core/data-access.in.cc mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

core/shortcuts.h: vars.csv mkvars
	$(top_srcdir)/dballe/mkvars -t shortcuts.h $< -o $@

core/shortcuts.cc: vars.csv mkvars
	$(top_srcdir)/dballe/mkvars -t shortcuts.cc $< -o $@

core/shortcuts-access.in.cc: vars.csv mkvars
	$(top_srcdir)/dballe/mkvars -t shortcuts-access.in.cc $< -o $@

core/shortcuts-access.cc: core/shortcuts-access.in.cc mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

msg/msg-extravars.h: vars.csv mkvars
	$(top_srcdir)/dballe/mkvars -t msg-extravars.h $< -o $@

msg/cursor-access.cc: msg/cursor-access.in.cc mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

db/v7/cursor-access.cc: db/v7/cursor-access.in.cc mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

db/summary-access.cc: db/summary-access.in.cc mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

fortran/commonapi-access.cc: fortran/commonapi-access.in.cc mklookup
	$(top_srcdir)/dballe/mklookup $< -o $@

core/aliases.cc: core/aliases.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

BUILT_SOURCES = core/aliases.cc \
				core/shortcuts.h core/shortcuts.cc core/shortcuts-access.in.cc core/shortcuts-access.cc msg/msg-extravars.h \
				core/query-access.cc core/data-access.cc \
				msg/cursor-access.cc db/v7/cursor-access.cc \
				db/summary-access.cc fortran/commonapi-access.cc
EXTRA_DIST += mklookup mkvars vars.csv \
			  core/aliases.gperf core/aliases.cc \
			  core/shortcuts.h core/shortcuts.cc core/shortcuts-access.in.cc core/shortcuts-access.cc msg/msg-extravars.h \
			  core/query-access.in.cc core/query-access.cc \
			  core/data-access.in.cc core/data-access.cc \
			  msg/cursor-access.in.cc msg/cursor-access.cc \
			  db/v7/cursor-access.in.cc db/v7/cursor-access.cc \
			  db/summary-access.in.cc db/summary-access.cc \
			  fortran/commonapi-access.in.cc fortran/commonapi-access.cc

noinst_PROGRAMS =
EXTRA_PROGRAMS =


#
# Shared library
#

dballeincludedir = $(includedir)/dballe

nobase_dist_dballeinclude_HEADERS = \
	fwd.h \
	file.h \
	importer.h \
	exporter.h \
	message.h \
	types.h \
	value.h \
	values.h \
	data.h \
	query.h \
	var.h \
	cursor.h \
	db.h \
	python.h \
	core/fwd.h \
	core/error.h \
	core/shortcuts.h \
	core/enq.h \
	core/cursor.h \
	core/aliases.h \
	core/benchmark.h \
	core/defs.h \
	core/var.h \
	core/values.h \
	core/file.h \
	core/arrayfile.h \
	core/csv.h \
	core/data.h \
	core/query.h \
	core/structbuf.h \
	core/matcher.h \
	core/match-wreport.h \
	core/smallset.h \
	core/varmatch.h \
	core/string.h \
	core/trace.h \
	core/json.h \
	msg/fwd.h \
	msg/bulletin.h \
	msg/context.h \
	msg/msg.h \
	msg/msg-extravars.h \
	msg/cursor.h \
	msg/json_codec.h \
	msg/wr_codec.h \
	msg/wr_exporters/common.h \
	sql/fwd.h \
	sql/sql.h \
	sql/querybuf.h \
	sql/sqlite.h \
	db/fwd.h \
	db/defs.h \
	db/db.h \
	db/v7/fwd.h \
	db/v7/utils.h \
	db/v7/trace.h \
	db/v7/transaction.h \
	db/v7/batch.h \
	db/v7/cache.h \
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
	db/summary_memory.h \
	db/explorer.h \
	cmdline/cmdline.h \
	cmdline/conversion.h \
	cmdline/processor.h \
	cmdline/dbadb.h \
	fortran/api.h \
	fortran/enq.h \
	fortran/dbapi.h \
	fortran/traced.h \
	fortran/commonapi.h \
	fortran/msgapi.h
if HAVE_LIBPQ
nobase_dist_dballeinclude_HEADERS += \
	sql/postgresql.h \
	db/v7/postgresql/repinfo.h \
	db/v7/postgresql/station.h \
	db/v7/postgresql/levtr.h \
	db/v7/postgresql/data.h \
	db/v7/postgresql/driver.h
endif
if HAVE_MYSQL
nobase_dist_dballeinclude_HEADERS += \
	sql/mysql.h \
	db/v7/mysql/repinfo.h \
	db/v7/mysql/station.h \
	db/v7/mysql/levtr.h \
	db/v7/mysql/data.h \
	db/v7/mysql/driver.h
endif
if HAVE_XAPIAN
nobase_dist_dballeinclude_HEADERS += \
	db/summary_xapian.h
endif

lib_LTLIBRARIES = libdballe.la
libdballe_la_SOURCES = \
	file.cc \
	importer.cc \
	exporter.cc \
	message.cc \
	types.cc \
	value.cc \
	values.cc \
	data.cc \
	query.cc \
	var.cc \
	cursor.cc \
	db.cc \
	core/error.cc \
	core/cursor.cc \
	core/aliases.cc \
	core/shortcuts.cc \
	core/shortcuts-access.cc \
	core/benchmark.cc \
	core/defs.cc \
	core/var.cc \
	core/values.cc \
	core/file.cc \
	core/arrayfile.cc \
	core/csv.cc \
	core/data.cc \
	core/data-access.cc \
	core/query.cc \
	core/query-access.cc \
	core/structbuf.cc \
	core/matcher.cc \
	core/match-wreport.cc \
	core/varmatch.cc \
	core/json.cc \
	core/string.cc \
	msg/bulletin.cc \
	msg/context.cc \
	msg/msg.cc \
	msg/cursor.cc \
	msg/cursor-access.cc \
	msg/json_codec.cc \
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
	db/v7/utils.cc \
	db/v7/trace.cc \
	db/v7/transaction.cc \
	db/v7/batch.cc \
	db/v7/cache.cc \
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
	db/v7/cursor-access.cc \
	db/v7/qbuilder.cc \
	db/v7/import.cc \
	db/v7/export.cc \
	db/summary.cc \
	db/summary_memory.cc \
	db/summary-access.cc \
	db/explorer.cc \
	cmdline/cmdline.cc \
	cmdline/processor.cc \
	cmdline/conversion.cc \
	cmdline/dbadb.cc \
	fortran/api.cc \
	fortran/traced.cc \
	fortran/commonapi.cc \
	fortran/commonapi-access.cc \
	fortran/msgapi.cc \
	fortran/dbapi.cc
if HAVE_LIBPQ
libdballe_la_SOURCES += \
	sql/postgresql.cc \
	db/v7/postgresql/repinfo.cc \
	db/v7/postgresql/station.cc \
	db/v7/postgresql/levtr.cc \
	db/v7/postgresql/data.cc \
	db/v7/postgresql/driver.cc
endif
if HAVE_MYSQL
libdballe_la_SOURCES += \
	sql/mysql.cc \
	db/v7/mysql/repinfo.cc \
	db/v7/mysql/station.cc \
	db/v7/mysql/levtr.cc \
	db/v7/mysql/data.cc \
	db/v7/mysql/driver.cc
endif
if HAVE_XAPIAN
libdballe_la_SOURCES += \
	db/summary_xapian.cc
endif

libdballe_la_CPPFLAGS = $(AM_CPPFLAGS) -Werror
libdballe_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(common_libs)

# filter.cc and sat.cc are here for now until I manage to turn them into a working system
EXTRA_DIST += \
	core/vasprintf.h \
	core/byteswap.h \
	msg/wr_importers/base.h

#
# Unit testing
#

check_PROGRAMS = test-dballe
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
#TESTS = $(check_PROGRAMS)

check-local:
	$(TESTS_ENVIRONMENT) -v $(check_PROGRAMS)

#CXXFLAGS += -O0

dist_noinst_HEADERS = \
	core/tests.h \
	msg/tests.h \
	db/tests.h

test_dballe_SOURCES = \
	tests-main.cc \
	core/tests.cc \
	core/shortcuts-test.cc \
	file-test.cc \
	importer-test.cc \
	exporter-test.cc \
	message-test.cc \
	types-test.cc \
	value-test.cc \
	values-test.cc \
	data-test.cc \
	query-test.cc \
	var-test.cc \
	cursor-test.cc \
	db-test.cc \
	core/cursor-test.cc \
	core/aliases-test.cc \
	core/defs-test.cc \
	core/var-test.cc \
	core/values-test.cc \
	core/file-test.cc \
	core/data-test.cc \
	core/query-test.cc \
	core/structbuf-test.cc \
	core/csv-test.cc \
	core/matcher-test.cc \
	core/match-wreport-test.cc \
	core/smallset-test.cc \
	core/varmatch-test.cc \
	core/json-test.cc \
	core/string-test.cc \
	msg/tests.cc \
	msg/bulletin-test.cc \
	msg/context-test.cc \
	msg/msg-test.cc \
	msg/cursor-test.cc \
	msg/json_codec-test.cc \
	msg/wr_codec_generic-test.cc \
	msg/wr_codec-test.cc \
	msg/wr_import-test.cc \
	msg/wr_export-test.cc \
	sql/querybuf-test.cc \
	sql/sqlite-test.cc \
	db/tests.cc \
	db/v7/utils-test.cc \
	db/v7/trace-test.cc \
	db/v7/batch-test.cc \
	db/v7/cache-test.cc \
	db/v7/repinfo-test.cc \
	db/v7/station-test.cc \
	db/v7/levtr-test.cc \
	db/v7/data-test.cc \
	db/db-test.cc \
	db/db-basic-test.cc \
	db/db-misc-test.cc \
	db/db-query-station-test.cc \
	db/db-query-data-test.cc \
	db/db-query-summary-test.cc \
	db/db-import-test.cc \
	db/db-export-test.cc \
	db/summary-test.cc \
	db/explorer-test.cc \
	fortran/traced-test.cc \
	fortran/commonapi-test.cc \
	fortran/msgapi-test.cc \
	fortran/dbapi-test.cc \
	cmdline/processor-test.cc \
	cmdline/dbadb-test.cc
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

# TODO: remove/move code
# noinst_PROGRAMS += bench-run
# 
# dist_noinst_HEADERS += \
# 	db/benchmark.h
# 
# bench_run_SOURCES = \
# 	db/db-query-parse-bench.cc \
# 	db/db-import-bench.cc \
# 	db/db-query-bench.cc
# bench_run_LDADD = \
# 	libdballe.la \
# 	$(common_libs)


#
# Profile
#

noinst_PROGRAMS += profile-run

profile_run_SOURCES = \
	profile-main.cc
profile_run_LDADD = \
	libdballe.la \
	$(common_libs)


# toplevel

EXTRA_DIST += msg/wr_exporters/exporters.dox \
	msg/ltypes.txt 
