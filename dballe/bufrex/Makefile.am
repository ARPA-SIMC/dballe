## Process this file with automake to produce Makefile.in

bufrexincludedir = $(includedir)/dballe/bufrex

# Include the .cc files that contain template definitions
bufrexinclude_HEADERS = \
		bufrex_dtable.h \
		bufrex_opcode.h \
		bufrex_subset.h \
		bufrex_msg.h \
		bufrex.h

noinst_LTLIBRARIES = libbufrex.la
libbufrex_la_SOURCES = \
		bufrex_dtable.c \
		bufrex_opcode.c \
		bufrex_subset.c \
		bufrex_msg.c \
		bufr_decoder.c \
		bufr_encoder.c \
		crex_decoder.c \
		crex_encoder.c \
		bufrex_raw_conv.c \
		bufrex.c \
		importers/generic.c \
		importers/synop.c \
		importers/metar.c \
		importers/pilot.c \
		importers/temp.c \
		importers/flight.c \
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
