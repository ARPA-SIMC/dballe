## Process this file with automake to produce Makefile.in

ioincludedir = $(includedir)/dballe/io

# Include the .cc files that contain template definitions
ioinclude_HEADERS = \
		encoding.h \
		dba_rawmsg.h \
		dba_rawfile.h \
		dba_file_readers.h \
		dba_file_writers.h

noinst_LTLIBRARIES = libio.la
libio_la_SOURCES = \
		dba_rawmsg.c \
		dba_rawfile.c \
		dba_file_readers.c \
		dba_file_writers.c

EXTRA_DIST = io.dox
