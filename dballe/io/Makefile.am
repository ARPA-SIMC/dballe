## Process this file with automake to produce Makefile.in

ioincludedir = $(includedir)/dballe/io

# Include the .cc files that contain template definitions
ioinclude_HEADERS = \
		readers.h \
		writers.h

noinst_LTLIBRARIES = libio.la
libio_la_SOURCES = \
		readers.c \
		writers.c

EXTRA_DIST = io.dox
