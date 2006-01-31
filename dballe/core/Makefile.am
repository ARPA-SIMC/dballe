## Process this file with automake to produce Makefile.in

coreincludedir = $(includedir)/dballe/core

coreinclude_HEADERS = \
	verbose.h \
	fast.h \
	csv.h \
	dba_array.h \
	dba_record.h \
	dba_vartable.h \
	dba_var.h

noinst_LTLIBRARIES = libcore.la
libcore_la_SOURCES = \
	verbose.c \
	csv.c \
	record_keyword.c \
	dba_array.c \
	dba_record.c \
	dba_vartable.c \
	dba_var.c

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

noinst_DATA = record_keyword.dox record_keyword.rst record_keyword.tex

record_keyword.c: record_keyword.gperf
	gperf < $< > $@

record_keyword.rst: record_keyword.gperf mkkeydoc
	./mkkeydoc rst < $< > $@

record_keyword.dox: record_keyword.gperf mkkeydoc
	./mkkeydoc dox < $< > $@

record_keyword.tex: record_keyword.gperf mkkeydoc
	./mkkeydoc tex < $< > $@

distclean-local:
	rm -f record_keyword.c record_keyword.dox dba_varconv.c

EXTRA_DIST = core.dox record_keyword.gperf mkkeydoc
