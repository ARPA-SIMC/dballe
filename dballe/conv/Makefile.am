## Process this file with automake to produce Makefile.in

convincludedir = $(includedir)/dballe/conv

convinclude_HEADERS = \
	conv.h

noinst_LTLIBRARIES = libconv.la
libconv_la_SOURCES = \
	uniconv.c \
	conv.c

uniconv.c: uniconv.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

distclean-local:
	rm -f uniconv.c

EXTRA_DIST = conv.dox uniconv.gperf
