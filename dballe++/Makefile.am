## Process this file with automake to produce Makefile.in

#
# Shared library
#

dballeincludedir = $(includedir)/dballe++

dballeinclude_HEADERS = \
		error.h \
		var.h \
		record.h \
		bufrex.h \
		msg.h \
		db.h \
		format.h

lib_LTLIBRARIES = libdballepp.la
libdballepp_la_SOURCES = \
		error.cc \
		var.cc \
		bufrex.cc \
		msg.cc \
		db.cc \
		format.cc \
		record.cc
libdballepp_la_LIBADD = ../dballe/db/libdballe-db.la
libdballepp_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@


#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

#noinst_HEADERS = test-utils-bufrex.h

tut_test_SOURCES = \
	tests/error.cc \
	tests/format.cc \
	tests/record.cc \
	tests/bufrex.cc \
	tests/msg.cc \
	tests/var.cc \
	../dballe/core/test-utils-core.cc \
	../dballe/bufrex/test-utils-bufrex.cc \
	../dballe/msg/test-utils-msg.cc \
	../dballe/db/test-utils-db.cc \
	../dballe/tut/tut-main.cc
if DB_TESTS
tut_test_SOURCES += tests/db.cc
endif
tut_test_LDADD = \
	../dballe/core/libdballe-core.la \
	../dballe/bufrex/libdballe-bufrex.la \
	../dballe/msg/libdballe-msg.la \
	../dballe/db/libdballe-db.la \
	libdballepp.la
