## Process this file with automake to produce Makefile.in

convincludedir = $(includedir)/dballe/conv

convinclude_HEADERS = \
	dba_conv.h

noinst_LTLIBRARIES = libconv.la
libconv_la_SOURCES = \
	dba_uniconv.c \
	dba_conv.c

dba_uniconv.c: dba_uniconv.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

distclean-local:
	rm -f dba_uniconv.c

EXTRA_DIST = conv.dox dba_uniconv.gperf
