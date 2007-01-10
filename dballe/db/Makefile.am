## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

#
# Shared library
#

dbincludedir = $(includedir)/dballe/db

# Include the .cc files that contain template definitions
dbinclude_HEADERS = \
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

lib_LTLIBRARIES = libdballe-db.la
libdballe_db_la_SOURCES = \
		querybuf.c \
		internals.c \
		repinfo.c \
		pseudoana.c \
		context.c \
		data.c \
		attr.c \
		cursor.c \
		db.c \
		import.c \
		export.c
libdballe_db_la_LIBADD = \
		../core/libdballe-core.la \
		../bufrex/libdballe-bufrex.la \
		../msg/libdballe-msg.la
libdballe_db_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@

#
# Unit testing
#

TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest

check_PROGRAMS =
TESTS =

if DB_TESTS
check_PROGRAMS += tut_test
TESTS += $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = test-utils-db.h

tut_test_SOURCES = \
	tests/repinfo.cc \
	tests/pseudoana.cc \
	tests/context.cc \
	tests/dba_db_dballe.cc \
	tests/dba_db_import.cc \
	../core/test-utils-core.cc \
	../bufrex/test-utils-bufrex.cc \
	../msg/test-utils-msg.cc \
	test-utils-db.cc \
	../tut/tut-main.cc
tut_test_LDADD = \
	../core/libdballe-core.la \
	../bufrex/libdballe-bufrex.la \
	../msg/libdballe-msg.la \
	libdballe-db.la
endif
