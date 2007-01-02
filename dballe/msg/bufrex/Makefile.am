## Process this file with automake to produce Makefile.in

bufrexincludedir = $(includedir)/dballe/bufrex

# Include the .cc files that contain template definitions
bufrexinclude_HEADERS = \
		bufrex.h

noinst_LTLIBRARIES = libbufrex.la
libbufrex_la_SOURCES = \
		bufrex_raw_conv.c \
		bufrex.c \
		importers/generic.c \
		importers/synop.c \
		importers/metar.c \
		importers/pilot.c \
		importers/temp.c \
		importers/flight.c \
		importers/sat.c \
		exporters/exporters.c \
		exporters/generic.c \
		exporters/synop.c \
		exporters/metar.c \
		exporters/ship.c \
		exporters/buoy.c \
		exporters/pilot.c \
		exporters/temp.c \
		exporters/flight.c

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

#EXTRA_DIST = exporters.h exporters.dox
EXTRA_DIST = bufrex.dox exporters/exporters.h exporters/exporters.dox
