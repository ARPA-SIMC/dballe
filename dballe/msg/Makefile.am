## Process this file with automake to produce Makefile.in

messageincludedir = $(includedir)/dballe/msg

# Include the .cc files that contain template definitions
messageinclude_HEADERS = \
		datum.h \
		level.h \
		vars.h \
		dba_msg.h

noinst_LTLIBRARIES = libmsg.la
libmsg_la_SOURCES = \
		vars.c \
		datum.c \
		level.c \
		dba_msg.c

vars.h vars.c: vars.csv mkvars
	./mkvars < $<

EXTRA_DIST = msg.dox mkvars vars.csv vars.h vars.c
