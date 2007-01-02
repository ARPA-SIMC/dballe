## Process this file with automake to produce Makefile.in

coreincludedir = $(includedir)/dballe/core

coreinclude_HEADERS = \
	error.h \
	conv.h \
	verbose.h \
	fast.h \
	csv.h \
	aliases.h \
	record.h \
	vartable.h \
	var.h

noinst_LTLIBRARIES = libcore.la
libcore_la_SOURCES = \
	error.c \
	uniconv.c \
	conv.c \
	verbose.c \
	csv.c \
	aliases.c \
	record_keyword.c \
	record.c \
	vartable.c \
	var.c

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

noinst_DATA = record_keyword.dox record_keyword.rst record_keyword.tex aliases.dox aliases.tex

uniconv.c: uniconv.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

record_keyword.c: record_keyword.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

aliases.c: aliases.gperf
	if ! gperf < $< > $@; then rm $@; /bin/false; fi

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
