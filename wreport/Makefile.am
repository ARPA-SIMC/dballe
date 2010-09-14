## Process this file with automake to produce Makefile.in

INCLUDES = -DTABLE_DIR=\"$(tabledir)\" -I$(top_srcdir) $(LUA_CFLAGS)

#
# Shared library
#

wreportincludedir = $(includedir)/wreport

# Include the .cc files that contain template definitions
wreportinclude_HEADERS = \
	aliases.h \
	conv.h \
	dtable.h \
	error.h \
	fast.h \
	bulletin.h \
	opcode.h \
	subset.h \
	var.h \
	varinfo.h \
	vartable.h

lib_LTLIBRARIES = libwreport.la
libwreport_la_SOURCES = \
	error.cc \
	uniconv.cc \
	conv.cc \
	varinfo.cc \
	aliases.cc \
	vartable.cc \
	var.cc \
	template-aliases.cc \
	opcode.cc \
	dtable.cc \
	subset.cc \
	bulletin.cc \
	bufr_decoder.cc \
	bufr_encoder.cc \
	crex_decoder.cc \
	crex_encoder.cc

if LUA
libwreport_la_SOURCES += var-lua.cc
endif

libwreport_la_LDFLAGS = -version-info @LIBDBALLE_VERSION_INFO@ $(LUA_LIBS)

uniconv.cc: uniconv.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

aliases.cc: aliases.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

template-aliases.cc: template-aliases.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

template-aliases.tex: template-aliases.gperf mktaliases
	if ! $(srcdir)/mktaliases < $< ; then rm $@; /bin/false; fi
	
aliases.dox: aliases.gperf mkaliasdoc
	$(srcdir)/mkaliasdoc dox < $< > $@

aliases.tex: aliases.gperf mkaliasdoc
	$(srcdir)/mkaliasdoc tex < $< > $@

btable.dox: $(top_srcdir)/tables/dballe.txt $(top_srcdir)/tables/mkbdoc
	$(top_srcdir)/tables/mkbdoc dox < $< > $@ 

distclean-local:
	rm -f template-aliases.tex aliases.dox aliases.tex btable.dox

noinst_DATA = aliases.dox aliases.tex btable.dox

BUILT_SOURCES = aliases.cc uniconv.cc template-aliases.cc template-aliases.tex
EXTRA_DIST = aliases.gperf mkaliasdoc uniconv.gperf template-aliases.gperf mktaliases

#
# Unit testing
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/extra/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = test-utils-wreport.h

tut_test_SOURCES = \
	test-utils-wreport.cc \
	tests/error.cc \
	tests/conv.cc \
	tests/fast.cc \
	tests/varinfo.cc \
	tests/aliases.cc \
	tests/vartable.cc \
	tests/var.cc \
	tests/opcode.cc \
	tests/dtable.cc \
	tests/bulletin.cc \
	tests/bufr_decoder.cc \
	tests/bufr_encoder.cc \
	tests/crex_decoder.cc \
	tests/tut-main.cpp
tut_test_LDADD = \
	libwreport.la \
	$(WIBBLE_LIBS)

if LUA
noinst_HEADERS += test-utils-lua.h
tut_test_SOURCES += \
	tests/lua.cc \
	test-utils-lua.cc
endif

