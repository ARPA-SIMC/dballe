## Process this file with automake to produce Makefile.in

bufrexincludedir = $(includedir)/dballe/bufrex

# Include the .cc files that contain template definitions
bufrexinclude_HEADERS = \
		bufrex_dtable.h \
		bufrex_opcode.h \
		bufrex_raw.h \
		bufrex.h

noinst_LTLIBRARIES = libbufrex.la
libbufrex_la_SOURCES = \
		bufrex_dtable.c \
		bufrex_opcode.c \
		bufrex_raw.c \
		bufr_decoder.c \
		bufr_encoder.c \
		crex_decoder.c \
		crex_encoder.c \
		bufrex_raw_conv.c \
		bufrex.c \
		importers/im-generic.c \
		importers/im-synop.c \
		importers/im-metar.c \
		importers/im-sounding.c \
		importers/im-flight.c \
		exporters/ex-generic.c \
		exporters/ex-synop.c \
		exporters/ex-metar.c \
		exporters/ex-ship.c \
		exporters/ex-buoy.c \
		exporters/ex-pilot.c \
		exporters/ex-temp.c \
		exporters/ex-flight.c

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

#EXTRA_DIST = exporters.h exporters.dox
EXTRA_DIST = bufrex.dox exporters/exporters.h exporters/exporters.dox
