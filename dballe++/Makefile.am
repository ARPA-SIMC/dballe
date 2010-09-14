## Process this file with automake to produce Makefile.in

#
# Shared library
#
INCLUDES = -I$(top_srcdir)

dballeincludedir = $(includedir)/dballe++

dballeinclude_HEADERS = \
		init.h \
		error.h \
		var.h \
		record.h \
		msg.h \
		format.h

#if DO_DBALLE_BUFREX
#dballeinclude_HEADERS += \
#		bufrex.h
#endif

if DO_DBALLE_DB
dballeinclude_HEADERS += \
		db.h
endif

lib_LTLIBRARIES = libdballepp.la
libdballepp_la_SOURCES = \
		init.cc \
		error.cc \
		var.cc \
		record.cc \
		msg.cc \
		format.cc

#if DO_DBALLE_BUFREX
#libdballepp_la_SOURCES += \
#		bufrex.cc
#endif

if DO_DBALLE_DB
libdballepp_la_SOURCES += \
		db.cc
endif
libdballepp_la_LIBADD = ../dballe/libdballe.la
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
	tests/var.cc \
	tests/record.cc \
	../dballe/core/test-utils-core.cc \
	../dballe/tut/tut-main.cc \
	tests/format.cc \
	tests/msg.cc \
	../dballe/msg/test-utils-msg.cc

#if DO_DBALLE_BUFREX
#tut_test_SOURCES += \
#	tests/bufrex.cc \
#	../dballe/bufrex/test-utils-bufrex.cc
#endif

if DO_DBALLE_DB
tut_test_SOURCES += \
	tests/db.cc \
	../dballe/db/test-utils-db.cc
endif

tut_test_LDADD = \
	libdballepp.la \
	../dballe/db/libdballe-db.la \
	../dballe/msg/libdballe-msg.la \
	../dballe/bufrex/libdballe-bufrex.la \
	../dballe/core/libdballe-core.la
