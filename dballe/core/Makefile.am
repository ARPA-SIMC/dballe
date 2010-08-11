## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) $(LUA_CFLAGS)

#
# Shared library
#

coreincludedir = $(includedir)/dballe/core

coreinclude_HEADERS = \
	error.h \
	conv.h \
	verbose.h \
	fast.h \
	rawmsg.h \
	file.h \
	file_internals.h \
	csv.h \
	aliases.h \
	record.h \
	vartable.h \
	var.h

noinst_LTLIBRARIES = libdballe-core.la
libdballe_core_la_SOURCES = \
	error.c \
	uniconv.c \
	conv.c \
	verbose.c \
	rawmsg.c \
	file.c \
	csv.c \
	aliases.c \
	record_keyword.c \
	record.c \
	vartable.c \
	var.c

if LUA
libdballe_core_la_SOURCES += var-lua.c
endif

libdballe_core_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(LUA_LIBS)


uniconv.c: uniconv.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

record_keyword.c: record_keyword.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

aliases.c: aliases.gperf
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
	tests/aliases.cc \
	tests/conv.cc \
	tests/error.cc \
	tests/fast.cc \
	tests/file.cc \
	tests/rawmsg.cc \
	tests/record.cc \
	tests/var.cc \
	tests/vartable.cc \
	test-utils-core.cc \
	../tut/tut-main.cc
tut_test_LDADD = \
	libdballe-core.la \
	$(LUA_LIBS)

if LUA
noinst_HEADERS += test-utils-lua.h
tut_test_SOURCES += \
	tests/lua.cc \
	test-utils-lua.cc
endif


#
# Documentation bits
#

noinst_DATA = record_keyword.dox record_keyword.rst record_keyword.tex aliases.dox aliases.tex btable.dox

record_keyword.rst: record_keyword.gperf mkkeydoc
	cpp -P -C -I$(top_srcdir) $< | $(srcdir)/mkkeydoc rst > $@

record_keyword.dox: record_keyword.gperf mkkeydoc
	cpp -P -C -I$(top_srcdir) $< | $(srcdir)/mkkeydoc dox > $@

record_keyword.tex: record_keyword.gperf mkkeydoc
	cpp -P -C -I$(top_srcdir) $< | $(srcdir)/mkkeydoc tex > $@

aliases.dox: aliases.gperf mkaliasdoc
	$(srcdir)/mkaliasdoc dox < $< > $@

aliases.tex: aliases.gperf mkaliasdoc
	$(srcdir)/mkaliasdoc tex < $< > $@

btable.dox: $(top_srcdir)/tables/dballe.txt $(top_srcdir)/tables/mkbdoc
	$(top_srcdir)/tables/mkbdoc dox < $< > $@ 

distclean-local:
	rm -f record_keyword.c record_keyword.dox record_keyword.tex aliases.c aliases.dox aliases.tex uniconv.c btable.dox

EXTRA_DIST = record_keyword.gperf mkkeydoc aliases.gperf mkaliasdoc uniconv.gperf
