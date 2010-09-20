## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir)

#
# Shared library
#

dbincludedir = $(includedir)/dballe/db

# Include the .cc files that contain template definitions
dbinclude_HEADERS = \
		odbcworkarounds.h \
		querybuf.h \
		internals.h \
		repinfo.h \
		pseudoana.h \
		context.h \
		data.h \
		attr.h \
		cursor.h \
		db.h \
		import.h \
		export.h

noinst_LTLIBRARIES = libdballe-db.la
libdballe_db_la_SOURCES = \
		querybuf.cc \
		internals.cc \
		repinfo.cc \
		db.cc

#		pseudoana.cc
#		context.cc
#		data.cc
#		attr.cc
#		cursor.cc

#		import.cc \
#		export.cc

libdballe_db_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

#
# Unit testing
#

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

check_PROGRAMS =
TESTS =

check_PROGRAMS += tut_test
TESTS += $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = test-utils-db.h

tut_test_SOURCES = \
	tests/repinfo.cc \
	tests/pseudoana.cc \
	tests/context.cc \
	tests/data.cc \
	tests/attr.cc \
	tests/dba_db_dballe.cc \
	tests/db_import.cc \
	tests/db_export.cc \
	../core/test-utils-core.cc \
	../bufrex/test-utils-bufrex.cc \
	../msg/test-utils-msg.cc \
	test-utils-db.cc \
	../tut/tut-main.cc
tut_test_LDADD = \
	libdballe-db.la \
	../msg/libdballe-msg.la \
	../bufrex/libdballe-bufrex.la \
	../core/libdballe-core.la
