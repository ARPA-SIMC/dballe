## Process this file with automake to produce Makefile.in

aofincludedir = $(includedir)/dballe/aof

# Include the .cc files that contain template definitions
aofinclude_HEADERS = \
		decoder.h

noinst_LTLIBRARIES = libaof.la
libaof_la_SOURCES = \
		decoder.c \
		importers/common.c \
		importers/synop.c \
		importers/dribu.c \
		importers/flight.c \
		importers/temp.c \
		importers/pilot.c

EXTRA_DIST = aof.dox importers/common.h
