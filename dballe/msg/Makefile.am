## Process this file with automake to produce Makefile.in

messageincludedir = $(includedir)/dballe/msg

# Include the .cc files that contain template definitions
messageinclude_HEADERS = \
		datum.h \
		level.h \
		vars.h \
		msg.h \
		msgs.h \
		readers.h \
		writers.h \
		marshal.h \
		file.h \
		aof_codec.h \
		bufrex_codec.h

lib_LTLIBRARIES = libdballe-msg.la
libdballe_msg_la_SOURCES = \
		vars.c \
		datum.c \
		level.c \
		msg.c \
		msgs.c \
		readers.c \
		writers.c \
		marshal.c \
		file.c \
		aof_codec.c \
		aof_importers/common.c \
		aof_importers/synop.c \
		aof_importers/dribu.c \
		aof_importers/flight.c \
		aof_importers/temp.c \
		aof_importers/pilot.c \
		bufrex_codec.c \
		bufrex_importers/generic.c \
		bufrex_importers/synop.c \
		bufrex_importers/metar.c \
		bufrex_importers/pilot.c \
		bufrex_importers/temp.c \
		bufrex_importers/flight.c \
		bufrex_importers/sat.c \
		bufrex_exporters/exporters.c \
		bufrex_exporters/generic.c \
		bufrex_exporters/synop.c \
		bufrex_exporters/metar.c \
		bufrex_exporters/ship.c \
		bufrex_exporters/buoy.c \
		bufrex_exporters/pilot.c \
		bufrex_exporters/temp.c \
		bufrex_exporters/flight.c

vars.h vars.c: vars.csv mkvars
	./mkvars < $<

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

EXTRA_DIST = msg.dox mkvars vars.csv vars.h vars.c aof_importers/common.h bufrex_exporters/exporters.h bufrex_exporters/exporters.dox
