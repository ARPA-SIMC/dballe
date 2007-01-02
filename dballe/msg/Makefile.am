## Process this file with automake to produce Makefile.in

SUBDIRS = . bufrex

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

noinst_LTLIBRARIES = libmsg.la
libmsg_la_SOURCES = \
		vars.c \
		datum.c \
		level.c \
		msg.c \
		msgs.c \
		readers.c \
		writers.c \
		marshal.c \
		file.c \
		aof_decoder.c \
		aof_importers/common.c \
		aof_importers/synop.c \
		aof_importers/dribu.c \
		aof_importers/flight.c \
		aof_importers/temp.c \
		aof_importers/pilot.c \
		bufrex_codec.c \
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

vars.h vars.c: vars.csv mkvars
	./mkvars < $<

INCLUDES = -DTABLE_DIR=\"$(tabledir)\"

EXTRA_DIST = msg.dox mkvars vars.csv vars.h vars.c exporters/exporters.h exporters/exporters.dox
