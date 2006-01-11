## Process this file with automake to produce Makefile.in

messageincludedir = $(includedir)/dballe/msg

# Include the .cc files that contain template definitions
messageinclude_HEADERS = \
		datum.h \
		level.h \
		vars_h.inc \
		dba_msg.h

noinst_LTLIBRARIES = libmsg.la
libmsg_la_SOURCES = \
		datum.c \
		level.c \
		dba_msg.c

dba_msg.c: vars_h.inc vars_c.inc

vars_h.inc vars_c.inc: vars.csv mkvars
	./mkvars < $<

EXTRA_DIST = msg.dox mkvars vars.csv vars_h.inc vars_c.inc
