## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) $(WREPORT_CFLAGS)

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
		station.h \
		context.h \
		data.h \
		attr.h \
		cursor.h \
		db.h

noinst_LTLIBRARIES = libdballe-db.la
libdballe_db_la_SOURCES = \
		querybuf.cc \
		internals.cc \
		repinfo.cc \
		station.cc \
		context.cc \
		data.cc \
		attr.cc \
		cursor.cc \
		db.cc \
		import.cc \
		export.cc

libdballe_db_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(WREPORT_LIBS)

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
	../core/test-utils-core.cc \
	../msg/test-utils-msg.cc \
	test-utils-db.cc \
	tests/querybuf.cc \
	tests/repinfo.cc \
	tests/station.cc \
	tests/context.cc \
	tests/data.cc \
	tests/attr.cc \
	tests/db.cc \
	tests/db_import.cc \
	tests/db_export.cc \
	../core/tests/tut-main.cpp
tut_test_LDADD = \
	libdballe-db.la \
	../msg/libdballe-msg.la \
	../core/libdballe-core.la \
	$(WREPORT_LIBS) $(LUA_LIBS) $(WIBBLE_LIBS)
