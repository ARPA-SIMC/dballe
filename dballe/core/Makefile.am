## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) $(WREPORT_CFLAGS) $(LUA_CFLAGS)

#
# Shared library
#

coreincludedir = $(includedir)/dballe/core

coreinclude_HEADERS = \
	verbose.h \
	var.h \
	rawmsg.h \
	file.h \
	csv.h \
	record.h

noinst_LTLIBRARIES = libdballe-core.la
libdballe_core_la_SOURCES = \
	verbose.cc \
	var.cc \
	rawmsg.cc \
	file.cc \
	csv.cc \
	record_keyword.cc \
	record.cc

libdballe_core_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(WREPORT_LIBS) $(LUA_LIBS)


record_keyword.cc: record_keyword.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = test-utils-core.h

tut_test_SOURCES = \
	tests/rawmsg.cc \
	tests/file.cc \
	tests/record.cc \
	test-utils-core.cc \
	tests/tut-main.cpp
tut_test_LDADD = \
	libdballe-core.la \
	$(WREPORT_LIBS) \
	$(LUA_LIBS) \
	$(WIBBLE_LIBS)

#
# Documentation bits
#

noinst_DATA = record_keyword.dox record_keyword.rst record_keyword.tex

record_keyword.rst: record_keyword.gperf mkkeydoc
	cpp -P -C -I$(top_srcdir) $< | $(srcdir)/mkkeydoc rst > $@

record_keyword.dox: record_keyword.gperf mkkeydoc
	cpp -P -C -I$(top_srcdir) $< | $(srcdir)/mkkeydoc dox > $@

record_keyword.tex: record_keyword.gperf mkkeydoc
	cpp -P -C -I$(top_srcdir) $< | $(srcdir)/mkkeydoc tex > $@

distclean-local:
	rm -f record_keyword.dox record_keyword.tex

BUILT_SOURCES = record_keyword.cc
EXTRA_DIST = record_keyword.gperf mkkeydoc
