## Process this file with automake to produce Makefile.in

ioincludedir = $(includedir)/dballe/io

# Include the .cc files that contain template definitions
ioinclude_HEADERS = \
		dba_rawmsg.h \
		dba_rawfile.h \
		readers.h \
		writers.h

noinst_LTLIBRARIES = libio.la
libio_la_SOURCES = \
		dba_rawmsg.c \
		dba_rawfile.c \
		readers.c \
		writers.c

EXTRA_DIST = io.dox
