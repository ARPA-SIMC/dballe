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
	rawfile.h \
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
	rawfile.c \
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
#TESTS_ENVIRONMENT = TOP_SRCDIR=$(top_srcdir) $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

tut_test_SOURCES = \
	tests/dba_core_error.cc \
	tests/dba_core.cc \
	tests/dba_core_aliases.cc \
	tests/dba_core_btable.cc \
	tests/dba_core_var.cc \
	tests/dba_core_record.cc \
	$(top_srcdir)/extra/test-utils-core.cc \
	$(top_srcdir)/extra/tut-main.cc
tut_test_LDADD = $(DBALLELIBS) libdballe-core.la


#
# Documentation bits
#

noinst_DATA = record_keyword.dox record_keyword.rst record_keyword.tex aliases.dox aliases.tex

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

distclean-local:
	rm -f record_keyword.c record_keyword.dox record_keyword.tex aliases.c aliases.dox aliases.tex uniconv.c

EXTRA_DIST = core.dox record_keyword.gperf mkkeydoc aliases.gperf mkaliasdoc uniconv.gperf
