## Process this file with automake to produce Makefile.in

coreincludedir = $(includedir)/dballe/core

coreinclude_HEADERS = \
	verbose.h \
	dba_csv.h \
	dba_array.h \
	dba_record.h \
	dba_varconv.h \
	dba_vartable.h \
	dba_var.h

noinst_LTLIBRARIES = libcore.la
libcore_la_SOURCES = \
	verbose.c \
	dba_csv.c \
	dba_array.c \
	dba_record_keyword.c \
	dba_record.c \
	dba_varconv.c \
	dba_vartable.c \
	dba_var.c

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

noinst_DATA = dba_record_keyword.dox dba_record_keyword.rst dba_record_keyword.tex

dba_record_keyword.c: dba_record_keyword.gperf
	gperf < $< > $@

dba_record_keyword.rst: dba_record_keyword.gperf mkkeydoc
	./mkkeydoc rst < $< > $@

dba_record_keyword.dox: dba_record_keyword.gperf mkkeydoc
	./mkkeydoc dox < $< > $@

dba_record_keyword.tex: dba_record_keyword.gperf mkkeydoc
	./mkkeydoc tex < $< > $@

dba_varconv.c: dba_varconv.gperf compile_varconv
	./compile_varconv < $< | gperf > $@

distclean-local:
	rm -f dba_record_keyword.c dba_record_keyword.dox dba_varconv.c

EXTRA_DIST = core.dox dba_record_keyword.gperf mkkeydoc dba_varconv.gperf compile_varconv
