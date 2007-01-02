## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

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

lib_LTLIBRARIES = libdballe-core.la
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
libdballe_core_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@


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
	libdballe-core.la


#
# Documentation bits
#

noinst_DATA = record_keyword.dox record_keyword.rst record_keyword.tex aliases.dox aliases.tex btable.dox

record_keyword.rst: record_keyword.gperf mkkeydoc
	cpp -P -C -I../.. $< | ./mkkeydoc rst > $@

record_keyword.dox: record_keyword.gperf mkkeydoc
	cpp -P -C -I../.. $< | ./mkkeydoc dox > $@

record_keyword.tex: record_keyword.gperf mkkeydoc
	cpp -P -C -I../.. $< | ./mkkeydoc tex > $@

aliases.dox: aliases.gperf mkaliasdoc
	./mkaliasdoc dox < $< > $@

aliases.tex: aliases.gperf mkaliasdoc
	./mkaliasdoc tex < $< > $@

btable.dox: ../../tables/dballe.txt ../../tables/mkbdoc
	../../tables/mkbdoc dox < $< > $@ 

distclean-local:
	rm -f record_keyword.c record_keyword.dox record_keyword.tex aliases.c aliases.dox aliases.tex uniconv.c btable.dox

EXTRA_DIST = record_keyword.gperf mkkeydoc aliases.gperf mkaliasdoc uniconv.gperf
