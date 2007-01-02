## Process this file with automake to produce Makefile.in

dballeincludedir = $(includedir)/dballe++

dballeinclude_HEADERS = \
		error.h \
		var.h \
		record.h \
		db.h \
		init.h \
		format.h \
		bindhelp.h

lib_LTLIBRARIES = libdballepp.la
libdballepp_la_SOURCES = \
		error.cc \
		var.cc \
		db.cc \
		format.cc \
		record.cc

libdballepp_la_LDFLAGS = -lm -lodbc -version-info @LIBDBALLE_VERSION_INFO@

#INCLUDES = -DDBALLEPP_COMPILE_TESTSUITE
